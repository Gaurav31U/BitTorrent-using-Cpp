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

using json = nlohmann::json;

unsigned int SHA_DIGEST_LENGTH = 20;

// helper: return length in bytes of a bencoded element starting at pos, 0 on error
static size_t bencode_element_length(const std::string &s, size_t pos) {
    if (pos >= s.size()) return 0;
    char c = s[pos];
    // integer: i<digits>e
    if (c == 'i') {
        size_t e = s.find('e', pos + 1);
        if (e == std::string::npos) return 0;
        return e - pos + 1;
    }
    // list or dict: l...e or d...e
    if (c == 'l' || c == 'd') {
        size_t i = pos + 1;
        while (i < s.size() && s[i] != 'e') {
            size_t child = bencode_element_length(s, i);
            if (child == 0) return 0;
            i += child;
        }
        if (i >= s.size() || s[i] != 'e') return 0;
        return i - pos + 1;
    }
    // string: <len>:<data>
    if (std::isdigit(static_cast<unsigned char>(c))) {
        size_t colon = s.find(':', pos);
        if (colon == std::string::npos) return 0;
        size_t num = 0;
        try {
            num = std::stoul(s.substr(pos, colon - pos));
        } catch (...) {
            return 0;
        }
        size_t start_data = colon + 1;
        if (start_data + num > s.size()) return 0;
        return (colon - pos) + 1 + num;
    }
    return 0;
}

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
        
        // compute SHA-1 over the original bencoded "info" dictionary bytes
        size_t info_key_pos = encoded_value.find("4:info");
        if (info_key_pos == std::string::npos) {
            std::cerr << "failed to find info key in torrent data" << std::endl;
            return 1;
        }
        size_t info_start = info_key_pos + 6; // skip "4:info"
        size_t info_len = bencode_element_length(encoded_value, info_start);
        if (info_len == 0) {
            std::cerr << "failed to locate bencoded info dictionary" << std::endl;
            return 1;
        }
        std::string info_raw = encoded_value.substr(info_start, info_len);
        SHA1 sha1;
        sha1.update(info_raw.data(), info_raw.size());
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
          std::cerr << "curl_easy_init() failed" << std::endl;
          return -1;
        }
        // compute SHA-1 over the original bencoded "info" dictionary bytes
        size_t info_key_pos = encoded_value.find("4:info");
        if (info_key_pos == std::string::npos) {
            std::cerr << "failed to find info key in torrent data" << std::endl;
            return -1;
        }
        size_t info_start = info_key_pos + 6; // skip "4:info"
        size_t info_len = bencode_element_length(encoded_value, info_start);
        if (info_len == 0) {
            std::cerr << "failed to locate bencoded info dictionary" << std::endl;
            return -1;
        }
        std::string info_raw = encoded_value.substr(info_start, info_len);
        SHA1 sha1;
        sha1.update(info_raw.data(), info_raw.size());
        std::string hash = sha1.final();
        std::string info_value = info_raw; // use original bytes for peers branch too

        std::string announce_url = decoded_value["announce"].get<std::string>();
        std::string peer_id = "abcdefghijklmnoptrst";
        int64_t length = decoded_value["info"]["length"].get<int64_t>();
        std::ostringstream oss;
        char *esc_info = curl_easy_escape(curl, reinterpret_cast<const char*>(hash.data()), static_cast<int>(SHA_DIGEST_LENGTH));
        char *esc_peer = curl_easy_escape(curl, peer_id.c_str(), static_cast<int>(peer_id.length()));
        oss << announce_url
            << "?info_hash=" << (esc_info ? esc_info : "")
            << "&peer_id="   << (esc_peer ? esc_peer : "")
            << "&port="      << 6881
            << "&uploaded=" << 0
            << "&downloaded=" << 0
            << "&left=" << length
            << "&compact=" << 1;
        std::string url = oss.str();
        if (esc_info) curl_free(esc_info);
        if (esc_peer) curl_free(esc_peer);
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        result = curl_easy_perform(curl);
        if(result != CURLE_OK) {
          std::cerr << "curl_easy_perform() failed" << std::endl;
          return -1;
        }
        size_t begin = 0;
        json content = recursion_decode(response, begin);
        std::string peers = content["peers"].get<std::string>();
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
