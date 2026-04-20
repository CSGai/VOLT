cmake_minimum_required(VERSION 3.14)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("TradingBot")

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ... rest of file
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)

set(CPR_USE_SYSTEM_CURL OFF)
set(CPR_FORCE_USE_SYSTEM_CURL OFF)

set(CURL_STATICLIB ON)
set(BUILD_CURL_EXE OFF)
set(CURL_ZLIB OFF)

# Fetch CPR as static library
include(FetchContent)


FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)


# Add executable
add_executable(TradingBot 
    src/TradingBot.cpp
    src/TradingBot.h

    # market
    src/APIs/market/market.h
    src/APIs/market/YahooFinance.cpp
    src/APIs/market/Analysis.cpp

    # utils
    src/utils/utils.h
    src/utils/unix_timestamp_converter.cpp
)

target_link_libraries(TradingBot PRIVATE 
    cpr::cpr 
    nlohmann_json::nlohmann_json
)

target_include_directories(TradingBot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)