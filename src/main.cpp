#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
// #include "httplib.h"
#include "sha1.h"
#include "recursion_decode.h"
#include "bencode_json.h"
#include "lib/nlohmann/json.hpp"
#include <iomanip>
#include <curl/curl.h>
// #include <openssl/sha.h>

using json = nlohmann::json;

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::string *response = reinterpret_cast<std::string*>(userdata);
    response->append(ptr, size * nmemb);
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
        
        CURL *curl;
        CURLcode result;
        curl = curl_easy_init();
        if(curl == NULL) {
          cerr << "curl_easy_init() failed" << endl;
          return -1;
        }
        std::string info_value = encode_bencode_value(decoded_value["info"]);
        std::vector<uint8_t> bytes(info_value.begin(), info_value.end());
        std::string announce_url = decoded_value["announce"].get<string>();
        std::tring peer_id = "abcdefghijklmnoptrst";
        int64_t length = decoded_value["info"]["length"].get<int64_t>();
        std::ostringstream oss;
        unsigned char hash[SHA_DIGEST_LENGTH]; 
        SHA1(reinterpret_cast<const unsigned char*>(bytes.data()),
             bytes.size(),
             hash);
        oss << announce_url
            << "?info_hash=" << curl_easy_escape(curl, reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH)
            << "&peer_id="   << curl_easy_escape(curl, peer_id.c_str(), peer_id.length())
            << "&port="      << 6881
            << "&uploaded=" << 0
            << "&downloaded=" << 0
            << "&left=" << length
            << "&compact=" << 1;
        std::string url = oss.str();
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        result = curl_easy_perform(curl);
        if(result != CURLE_OK) {
          cerr << "curl_easy_perform() failed" << endl;
          return -1;
        }
        size_t begin = 0;
        json content = decode_bencoded_value(response, begin);
        std::string peers = content.at("peers").get<string>();
        for(size_t i = 0; i < peers.size(); i+=6) {
          unsigned char ip1 = static_cast<unsigned char>(peers[i]);
          unsigned char ip2 = static_cast<unsigned char>(peers[i + 1]);
          unsigned char ip3 = static_cast<unsigned char>(peers[i + 2]);
          unsigned char ip4 = static_cast<unsigned char>(peers[i + 3]);
    
          uint16_t port = (static_cast<unsigned char>(peers[i + 4]) << 8) |
                          static_cast<unsigned char>(peers[i + 5]);
    
          printf("%d.%d.%d.%d:%d\n", ip1, ip2, ip3, ip4, port);
        }
        curl_easy_cleanup(curl);
    }else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
