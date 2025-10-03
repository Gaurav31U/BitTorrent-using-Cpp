#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "bencode/bencode.h"
#include "metainfo/metainfo.h"
#include "lib/nlohmann/json.hpp"
#include "peers/peers.h"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    const std::string command = argv[1];

    if (command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        std::cerr << "Logs from your program will appear here!" << std::endl;

        // Uncomment this block to pass the first stage
        std::string encoded_value = argv[2];
        bencode::Decoder decoder(encoded_value);
        json decoded_value = decoder.decode_bencoded_value();
        std::cout << decoded_value.dump() << std::endl;
    } else if (command == "info") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " info sample.torrent" << std::endl;
            return 1;
        }
        const std::string filename = argv[2];
        metainfo::MetaInfo meta_info = metainfo::MetaInfo(filename);
        meta_info.extract_meta_info();
        meta_info.print_meta_info();
    } else if (command == "peers") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " peers sample.torrent" << std::endl;
            return 1;
        }
        const std::string filename = argv[2];
        metainfo::MetaInfo meta_info = metainfo::MetaInfo(filename);
        meta_info.extract_meta_info();
        peers::TrackerRequest tracker_request(meta_info);
        peers::RequestResult request_result = tracker_request.send_tracker_request();
        request_result.print_request_result();
    } else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}