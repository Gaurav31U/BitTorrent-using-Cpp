#ifndef CODECRAFTERS_BITTORRENT_METAINFO_H
#define CODECRAFTERS_BITTORRENT_METAINFO_H
#include <string>

#include "../lib/nlohmann/json.hpp"

namespace metainfo {
    class MetaInfo {
        public:
            explicit MetaInfo(const std::string& filename);
            void extract_meta_info();
            void print_meta_info() const;
            std::string get_tracker() const;
            std::string get_info_hash_raw() const;
            int64_t get_length() const;

        private:
            std::string filename;
            nlohmann::json metainfo_json;
            nlohmann::json info;
            std::string info_hash;
            std::string info_hash_raw;
            std::string tracker;
            int64_t length;
            std::string name;
            int64_t piece_length;
            std::vector<std::uint8_t> pieces;

            void assert_metainfo_content();
            void print_piece_hashes() const;

    };
}

#endif //CODECRAFTERS_BITTORRENT_METAINFO_H