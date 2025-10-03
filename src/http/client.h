//
// Created by Isaac Lefrançois on 5/15/24.
//

#ifndef BITTORRENT_STARTER_CPP_CLIENT_H
#define BITTORRENT_STARTER_CPP_CLIENT_H

#include <cstring>
#include <iostream>
#include <string>
#include <vector>


#include <curl/curl.h>

namespace http {

    struct Response {
        long status_code{};
//        CURLcode curl_code;
        std::vector<uint8_t> body;
    };


    class Client {
    public:
        inline Client() {
            _handle = curl_easy_init();
        }
        inline ~Client() {
            curl_easy_cleanup(_handle);
        }

        inline Response get(const std::string& url) {
            Response response;
            CURLcode res;

            if (_handle) {
                curl_easy_setopt(_handle, CURLOPT_URL, url.c_str());
                curl_easy_setopt(_handle, CURLOPT_HTTPGET, 1L);
                curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION, write_callback);
                curl_easy_setopt(_handle, CURLOPT_WRITEDATA, &response);
                curl_easy_setopt(_handle, CURLOPT_FOLLOWLOCATION, 1L);
//                curl_easy_setopt(_handle, CURLOPT_ERRORBUFFER, response.curl_code);
                res = curl_easy_perform(_handle);
                curl_easy_getinfo(_handle, CURLINFO_RESPONSE_CODE, &response.status_code);
            }

            if (res != CURLE_OK) {
                throw std::runtime_error(curl_easy_strerror(res));
            }

//            spdlog::info("GET {}", response.status_code);
//            spdlog::info("Status code: {}", magic_enum::enum_name(response.status_code));
//            spdlog::info("Response size: {}", response.body.size());
//            spdlog::info("CURLOPT_ERRORBUFFER {}", magic_enum::enum_name(response.curl_code));
//            spdlog::info("Response: {}", std::string(response.body.begin(), response.body.end()));

            return response;
        }

        // Write a callback function to store the response body
        inline static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userdata)
        {
            std::cout << "write_callback" << std::endl;
            size_t realsize = size * nmemb;
            auto* mem = (struct Response*) userdata;
//            auto* body = static_cast<std::vector<uint8_t>*>(userdata);
//            body->insert(body->end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + realsize);
            mem->body.insert(mem->body.end(), static_cast<uint8_t*>(contents), static_cast<uint8_t*>(contents) + realsize);
            return realsize;
        }


    private:
        CURL* _handle;
    };

}  // namespace http_client



#endif  // BITTORRENT_STARTER_CPP_CLIENT_H
