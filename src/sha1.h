/*
    sha1.hpp - header-only SHA-1 implementation

    Copyright (c) 2017, 2021, 2023 Peter Thorson. All rights reserved.

    This file is part of tiny-sha1.

    tiny-sha1 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    tiny-sha1 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with tiny-sha1.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SHA1_HPP
#define SHA1_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

class SHA1 {
public:
    SHA1() {
        reset();
    }

    void update(const std::string &s) {
        update(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
    }

    void update(const uint8_t *data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            m_buffer[m_buffer_len++] = data[i];
            if (m_buffer_len == 64) {
                transform();
                m_buffer_len = 0;
            }
        }
    }

    std::string final() {
        uint64_t total_bits = (m_block_count * 64 + m_buffer_len) * 8;
        m_buffer[m_buffer_len++] = 0x80;
        if (m_buffer_len > 56) {
            while (m_buffer_len < 64) {
                m_buffer[m_buffer_len++] = 0;
            }
            transform();
            m_buffer_len = 0;
        }

        while (m_buffer_len < 56) {
            m_buffer[m_buffer_len++] = 0;
        }

        for (int i = 0; i < 8; ++i) {
            m_buffer[56 + i] = (total_bits >> (56 - i * 8)) & 0xFF;
        }
        transform();

        std::string hash;
        for (int i = 0; i < 5; ++i) {
            char buf[9];
            snprintf(buf, sizeof(buf), "%08x", m_h[i]);
            hash += buf;
        }
        return hash;
    }

private:
    void transform() {
        std::array<uint32_t, 80> w;
        for (int i = 0; i < 16; ++i) {
            w[i] = (m_buffer[i * 4] << 24) | (m_buffer[i * 4 + 1] << 16) | (m_buffer[i * 4 + 2] << 8) | m_buffer[i * 4 + 3];
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = m_h[0];
        uint32_t b = m_h[1];
        uint32_t c = m_h[2];
        uint32_t d = m_h[3];
        uint32_t e = m_h[4];

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = left_rotate(b, 30);
            b = a;
            a = temp;
        }

        m_h[0] += a;
        m_h[1] += b;
        m_h[2] += c;
        m_h[3] += d;
        m_h[4] += e;

        m_block_count++;
    }

    void reset() {
        m_h[0] = 0x67452301;
        m_h[1] = 0xEFCDAB89;
        m_h[2] = 0x98BADCFE;
        m_h[3] = 0x10325476;
        m_h[4] = 0xC3D2E1F0;
        m_buffer_len = 0;
        m_block_count = 0;
    }

    uint32_t left_rotate(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }

    std::array<uint32_t, 5> m_h;
    std::array<uint8_t, 64> m_buffer;
    size_t m_buffer_len;
    uint64_t m_block_count;
};

#endif // SHA1_HPP