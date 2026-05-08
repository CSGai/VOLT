cmake_minimum_required(VERSION 3.14)

project(TradingBot LANGUAGES CXX)

# ========================
# Global settings
# ========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static on Windows, shared on Linux (less pain)
if(WIN32)
    set(BUILD_SHARED_LIBS OFF)
else()
    set(BUILD_SHARED_LIBS ON)
endif()

# ========================
# MSVC debug fix
# ========================
if(MSVC AND POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")
endif()

include(FetchContent)

# ========================
# CPR (HTTP)
# ========================
set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
set(CPR_FORCE_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)

# curl config (important for Linux!)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)

# SSL handling
if(WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_STATICLIB ON CACHE BOOL "" FORCE)
else()
    # On Linux use OpenSSL (default)
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)

# ========================
# nlohmann/json
# ========================
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# ========================
# pugixml
# ========================
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
)

# ========================
# mbedTLS (TLS backend for ixwebsocket)
# ========================
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG v3.6.2
)

# ========================
# ixwebsocket (cross-platform WSS client)
# ========================
set(USE_TLS ON CACHE BOOL "" FORCE)
set(USE_MBED_TLS ON CACHE BOOL "" FORCE)
set(USE_OPEN_SSL OFF CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.4
)

# ========================
# Fetch deps
# ========================
FetchContent_MakeAvailable(
    cpr
    json
    pugixml
    mbedtls
)

# Short-circuit ixwebsocket's find_package(MbedTLS REQUIRED) by pre-populating
# the vars from the mbedtls FetchContent targets before bringing in ixwebsocket.
set(MBEDTLS_FOUND TRUE)
set(MBEDTLS_INCLUDE_DIRS "${mbedtls_SOURCE_DIR}/include")
set(MBEDTLS_LIBRARIES mbedtls mbedx509 mbedcrypto)

FetchContent_MakeAvailable(ixwebsocket)

# ========================
# Executable
# ========================
add_executable(TradingBot
    src/TradingBot.cpp

    # market
    src/services/collectors/market/Yahoo.cpp
    src/services/collectors/market/wss.cpp

    # news
    src/services/collectors/news/Rss.cpp
    src/services/collectors/news/yahoo_rss.cpp
    # utils
    src/utils/datetime.cpp
    src/utils/misc.cpp
    src/utils/Scheduler.cpp
)

target_include_directories(TradingBot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ========================
# Link
# ========================
target_link_libraries(TradingBot PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    pugixml::pugixml
    ixwebsocket
)

# ========================
# Platform fixes
# ========================
if(MSVC)
    target_compile_options(TradingBot PRIVATE /wd4267)
endif()

# Linux needs these sometimes when curl pulls them transitively
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(TradingBot PRIVATE Threads::Threads dl)
endif()