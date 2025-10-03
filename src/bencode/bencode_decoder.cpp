#include <iostream>
#include <stack>
#include "../lib/nlohmann/json.hpp"
#include "bencode.h"
#include "../lib/utf8cpp/utf8.h"

using json = nlohmann::json;

namespace bencode {
    EncodeType peek_type(const char c) {
        if (std::isdigit(c)) {
            return EncodeType::String;
        }
        switch (c) {
            case 'i': return EncodeType::Integer;
            case 'l': return EncodeType::List;
            case 'd': return EncodeType::Dict;
            case 'e': return EncodeType::End;
            default: return EncodeType::Unknown;
        }
    }

    Decoder::Decoder(const std::string& encoded_value) {
        this->encoded_value = encoded_value;
        this->original_encoded_value = encoded_value;
    }

    json render_json_dictionary(std::map<std::string, json>& dict) {
        json result = json::object();
        for (const auto& [k, v] : dict) {
            result[k] = v;
        }
        return result;
    }

    json render_json_list(const std::vector<json>& lst) {
        json result = json::array();
        for (const auto& v : lst) {
            result.push_back(v);
        }
        return result;
    }

    void Decoder::add_to_current_list(const json& val) {
        assert(!lst_entries.empty() && "Inside list but no list entries");
        std::vector<json> lst = lst_entries.top();
        lst_entries.pop();
        lst.push_back(val);
        lst_entries.push(lst);
    }

    void Decoder::add_to_current_dict(const json& val) {
        assert(!dict_entries.empty() && "Inside dict but no dict entries");
        assert(dict_keys.size() == dict_entries.size() && "Inside dict with int but no dict key");
        // we are in a dictionary and a key for our entry already exist
        std::string key = dict_keys.top();
        dict_keys.pop();

        std::map<std::string, json> dict = dict_entries.top();
        dict_entries.pop();

        dict.insert_or_assign(key, val);

        dict_entries.push(dict);
    }

    void Decoder::open_dict() {
        curr_collection.push(EncodeType::Dict);

        const std::map<std::string, json> dict;
        dict_entries.push(dict);

        encoded_value = encoded_value.substr(1);
    }

    void Decoder::open_list() {
        curr_collection.push(EncodeType::List);

        constexpr std::vector<json> lst;
        lst_entries.push(lst);

        encoded_value = encoded_value.substr(1);
    }

    void Decoder::parse_int() {
        const size_t e_idx = encoded_value.find('e');
        assert(e_idx != std::string::npos && "Int value has no end char");
        const std::string number_string = encoded_value.substr(1, e_idx);

        int64_t number = std::atoll(number_string.c_str());
        const json number_json = json(number);

        encoded_value = encoded_value.substr(number_string.size() + 1); // + 1 is 'e' char

        if (curr_collection.empty()) {
            // it is single integer
            assert(encoded_value.empty() && "No collection but more than one value");
            result = number_json;
        } else {
            EncodeType curr_coll = curr_collection.top();

            if (curr_coll == EncodeType::Dict) {
                add_to_current_dict(number_json);
            }
            if (curr_coll == EncodeType::List) {
                add_to_current_list(number_json);
            }
        }
    }

    void Decoder::parse_string() {
        const size_t colon_index = encoded_value.find(':');
        assert(colon_index != std::string::npos && "String value has no ':'");

        const std::string number_string = encoded_value.substr(0, colon_index);
        const int64_t str_size = std::atoll(number_string.c_str());

        std::string string_value = encoded_value.substr(colon_index + 1, str_size);
        json json_str;
        if (utf8::is_valid(string_value)) {
            json_str = json(string_value);
        } else {
            json_str = json::binary(std::vector<std::uint8_t>(string_value.begin(), string_value.end()));
        }

        encoded_value = encoded_value.substr(colon_index + 1 + str_size);

        if (curr_collection.empty()) {
            // it is single string
            assert(encoded_value.empty() && "No collection but more than one value");
            result = json_str;
        } else {
            const EncodeType curr_coll = curr_collection.top();

            if (curr_coll == EncodeType::Dict) {
                assert((dict_keys.size() == dict_entries.size() || dict_keys.size() == dict_entries.size() - 1) && "Wrong number of dict keys to dict entries");
                if (dict_keys.size() == dict_entries.size()) {
                    // we are in a dictionary and a key for our entry already exist
                    add_to_current_dict(json_str);
                } else {
                    assert(!dict_entries.empty() && "Inside dict but no dict entries");
                    // we are in a dict and there is no key
                    dict_keys.push(string_value);
                }
            }
            if (curr_coll == EncodeType::List) {
                add_to_current_list(json_str);
            }
        }
    }

    void Decoder::close_collection() {
        assert(!curr_collection.empty() && "Closing collection but collection stack is empty");

        const EncodeType curr_coll = curr_collection.top();
        curr_collection.pop();

        assert(!(curr_coll == EncodeType::Dict && dict_entries.empty()) && "No dict entries closing dictionary");
        assert(!(curr_coll == EncodeType::Dict && dict_entries.size() == dict_keys.size()) && "Same number of dict entries and keys when closing coll");
        assert(!(curr_coll == EncodeType::List && lst_entries.empty()) && "No lst entries closing list");

        encoded_value = encoded_value.substr(1);

        json json_coll;

        assert((curr_coll == EncodeType::Dict || curr_coll == EncodeType::List) && "Invalid collection type");
        if (curr_coll == EncodeType::Dict) {
            std::map<std::string, json> dict = dict_entries.top();
            dict_entries.pop();
            json_coll = render_json_dictionary(dict);
        } else {
            std::vector<json> lst = lst_entries.top();
            lst_entries.pop();
            json_coll = render_json_list(lst);
        }

        assert((curr_collection.empty() || curr_collection.top() == EncodeType::List || curr_collection.top() == EncodeType::Dict) && "Wrong collection type");
        if (curr_collection.empty()) {
            // means we arrived to the end of nesting, this was the last collection
            assert(encoded_value.empty() && "Closing last collection but still have chars to read");
            result = json_coll;
        } else if (curr_collection.top() == EncodeType::List) {
            // means we are inside a list, the list values will continue
            // render the json of the collection and add to the list values
            add_to_current_list(json_coll);
        } else {
            // means we are inside a dict, the dict values will continue
            // render the json of the collection and add to the list values
            assert(!(dict_keys.empty() || dict_entries.empty() || dict_keys.size() != dict_entries.size()) && "Closed a collection inside a dict but it is in wrong state");
            add_to_current_dict(json_coll);
        }
    }

    json Decoder::decode_bencoded_value() {
        while (!encoded_value.empty()) {
            const char init_char = encoded_value[0];
            switch (peek_type(init_char)) {
                case EncodeType::Dict:
                    open_dict();
                    break;
                case EncodeType::List:
                    open_list();
                    break;
                case EncodeType::Integer:
                    parse_int();
                    break;
                case EncodeType::String:
                    parse_string();
                    break;
                case EncodeType::End:
                    close_collection();
                    break;
                default: throw std::runtime_error("Invalid input: " + encoded_value);
            }
        }
        assert(curr_collection.empty() && "Did not close all collections");

        return result;
    }
}
