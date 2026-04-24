cmake_minimum_required(VERSION 3.14)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("TradingBot")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

# cpr settings
set(CPR_USE_SYSTEM_CURL OFF)
set(CPR_FORCE_USE_SYSTEM_CURL OFF)
set(CURL_STATICLIB ON)
set(BUILD_CURL_EXE OFF)
set(CURL_ZLIB OFF)

include(FetchContent)

FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        v1.14
)

# FetchContent_Declare(ixwebsocket
#     GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
#     GIT_TAG v11.4.5
# )

FetchContent_MakeAvailable(
    cpr
    json
    #ixwebsocket
    pugixml
)

add_executable(TradingBot
    src/TradingBot.cpp

    # market
    src/services/collectors/market/market.h
    src/services/collectors/market/Yahoo.cpp

    # news
    src/services/collectors/news/news.h
    src/services/collectors/news/rss.cpp
    # src/services/collectors/news/wss.cpp
    # src/services/collectors/news/greed.cpp
    # src/services/collectors/news/cnbc.cpp
    # src/services/collectors/news/Article.cpp

    # utils
    src/utils/utils.h
    src/utils/json_utils.cpp
    src/utils/datetime.cpp
    src/utils/misc.cpp
    src/utils/xml_utils.cpp
)

target_link_libraries(TradingBot PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    # ixwebsocket::ixwebsocket
    pugixml
)

target_include_directories(TradingBot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)