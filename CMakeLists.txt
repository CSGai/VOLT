cmake_minimum_required(VERSION 3.14)
project(TradingBot LANGUAGES C CXX)

# ========================
# Global settings
# ========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Keep all fetched deps static; avoids DLL hell on Windows
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


# ========================
# CPR (HTTP)
# ========================
# set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
# set(BUILD_CURL_EXE      OFF CACHE BOOL "" FORCE)
# set(CURL_ZLIB           OFF CACHE BOOL "" FORCE)
# set(CURL_USE_OPENSSL    ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        1.10.5
)

# ========================
# IXWebSocket  (TLS via OpenSSL)
# ========================
if(WIN32)
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64" CACHE PATH "" FORCE)
endif()

find_package(OpenSSL REQUIRED)

set(USE_TLS      ON  CACHE BOOL "" FORCE)
set(USE_OPEN_SSL ON  CACHE BOOL "" FORCE)
set(USE_MBED_TLS OFF CACHE BOOL "" FORCE)
set(USE_ZLIB     OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v11.4.5
)

# ========================
# nlohmann/json
# ========================
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

# ========================
# pugixml
# ========================
set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        v1.14
)


# ========================
# Fetch all deps
# ========================
FetchContent_MakeAvailable(
    cpr
    json
    pugixml
    ixwebsocket
)

# ========================
# Executable
# ========================
add_executable(TradingBot
    src/TradingBot.cpp

    src/services/collectors/market/Yahoo.cpp
    src/services/collectors/market/TradingView.cpp
    src/services/collectors/market/Frame.cpp
    src/services/collectors/market/cnn.cpp

    src/services/collectors/news/Rss.cpp
    src/services/collectors/news/yahoo.cpp
    src/services/collectors/news/fed.cpp

    src/utils/datetime.cpp
    src/utils/misc.cpp
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
    ixwebsocket::ixwebsocket
    OpenSSL::SSL
    OpenSSL::Crypto
)

# ========================
# Platform fixes
# ========================
if(MSVC)
    target_compile_options(TradingBot PRIVATE /wd4267)
endif()

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(TradingBot PRIVATE Threads::Threads dl)
endif()