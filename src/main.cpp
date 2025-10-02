#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json recursion_decode(const std::string& encoded_value, size_t& index) {
    if (index >= encoded_value.length()) {
        throw std::runtime_error("Unexpected end of encoded value");
    }

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
    } else if (encoded_value[index] == 'i') {
        // Decode integer
        size_t end_index = encoded_value.find('e', index);
        if (end_index == std::string::npos) {
            throw std::runtime_error("Invalid integer encoding");
        }
        std::string number_string = encoded_value.substr(index + 1, end_index - index - 1);
        int64_t number = std::atoll(number_string.c_str());
        index = end_index + 1;
        return json(number);
    } else if (encoded_value[index] == 'l') {
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
    } else {
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
        json decoded_value = recursion_decode(encoded_value);
        std::cout << decoded_value.dump() << std::endl;
    } else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
