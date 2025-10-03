#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include "httplib.h"
#include "sha1.h"
#include "recursion_decode.h"
#include "bencode_json.h"
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main(int argc, char* argv[]) {
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
        std::string encoded_value = argv[2];
        size_t idx = 0;
        json decoded_value = recursion_decode(encoded_value,idx);
        std::cout << decoded_value.dump() << std::endl;
    } else if(command == "info"){
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
        std::cout << "Piece Length: " << decoded_value["info"]["piece length"] << std::endl;
        std::cout << "Piece Hashes:" << "\n";
        std::string hashes  = decoded_value.at("info").at("pieces").get<std::string>();
        std::vector<uint8_t> pieces(hashes.begin(), hashes.end());
        for (size_t i = 0; i < pieces.size(); ++i) {
            if(i % 20 == 0 && i) {
                std::cout << "\n";
            }
            printf("%02x", pieces[i]);
        }
        std::cout << "\n";
    }else if(command == "peers"){
        if(argc < 3) {
            std::cerr << "Usage: " << argv[0] << " peers <compact_peer_list>" << std::endl;
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
        std::string announce_url = decoded_value["announce"].get<std::string>();
        
        json info_obj = decoded_value["info"];
        std::string info_bencoded = bencode_json(info_obj);
        SHA1 sha1;
        sha1.update(info_bencoded);
        std::string info_hash = sha1.final();

        httplib::Client cli(announce_url);
    
        // Use the Get() method with a path and a Params object
        httplib::Params params;
        params.emplace("info_hash", decoded_value["info_hash"]);
        params.emplace("peer_id", "-PC0001-123456789012");
        params.emplace("port", "6881");
        params.emplace("uploaded", "0");
        params.emplace("downloaded", "0");
        params.emplace("left", std::to_string(decoded_value["info"]["length"].get<int>()));
        params.emplace("compact", "1");
        
        std::string path = "/get";
        // Make the GET request with the parameters
        auto res = cli.Get(path, params, httplib::Headers());

        auto print_compact_peers = [](const std::string &peers_compact) {
            std::vector<uint8_t> peers_bytes(peers_compact.begin(), peers_compact.end());
            for (size_t i = 0; i + 5 < peers_bytes.size(); i += 6) {
                std::cout << (int)peers_bytes[i] << "."
                          << (int)peers_bytes[i + 1] << "."
                          << (int)peers_bytes[i + 2] << "."
                          << (int)peers_bytes[i + 3] << ":"
                          << ((peers_bytes[i + 4] << 8) | peers_bytes[i + 5])
                          << "\n";
            }
        };
        // Check for success
        if (res && res->status == 200) {
            std::string readBuffer = res->body;
            size_t ridx = 0;
            json response;
            try {
                response = recursion_decode(readBuffer, ridx);
            } catch (...) {
                std::cerr << "Failed to decode tracker response" << std::endl;
                return 1;
            }

            if (response.is_object() && response.contains("peers") && response["peers"].is_string()) {
                std::string peers_compact = response["peers"].get<std::string>();
                print_compact_peers(peers_compact);
            } else {
                std::cerr << "Tracker response has no compact peers string" << std::endl;
            }
        } else {
            std::cerr << "HTTP GET Request Failed!" << std::endl;
            if (res) {
                std::cerr << "Status code: " << res->status << std::endl;
            }
        }

        

    }else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
