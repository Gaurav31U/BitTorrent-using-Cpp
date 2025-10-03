#ifndef CODECRAFTERS_BITTORRENT_PEERS_H
#define CODECRAFTERS_BITTORRENT_PEERS_H
#include <curl/curl.h>

#include "../metainfo/metainfo.h"

namespace peers {
    struct RequestResult {
        std::vector<std::string> peers;
        int interval;

        void print_request_result();
    };
    class TrackerRequest {
        public:
            explicit TrackerRequest(const metainfo::MetaInfo& meta_info);
            RequestResult send_tracker_request();

        private:
            metainfo::MetaInfo info;
            int64_t left;
            std::string peer_id = "my peer id";
            int port = 6881;
            int uploaded = 0;
            int downloaded = 0;
            int compact = 1;

            std::string create_get_peers_url(CURL* curl);
            std::map<std::string, std::string> create_get_peers_url_params(CURL* curl) const;
            RequestResult convert_to_request_result(std::string& response);
    };
}

#endif //CODECRAFTERS_BITTORRENT_PEERS_H