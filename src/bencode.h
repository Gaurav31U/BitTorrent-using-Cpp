//
// Created by Isaac Lefrançois on 5/15/24.
//

#ifndef BITTORRENT_STARTER_CPP_BENCODE_H
#define BITTORRENT_STARTER_CPP_BENCODE_H

#include <map>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;


namespace bencode {

    inline std::string encode_bencode_value(const json& value) {
        json::value_t type = value.type();
        if (type == json::value_t::string) {
            std::string str = value.get<std::string>();
            return std::to_string(str.size()) + ":" + str;
        }
        else if (type == json::value_t::number_integer) {
            return "i" + std::to_string(value.get<int64_t>()) + "e";
        }
        else if (type == json::value_t::array) {
            std::string result = "l";
            for (const auto& item : value) {
                result += encode_bencode_value(item);
            }
            return result + "e";
        }
        else if (type == json::value_t::object) {
            std::string result = "d";
            for (auto it = value.begin(); it != value.end(); ++it) {
                result += encode_bencode_value(it.key()) + encode_bencode_value(it.value());
            }
            return result + "e";
        }

        return "";
    }

    inline std::tuple<json, std::string> decode_bencoded_value(const std::string& encoded_value) {
        if (std::isdigit(encoded_value[0])) {
            // Example: "5:hello" -> "hello"
            size_t colon_index = encoded_value.find(':');
            if (colon_index != std::string::npos) {
                std::string number_string = encoded_value.substr(0, colon_index);
                //            atoll is used to convert string to long long
                int64_t number = std::strtoll(number_string.c_str(), nullptr, 10);
                std::string str = encoded_value.substr(colon_index + 1, number);

                return std::make_tuple(
                  json(str), encoded_value.substr(colon_index + 1 + number)
                );
            }
            else {
                throw std::runtime_error("Invalid encoded value: " + encoded_value);
            }
        }
        else if (encoded_value[0] == 'i') {
            // Example: "i42e" -> 42
            size_t e_index = encoded_value.find('e');
            if (e_index == std::string::npos) {
                throw std::runtime_error("Invalid encoded value: " + encoded_value);
            }
            std::string number_string = encoded_value.substr(1, e_index - 1);

            int64_t number = std::strtoll(number_string.c_str(), nullptr, 10);
            return std::make_tuple(json(number), encoded_value.substr(e_index + 1));
        }
        else if (encoded_value[0] == 'l') {
            // Example: "l5:helloe" -> ["hello"]
            json list = json::array();
            // remove the 'l' from the beginning
            std::string rest = encoded_value.substr(1);
            while (rest[0] != 'e') {
                json value;
                std::tie(value, rest) = decode_bencoded_value(rest);
                list.push_back(value);
            }

            return std::make_tuple(list, rest.substr(1));
        }
        else if (encoded_value[0] == 'd') {
            // Example: "d3:cow3:moo4:spam4:eggse" -> {"cow": "moo", "spam": "eggs"}
            json dict = json::object();
            std::string rest = encoded_value.substr(1);
            while (rest[0] != 'e') {
                json key, value;
                std::tie(key, rest) = decode_bencoded_value(rest);
                std::tie(value, rest) = decode_bencoded_value(rest);
                dict[key.get<std::string>()] = value;
            }
            return std::make_tuple(dict, rest.substr(1));
        }
        else {
            throw std::runtime_error("Unhandled encoded value: " + encoded_value);
        }
    }
}  // namespace bencode

#endif  // BITTORRENT_STARTER_CPP_BENCODE_H
