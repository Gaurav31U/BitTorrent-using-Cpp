//
// Created by Isaac Lefrançois on 5/15/24.
//

#ifndef BITTORRENT_STARTER_CPP_URL_H
#define BITTORRENT_STARTER_CPP_URL_H

#include <curl/curl.h>
#include <fmt/core.h>
#include <magic_enum.hpp>


namespace http {
    struct Url
    {

        /**
         * curl_url() - Create a new URL handle
         */
        inline Url() : _handle(curl_url()) {}

        inline ~Url() {
            curl_url_cleanup(_handle);
        }

        /**
         * =delete - Disable copy constructor
         */
        inline Url(const Url&) = delete;

        /**
         * =delete - Disable copy assignment
         */
        inline Url& operator=(const Url&) = delete;

        /**
         * =delete - Disable move constructor
         */
        inline Url(Url&&) = delete;

        /**
         * =delete - Disable move assignment
         */
        inline Url& operator=(Url&&) = delete;

        /**
         * set - Set the URL
         * @param url - The URL to set
         */
         inline Url& base(const std::string& url)
         {
             _set(CURLUPart::CURLUPART_URL, url, 0);
             return *this;
         }

         /**
          * set_scheme - Set the URL scheme
          */

        inline Url& scheme(const std::string& scheme)
        {
            _set(CURLUPart::CURLUPART_SCHEME, scheme, CURLU_NON_SUPPORT_SCHEME);
            return *this;
        }

        template<typename ParamT>
        inline Url& query(std::string_view name, ParamT param)
        {
            _set(
                CURLUPart::CURLUPART_QUERY,
                fmt::format("{0}={1}", name, param),
                CURLU_APPENDQUERY
            );
            
            return *this;
        }

        inline std::string to_string() {
            return _get(CURLUPart::CURLUPART_URL, CURLU_NO_DEFAULT_PORT);
        }

    private:
        inline void _set(CURLUPart what, std::string_view value, unsigned int flags) {
            CURLUcode code = curl_url_set(_handle, what, value.data(), flags);
            if (code != CURLUcode::CURLUE_OK) {
                throw std::invalid_argument(
                    fmt::format(
                        "Can not set URL part {0} with provided argument {1}. Error "
                        "code: {2}",
                        magic_enum::enum_name(what), value, magic_enum::enum_name(code)
                    )
                  );
            }
        }

        inline std::string _get(CURLUPart what, unsigned int flags) {
            char* value;
            CURLUcode code = curl_url_get(_handle, what, &value, flags);
            if (code != CURLUcode::CURLUE_OK) {
                curl_free(value);
                throw std::invalid_argument(fmt::format(
                  "Can not get URL part {0}. Error code: {1}",
                  magic_enum::enum_name(what), magic_enum::enum_name(code)
                ));
            }
            std::string result(value);
            curl_free(value);

            return result;
        }

        Curl_URL* _handle;
    };

}  // namespace http

#endif  // BITTORRENT_STARTER_CPP_URL_H
