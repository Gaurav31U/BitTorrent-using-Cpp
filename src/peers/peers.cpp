
#include "peers.h"

#include <iostream>

#include "../crypto/crypto.h"

#include <curl/curl.h>

#include "../bencode/bencode.h"

namespace peers {
    void RequestResult::print_request_result() {
        for (auto peer : peers) {
            std::cout << peer << std::endl;
        }
    }

    TrackerRequest::TrackerRequest(const metainfo::MetaInfo& meta_info): info(meta_info) {
        left = info.get_length();
    }

    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* data) {
        std::string* str = reinterpret_cast<std::string*>(data);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    }

    RequestResult TrackerRequest::send_tracker_request() {
        CURL* curl = curl_easy_init();

        std::string url = create_get_peers_url(curl);

        std::string response;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "bitorrent");

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << " url: " << url << std::endl;
            RequestResult result(std::vector<std::string>(), 0);
            return result;
        }

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return convert_to_request_result(response);
    }

    std::string TrackerRequest::create_get_peers_url(CURL* curl) {
        std::map<std::string, std::string> params = create_get_peers_url_params(curl);
        std::stringstream ss;
        ss << info.get_tracker() << "?";
        for (const auto& [k, v] : params) {
            ss << k << "=" << v;
            ss << "&";
        }
        std::string url = ss.str();
        url.pop_back();
        return url;
    }

    std::map<std::string, std::string> TrackerRequest::create_get_peers_url_params(CURL* curl) const {
        std::map<std::string, std::string> params;

        std::string peer_id_hash = crypto::sha1_hex(peer_id).substr(0, 20);

        char* encoded_info_hash = curl_easy_escape(curl, info.get_info_hash_raw().c_str(), 0);

        params.insert(std::pair<std::string, std::string>("info_hash", encoded_info_hash));
        params.insert(std::pair<std::string, std::string>("peer_id", peer_id_hash));
        params.insert(std::pair<std::string, std::string>("port", std::to_string(port)));
        params.insert(std::pair<std::string, std::string>("uploaded", std::to_string(uploaded)));
        params.insert(std::pair<std::string, std::string>("downloaded", std::to_string(downloaded)));
        params.insert(std::pair<std::string, std::string>("left", std::to_string(left)));
        params.insert(std::pair<std::string, std::string>("compact", std::to_string(compact)));

        free(encoded_info_hash);

        return params;
    }

    RequestResult TrackerRequest::convert_to_request_result(std::string& response) {
        bencode::Decoder decoder(response);
        json decoded_value = decoder.decode_bencoded_value();
        int interval = decoded_value["interval"];
        nlohmann::json::binary_t& bin = decoded_value["peers"].get_binary();
        std::vector<uint8_t> ips_raw;
        ips_raw.assign(bin.begin(), bin.end());

        assert(!ips_raw.empty() && ips_raw.size() % 6 == 0 && "peers response is incorrect");

        std::vector<std::vector<uint8_t>> peers_raw;
        peers_raw.reserve(ips_raw.size() / 6);
        for (size_t i = 0; i < ips_raw.size(); i += 6) {
            std::vector<uint8_t> peer_raw;
            for (size_t j = 0; j < 6; j++) {
                peer_raw.push_back(ips_raw[i + j]);
            }
            peers_raw.emplace_back(peer_raw);
        }
        std::vector<std::string> peers;
        peers.reserve(peers_raw.size());
        for (auto peer_raw : peers_raw) {
            std::stringstream ss;
            ss << static_cast<unsigned>(peer_raw[0]) << '.';
            ss << static_cast<unsigned>(peer_raw[1]) << '.';
            ss << static_cast<unsigned>(peer_raw[2]) << '.';
            ss << static_cast<unsigned>(peer_raw[3]) << ':';
            const uint16_t peer_port = (static_cast<uint16_t>(peer_raw[4]) << 8)
                                | (static_cast<uint16_t>(peer_raw[5]) << 0);
            ss << peer_port;
            peers.emplace_back(ss.str());
        }

        RequestResult result(peers, interval);
        return result;
    }
}
