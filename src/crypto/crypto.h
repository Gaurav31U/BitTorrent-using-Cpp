
#ifndef CODECRAFTERS_BITTORRENT_CRYPTO_H
#define CODECRAFTERS_BITTORRENT_CRYPTO_H
#include <string>

namespace crypto {
    std::string sha1_raw(const std::string&);
    std::string sha1_hex(const std::string&);
    size_t get_sha1_size();
}

#endif //CODECRAFTERS_BITTORRENT_CRYPTO_H