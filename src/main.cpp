#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include "sha1.h"

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json recursion_decode(const std::string& encoded_value, size_t& index) {
    if (index >= encoded_value.length()) {
        throw std::runtime_error("Unexpected end of encoded value");
    }
    // d<key1><value1>...<keyN><valueN>e
    if (std::isdigit(encoded_value[index])) {
        // Decode string
        size_t colon_index = encoded_value.find(':', index);
        if (colon_index == std::string::npos) {
            throw std::runtime_error("Invalid string encoding");
        }
        std::string number_string = encoded_value.substr(index, colon_index - index);
        int64_t number = std::atoll(number_string.c_str());
        index = colon_index + 1;
        std::string str = encoded_value.substr(index, number);
        index += number;
        return json(str);
    } else 
    
    
    if (encoded_value[index] == 'i') {
        // Decode integer
        size_t end_index = encoded_value.find('e', index);
        if (end_index == std::string::npos) {
            throw std::runtime_error("Invalid integer encoding");
        }
        std::string number_string = encoded_value.substr(index + 1, end_index - index - 1);
        int64_t number = std::atoll(number_string.c_str());
        index = end_index + 1;
        return json(number);
    }else 
    
    
    if (encoded_value[index] == 'l') {
        // Decode list
        index++; // Skip 'l'
        std::vector<json> list;
        while (index < encoded_value.length() && encoded_value[index] != 'e') {
            list.push_back(recursion_decode(encoded_value, index));
        }
        if (index >= encoded_value.length() || encoded_value[index] != 'e') {
            throw std::runtime_error("Invalid list encoding");
        }
        index++; // Skip 'e'
        return json(list);
    } else 
    
    
    if(encoded_value[index]== 'd'){
        // Decode dictionary
        index++; // Skip 'd'
        json dict = json::object();
        while (index < encoded_value.length() && encoded_value[index] != 'e') {
            json key = recursion_decode(encoded_value, index);
            if (!key.is_string()) {
                throw std::runtime_error("Dictionary keys must be strings");
            }
            json value = recursion_decode(encoded_value, index);
            dict[key.get<std::string>()] = value;
        }
        if (index >= encoded_value.length() || encoded_value[index] != 'e') {
            throw std::runtime_error("Invalid dictionary encoding");
        }
        index++; // Skip 'e'
        return dict;
    
    }else {
        throw std::runtime_error("Unhandled encoded value at index " + std::to_string(index) + ": " + encoded_value);
    }
}

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


int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        std::cerr << "Logs from your program will appear here!" << std::endl;

        // Uncomment this block to pass the first stage
        std::string encoded_value = argv[2];
        size_t idx = 0;
        json decoded_value = recursion_decode(encoded_value,idx);
        std::cout << decoded_value.dump() << std::endl;
    } else 
    
    if(command == "info"){
        if(argc < 3) {
            std::cerr << "Usage: " << argv[0] << " info file.torent" << std::endl;
            return 1;
        }
        std::string file_name = argv[2];
        std::ifstream inFile(file_name, std::ios::in | std::ios::binary);
        if (!inFile) {
            std::cerr << "Could not open the file: " << file_name << std::endl;
            return 1;
        }
        std::string encoded_value((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        size_t idx = 0;
        json decoded_value= recursion_decode(encoded_value, idx);
        
        SHA1 sha1;
        json info_obj = decoded_value["info"];
        std::string info_bencoded = bencode_json(info_obj);
        sha1.update(info_bencoded);
        std::string binary_hash = sha1.final();


        std::string announce_url = decoded_value["announce"];
        std::cout << "Tracker URL: " << announce_url << std::endl;
        std::cout << "Length: " << decoded_value["info"]["length"] << std::endl;
        std::cout << "Info Hash: " << binary_hash << std::endl;
    }
    
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
