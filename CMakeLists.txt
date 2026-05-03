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
# Fetch deps
# ========================
FetchContent_MakeAvailable(
    cpr
    json
    pugixml
)

# ========================
# Executable
# ========================
add_executable(TradingBot
    src/TradingBot.cpp

    # market
    src/services/collectors/market/Yahoo.cpp

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