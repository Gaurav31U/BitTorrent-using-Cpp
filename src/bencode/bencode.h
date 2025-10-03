
#ifndef BENCODE_DECODER_H
#define BENCODE_DECODER_H

#include "../lib/nlohmann/json.hpp"
#include "stack"
#include "map"
#include "vector"

using json = nlohmann::json;

namespace bencode {
    enum class EncodeType {
        String, Integer, List, Dict, End, Unknown
    };
    class Decoder {
        public:
            explicit Decoder(const std::string& encoded_value);
            json decode_bencoded_value();
        private:
            std::string original_encoded_value;
            std::string encoded_value;
            json result;
            std::stack<std::string> dict_keys;
            std::stack<std::map<std::string, json>> dict_entries;
            std::stack<std::vector<json>> lst_entries;
            std::stack<EncodeType> curr_collection;

            void open_dict();
            void open_list();
            void parse_int();
            void parse_string();
            void close_collection();
            void add_to_current_list(const json& val);
            void add_to_current_dict(const json& val);
    };

    struct BEncodeItem {
        enum class Kind { Token, Value } kind;
        char token;
        json value;

        static BEncodeItem tok(char c) { return BEncodeItem{Kind::Token, c, {}}; }
        static BEncodeItem val(const json& j) { return BEncodeItem{Kind::Value, 0, j}; }
    };

    class Encoder {
        public:
            explicit Encoder(const json& decoded_value);
            std::string bencode_value();
        private:
            std::string bencoded_value;
            json decoded_value;

    };
}

#endif //BENCODE_DECODER_H
