#pragma once

#include "ExfilEnvelope.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace loot_archive {

struct LootFileEntry {
    std::string path;
    std::vector<unsigned char> data;
};

[[nodiscard]] inline std::string encode_base64(const std::vector<unsigned char>& data) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;

    int val = 0;
    int valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (out.size() % 4) {
        out.push_back('=');
    }

    return out;
}

[[nodiscard]] inline std::optional<std::vector<unsigned char>> decode_base64(std::string_view encoded) {
    static const int decode_table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    };

    std::vector<unsigned char> out;
    int val = 0;
    int valb = -8;

    for (unsigned char c : encoded) {
        if (c == '=') {
            break;
        }
        const int d = decode_table[c];
        if (d == -1) {
            continue;
        }
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return out;
}

[[nodiscard]] inline std::vector<unsigned char> encode_binary(const std::vector<LootFileEntry>& files) {
    std::vector<unsigned char> archive_buffer;

    auto append_uint32 = [&](uint32_t val) {
        archive_buffer.push_back((val >> 24) & 0xFF);
        archive_buffer.push_back((val >> 16) & 0xFF);
        archive_buffer.push_back((val >> 8) & 0xFF);
        archive_buffer.push_back(val & 0xFF);
    };

    for (const auto& entry : files) {
        const auto path_len = static_cast<uint32_t>(entry.path.size());
        const auto data_len = static_cast<uint32_t>(entry.data.size());

        append_uint32(path_len);
        archive_buffer.insert(archive_buffer.end(), entry.path.begin(), entry.path.end());
        append_uint32(data_len);
        archive_buffer.insert(archive_buffer.end(), entry.data.begin(), entry.data.end());
    }

    return archive_buffer;
}

[[nodiscard]] inline std::optional<std::vector<LootFileEntry>> decode_binary(
    const std::vector<unsigned char>& archive_buffer) {
    std::vector<LootFileEntry> files;
    std::size_t offset = 0;

    auto read_uint32 = [&](uint32_t& out) -> bool {
        if (offset + 4 > archive_buffer.size()) {
            return false;
        }
        out = (static_cast<uint32_t>(archive_buffer[offset]) << 24) |
              (static_cast<uint32_t>(archive_buffer[offset + 1]) << 16) |
              (static_cast<uint32_t>(archive_buffer[offset + 2]) << 8) |
              static_cast<uint32_t>(archive_buffer[offset + 3]);
        offset += 4;
        return true;
    };

    while (offset < archive_buffer.size()) {
        uint32_t path_len = 0;
        if (!read_uint32(path_len)) {
            break;
        }
        if (offset + path_len > archive_buffer.size()) {
            return std::nullopt;
        }

        LootFileEntry entry;
        entry.path.assign(
            reinterpret_cast<const char*>(archive_buffer.data() + offset),
            path_len);
        offset += path_len;

        uint32_t data_len = 0;
        if (!read_uint32(data_len)) {
            return std::nullopt;
        }
        if (offset + data_len > archive_buffer.size()) {
            return std::nullopt;
        }

        entry.data.assign(
            archive_buffer.begin() + static_cast<std::ptrdiff_t>(offset),
            archive_buffer.begin() + static_cast<std::ptrdiff_t>(offset + data_len));
        offset += data_len;

        files.push_back(std::move(entry));
    }

    return files;
}

[[nodiscard]] inline std::string encode_base64_archive(const std::vector<LootFileEntry>& files) {
    return encode_base64(encode_binary(files));
}

[[nodiscard]] inline std::optional<std::vector<LootFileEntry>> decode_base64_archive(
    std::string_view encoded) {
    const auto binary = decode_base64(encoded);
    if (!binary) {
        return std::nullopt;
    }
    return decode_binary(*binary);
}

} // namespace loot_archive
