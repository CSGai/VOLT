#pragma once
#include <cstdint>

enum class Destination : uint8_t {
    INFERENCE = 0x01,
    TRAINING  = 0x02
};

// Every frame on the wire:
// [1 byte : destination]
// [4 bytes: payload length, big-endian uint32]
// [N bytes: payload (raw float array, msgpack, protobuf, etc.)]

struct FrameHeader {
    Destination dest;
    uint32_t    payload_len;   // always write/read as big-endian
} __attribute__((packed));