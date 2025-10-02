#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(const std::string& encoded_value) {
    int length = encoded_value.length();
    if (length == 0) {
        throw std::runtime_error("Empty encoded value");
    }
    if (std::isdigit(encoded_value[0])) {
        // Example: "5:hello" -> "hello"
        size_t colon_index = encoded_value.find(':');
        if (colon_index != std::string::npos) {
            std::string number_string = encoded_value.substr(0, colon_index);
            int64_t number = std::atoll(number_string.c_str());
            std::string str = encoded_value.substr(colon_index + 1, number);
            return json(str);
        } else {
            throw std::runtime_error("Invalid encoded value: " + encoded_value);
        }
    }else if (encoded_value[0] == 'i' && encoded_value[length - 1] == 'e') {
        std::string number_string = encoded_value.substr(1, length - 2);
        int64_t number = std::atoll(number_string.c_str());
        return json(number);
    }else if(encoded_value[0]=='l' && encoded_value[length - 1]=='e'){
        std::vector<json> list;
        size_t index = 1; // Skip 'l'
        while (index < length - 1) { // Until 'e'
            size_t start_index = index;
            if (std::isdigit(encoded_value[index])) {
                size_t colon_index = encoded_value.find(':', index);
                if (colon_index != std::string::npos) {
                    std::string number_string = encoded_value.substr(index, colon_index - index);
                    int64_t number = std::atoll(number_string.c_str());
                    std::string str = encoded_value.substr(colon_index + 1, number);
                    list.push_back(json(str));
                    index = colon_index + 1 + number;
                } else {
                    throw std::runtime_error("Invalid list item in encoded value: " + encoded_value);
                }
            } else if (encoded_value[index] == 'i') {
                size_t end_index = encoded_value.find('e', index);
                if (end_index != std::string::npos) {
                    std::string number_string = encoded_value.substr(index + 1, end_index - index - 1);
                    int64_t number = std::atoll(number_string.c_str());
                    list.push_back(json(number));
                    index = end_index + 1;
                } else {
                    throw std::runtime_error("Invalid integer in list: " + encoded_value);
                }
            } else {
                throw std::runtime_error("Unsupported list item type in encoded value: " + encoded_value);
            }
        }
        return json(list);
    }
    else {
        throw std::runtime_error("Unhandled encoded value: " + encoded_value);
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
        json decoded_value = decode_bencoded_value(encoded_value);
        std::cout << decoded_value.dump() << std::endl;
    } else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
