#include <iomanip>

#include <openssl/sha.h>

namespace crypto {
    std::string sha1_raw(const std::string& inp) {
        unsigned char digest[SHA_DIGEST_LENGTH];
        SHA1(
            reinterpret_cast<const unsigned char*>(inp.c_str()), inp.size(), digest);
        return std::string(reinterpret_cast<char*>(digest), SHA_DIGEST_LENGTH);
    }

    std::string sha1_hex(const std::string& inp) {
        std::stringstream ss;
        unsigned char digest[SHA_DIGEST_LENGTH];

        SHA1(
            reinterpret_cast<const unsigned char*>(inp.c_str()), inp.size(), digest);

        for (unsigned char i : digest) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i);
        }
        return ss.str();
    }

    size_t get_sha1_size() {
        return SHA_DIGEST_LENGTH;
    }
}
