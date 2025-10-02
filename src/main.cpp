#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include "sha1.h"
#include "recursion_decode.h"

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
