#pragma once
#include <random>
#include <string>
#include <chrono>
#include <cstdint>

using UUID = std::string;
using TimePoint = size_t; // unix timestamp

//--------------------------------
//
//--------------------------------
inline UUID generate_uuid() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint32_t> dis_32;
    std::uniform_int_distribution<uint16_t> dis_16;
    std::uniform_int_distribution<uint8_t> dis_8;

    uint32_t time_low = dis_32(gen);
    uint16_t time_mid = dis_16(gen);
    uint16_t time_hi_version = (dis_16(gen) & 0x0FFF) | 0x4000; // Version 4
    uint8_t clock_seq_hi = (dis_8(gen) & 0x3F) | 0x80; // Variant 10
    uint8_t clock_seq_low = dis_8(gen);

    uint64_t node = 0;
    for (int i = 0; i < 6; i++) {
        node = (node << 8) | dis_8(gen);
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << std::setw(8) << time_low << '-';
    oss << std::setw(4) << time_mid << '-';
    oss << std::setw(4) << time_hi_version << '-';
    oss << std::setw(2) << static_cast<int>(clock_seq_hi);
    oss << std::setw(2) << static_cast<int>(clock_seq_low) << '-';
    oss << std::setw(12) << node;

    return oss.str();
}