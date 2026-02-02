# C++ BitTorrent Client

A lightweight, scratch-built BitTorrent client implemented in C++. This project demonstrates the core implementation of the BitTorrent protocol, including BEncoding parsing, communicating with HTTP trackers, establishing TCP connections with peers, and handling Magnet links via the Extension Protocol (BEP 09 & BEP 10).

## 🚀 Features

*   **BEncoding**: Decodes and Encodes Bencoded data (strings, integers, lists, dictionaries).
*   **Torrent File Parsing**: Extracts announce URLs, file lengths, and piece hashes from `.torrent` files.
*   **Tracker Discovery**: Connects to HTTP trackers to retrieve lists of available peers.
*   **Peer Communication**: Implements the BitTorrent Handshake protocol.
*   **File Downloading**:
    *   Downloads files piece-by-piece.
    *   Validates data integrity using SHA-1 hash verification.
    *   Handles "choke" and "unchoke" states.
*   **Magnet Link Support**:
    *   Parses magnet URIs.
    *   Implements the **Extension Protocol (BEP 10)**.
    *   Fetches metadata over the wire using `ut_metadata` **(BEP 09)**.
    *   Downloads files directly from magnet links without a `.torrent` file.

### Compilation
Link against the OpenSSL libraries (`ssl` and `crypto`) during compilation.

```bash
g++ -std=c++17 -O2 main.cpp -o bittorrent -lssl -lcrypto
```

## 💻 Usage

The program is controlled via command-line arguments. The first argument specifies the command mode.

### Working with .torrent Files

**1. Inspect a torrent file:**
Parses headers and outputs the tracker URL and Info Hash.
```bash
./bittorrent info sample.torrent
```

**2. Discover Peers:**
Contacts the tracker and lists `IP:Port` pairs of available peers.
```bash
./bittorrent peers sample.torrent
```

**3. Handshake with a Peer:**
Initiates a handshake with a specific peer to verify connection compatibility.
```bash
./bittorrent handshake sample.torrent <peer_ip>:<peer_port>
```

**4. Download a Specific Piece:**
Downloads a single piece (by index) to a local file.
```bash
./bittorrent download_piece <output_path> <sample.torrent> <piece_index>
```

**5. Download Full File:**
Downloads the entire file from the first available peer found.
```bash
./bittorrent download <output_path> <sample.torrent>
```

---

### Working with Magnet Links

**1. Parse Magnet Link:**
Extracts the Info Hash and Tracker URL from the link.
```bash
./bittorrent magnet_parse "magnet:?xt=urn:btih:..."
```

**2. Fetch Metadata (Magnet Info):**
Connects to a peer via the magnet link, uses the Extension Protocol to download the file metadata, and prints info.
```bash
./bittorrent magnet_info "magnet:?xt=urn:btih:..."
```

**3. Download Full File via Magnet:**
Resolves metadata and downloads the file completely.
```bash
./bittorrent magnet_download <output_path> "magnet:?xt=urn:btih:..."
```

---

### Utilities

**Decode BEncoded String:**
Useful for debugging raw tracker responses.
```bash
./bittorrent decode "d3:foo3:bare"
# Output: {"foo": "bar"}
```

## 📚 Technical Details

### Dependencies
*   **Arpa/Inet**: For low-level BSD socket connections.
*   **OpenSSL**: Used to generate the Info Hash and verify piece integrity.
*   **nlohmann::json**: Used to easily construct and parse BEncoded dictionaries and metadata extension payloads.

### Protocols Implemented
1.  **BitTorrent Protocol v1.0**: Standard blocking socket communication.
2.  **BEP 03 (The BitTorrent Protocol Specification)**: Core logic.
3.  **BEP 09 (Extension for Peers to Send/Receive Metadata)**: Allows magnet link downloading.
4.  **BEP 10 (Extension Protocol)**: Handles the handshake required to use BEP 09.

## ⚠️ Disclaimer

This is an educational implementation. It has the following limitations:
*   **Single Peer Connection**: It connects to only one peer at a time and downloads sequentially.
*   **Leech Only**: It does not seed (upload) data back to other peers.
*   **Blocking I/O**: Network operations are synchronous.
