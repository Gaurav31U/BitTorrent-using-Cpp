#ifndef BENCODE_HPP
#define BENCODE_HPP
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include "sha1.h"

#include "lib/nlohmann/json.hpp"
using json = nlohmann::json;

static std::string bencode_json(const json &j) {
    if (j.is_string()) {
        const std::string &s = j.get_ref<const std::string&>();
        return std::to_string(s.size()) + ":" + s;
    } else if (j.is_number_integer() || j.is_number_unsigned() || j.is_number_float()) {
        // Use integer encoding for numeric values
        return std::string("i") + std::to_string(j.get<int64_t>()) + "e";
    } else if (j.is_array()) {
        std::string out = "l";
        for (const auto &el : j) out += bencode_json(el);
        out += "e";
        return out;
    } else if (j.is_object()) {
        std::vector<std::string> keys;
        for (auto it = j.begin(); it != j.end(); ++it) keys.push_back(it.key());
        std::sort(keys.begin(), keys.end());
        std::string out = "d";
        for (const auto &k : keys) {
            // encode key as bencoded string
            out += std::to_string(k.size()) + ":" + k;
            out += bencode_json(j.at(k));
        }
        out += "e";
        return out;
    } else if (j.is_null()) {
        return std::string("0:"); // represent null as empty string (unlikely in torrents)
    } else {
        throw std::runtime_error("Unsupported JSON type for bencoding");
    }
}

#endif