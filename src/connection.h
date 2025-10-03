//
// Created by Isaac Lefrançois on 5/16/24.
//

#ifndef BITTORRENT_STARTER_CPP_CONNECTION_H
#define BITTORRENT_STARTER_CPP_CONNECTION_H

#include <spdlog/spdlog.h>
#include <magic_enum.hpp>

#include "http/client.h"
#include "http/url.h"
#include "torrent.h"

namespace connection {

    constexpr const std::size_t PEERS_CHUNK_SIZE_BYTES = 6;

  inline std::vector<std::tuple<std::string, std::uint32_t>> get_peers(const torrent::Torrent& torrent) {
    // http://bittorrent-test-tracker.codecrafters.io/announce?compact=1&downloaded=0&info_hash=%D6%9F%91%E6%B2%AELT%24h%D1%07%3Aq%D4%EA%13%87%9A%7F&left=92063&peer_id=00112233445566778899&port=6881&uploaded=0
    http::Url url;
    url.base(torrent.announce)
      .query("peer_id", "00112233445566778899")
      .query("port", 6881)
      .query("uploaded", 0)
      .query("downloaded", 0)
      .query("left", torrent.info.length)
      .query("compact", 1)
      .query("info_hash", utils::crypto::url_encode(torrent.get_info_hash()));

    spdlog::info("Getting peers from");
    std::cout << url.to_string() << std::endl;

    auto client = http::Client();

    http::Response response = client.get(url.to_string());

    // handle response
    std::string response_str(response.body.begin(), response.body.end());

    spdlog::info("Response: {}", response_str);
    json result;
    std::tie(result, std::ignore) = bencode::decode_bencoded_value(response_str);

    spdlog::info("{}", result.dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore));

    if (result.contains("failure reason")) {
        throw new std::runtime_error("Failure reason: " + result.at("failure reason").get<std::string>());
    }

    std::string peers = result.value("peers", json::string_t());

    std::vector<std::tuple<std::string, std::uint32_t>> peer_list;

    for (size_t i = 0; i < peers.length(); i += PEERS_CHUNK_SIZE_BYTES) {
        std::string peer_id = peers.substr(i, 4);
        std::string peer_port = peers.substr(i + 4, 2);

        std::string peer_ip = std::to_string(static_cast<uint8_t>(peer_id[0])) + "." +
                              std::to_string(static_cast<uint8_t>(peer_id[1])) + "." +
                              std::to_string(static_cast<uint8_t>(peer_id[2])) + "." +
                              std::to_string(static_cast<uint8_t>(peer_id[3]));

        std::uint32_t peer_port_int = static_cast<uint8_t>(peer_port[0]) << 8 | static_cast<uint8_t>(peer_port[1]);

        peer_list.emplace_back(peer_ip, peer_port_int);
    }

    return peer_list;
  }
}

#endif  // BITTORRENT_STARTER_CPP_CONNECTION_H
