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
        // Tracker URL: http://bittorrent-test-tracker.codecrafters.io/announce  // without duoble quote
        // Length: 92063/
        size_t idx = 0;
        json decoded_value= recursion_decode(encoded_value, idx);
        SHA1 sha1;

        size_t start_pos = 0;
        size_t info_start = encoded_value.find("4:info", start_pos);
        if (info_start == std::string::npos) {
            throw std::runtime_error("Could not find info dictionary");
        }

        // Move to the start of the dictionary content
        info_start += 6; // Skip "4:info" prefix
        size_t info_end = info_start;
        int nested_level = 1;

        // Find the end of the info dictionary by matching nested delimiters
        while (nested_level > 0 && info_end < encoded_value.length()) {
            char c = encoded_value[info_end];
            if (c == 'd') nested_level++;
            else if (c == 'e') nested_level--;
            info_end++;
        }

        // Extract the info dictionary including the 'd' prefix and 'e' suffix
        std::string info_section = encoded_value.substr(info_start - 1, info_end - (info_start - 1));
        
        // Calculate SHA1 hash
        sha1.update(info_section);
        std::string binary_hash = sha1.final();

        // Convert binary hash to hexadecimal
        std::string hex_hash;
        for (unsigned char byte : binary_hash) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", byte);
            hex_hash += hex;
        }



        std::string announce_url = decoded_value["announce"];
        std::cout << "Tracker URL: " << announce_url << std::endl;
        std::cout << "Length: " << decoded_value["info"]["length"] << std::endl;
        std::cout << "Info Hash: " << hex_hash << std::endl;
    }
    
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
