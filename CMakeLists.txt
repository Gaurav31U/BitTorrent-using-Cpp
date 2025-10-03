cmake_minimum_required(VERSION 3.13)
project(bittorrent-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

find_package(OpenSSL)

include(FetchContent)

IF (NOT nlohmann_json_FOUND)
    FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY "https://github.com/nlohmann/json"
            GIT_TAG "v3.11.3"
    )

    FetchContent_MakeAvailable(nlohmann_json)
ENDIF ()

IF (NOT fmt_FOUND)
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
            GIT_TAG "10.2.1"
    )

    FetchContent_MakeAvailable(fmt)
ENDIF ()


IF (NOT spdlog_FOUND)
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
            GIT_TAG "v1.14.1" # It's a good practice to pin a specific version for reproducibility
    )

    FetchContent_MakeAvailable(spdlog)
ENDIF ()

IF (NOT CURL_FOUND)
    FetchContent_Declare(
            CURL
            GIT_REPOSITORY "https://github.com/curl/curl"
            GIT_TAG "curl-8_7_1"
    )

    FetchContent_MakeAvailable(CURL)
ENDIF ()

IF (NOT magic_enum_FOUND)
    FetchContent_Declare(
            magic_enum
            GIT_REPOSITORY "https://github.com/Neargye/magic_enum"
            GIT_TAG "v0.9.5"
    )
    FetchContent_MakeAvailable(magic_enum)
ENDIF ()

set(SOURCE_FILES src/Main.cpp
        src/utils.h
        src/torrent.h
        src/bencode.h
        src/http/client.h
        src/http/url.h
        src/connection.h)

add_executable(bittorrent ${SOURCE_FILES})
target_link_libraries(bittorrent
        PRIVATE nlohmann_json::nlohmann_json
        fmt::fmt spdlog::spdlog
        OpenSSL::SSL OpenSSL::Crypto
        CURL::libcurl
        magic_enum::magic_enum
)