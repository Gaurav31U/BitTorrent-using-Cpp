#include "metainfo.h"

#include <iostream>
#include <string>
#include <fstream>
#include "../bencode/bencode.h"
#include "../crypto/crypto.h"
#include "../lib/nlohmann/json.hpp"


namespace metainfo {
    MetaInfo::MetaInfo(const std::string& filename) : length(0), piece_length(0) {
        this->filename = filename;
    }

    void MetaInfo::assert_metainfo_content() {
        assert(this->metainfo_json.is_object() && "metainfo is not json object");
        assert(this->metainfo_json.contains("announce") && this->metainfo_json["announce"].is_string() && "announce field is wrong");
        assert(this->metainfo_json.contains("info") && this->metainfo_json["info"].is_object() && "info field is wrong");
        assert(info.contains("length") && info["length"].is_number() && "info->length field is wrong");
        assert(info.contains("name") && info["name"].is_string() && "info->name field is wrong");
        assert(info.contains("piece length") && info["piece length"].is_number() && "info->'piece length' field is wrong");
        assert(info.contains("pieces") && info["pieces"].is_binary() && "info->pieces field is wrong");
    }

    void MetaInfo::extract_meta_info() {
        std::ifstream in(filename, std::ios::binary);
        assert(in && "Failed to open file");

        std::ostringstream buffer;
        buffer << in.rdbuf();

        const std::string input = buffer.str();

        bencode::Decoder decoder(input);

        this->metainfo_json = decoder.decode_bencoded_value();

        this->tracker = this->metainfo_json["announce"];
        this->info = this->metainfo_json["info"];
        this->length = this->info["length"];
        this->name = this->info["name"];
        this->piece_length = this->info["piece length"];
        const nlohmann::json::binary_t* bin = this->info["pieces"].get_ptr<const nlohmann::json::binary_t*>();
        this->pieces.assign(bin->begin(), bin->end());

        assert_metainfo_content();

        bencode::Encoder encoder(this->info);
        std::string bencoded_info = encoder.bencode_value();

        this->info_hash = crypto::sha1_hex(bencoded_info);
        this->info_hash_raw = crypto::sha1_raw(bencoded_info);
    }

    void MetaInfo::print_meta_info() const {
        std::cout << "Tracker URL: " << tracker << std::endl;
        std::cout << "Length: " << length << std::endl;
        std::cout << "Info Hash: " << info_hash << std::endl;
        std::cout << "Piece Length: " << piece_length << std::endl;
        std::cout << "Piece Hashes:" << std::endl;
        print_piece_hashes();
    }

    std::string MetaInfo::get_tracker() const {
        return tracker;
    }

    std::string MetaInfo::get_info_hash_raw() const {
        return info_hash_raw;
    }

    int64_t MetaInfo::get_length() const {
        return length;
    }

    void MetaInfo::print_piece_hashes() const {
        size_t sha1_size = crypto::get_sha1_size();
        for (int i = 0; i < pieces.size(); i += 20) {
            size_t end = std::min(i + sha1_size, pieces.size());
            std::vector sha1_buffer(pieces.begin() + i, pieces.begin() + end);
            std::stringstream ss;
            ss << std::hex << std::setfill('0');
            for (auto val : sha1_buffer) {
                ss << std::setw(2) << +val;
            }
            std::cout << ss.str() << std::endl;
        }
    }

}
