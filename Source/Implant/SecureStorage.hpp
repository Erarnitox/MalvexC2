#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

class SecureStorage {
private:
    static constexpr const char* MAGIC = "MLVX"; // 4-byte Magic Header
    static constexpr size_t MAGIC_LEN = 4;

    // A very simple XOR checksum
    static uint8_t calculate_checksum(const std::string& data) {
        uint8_t checksum = 0;
        for (char c : data) {
            checksum ^= static_cast<uint8_t>(c);
        }
        return checksum;
    }

public:
    static bool save_uuid(const fs::path& path, const std::string& uuid) {
        std::ofstream ofs(path, std::ios::binary);
        if (not ofs) {
            return false;
        }

        ofs.write(MAGIC, MAGIC_LEN);
        ofs.write(uuid.c_str(), uuid.length());

        uint8_t checksum = calculate_checksum(uuid);
        ofs.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
        return ofs.good();
    }

    static std::optional<std::string> load_uuid(const fs::path& path) {
        if (not fs::exists(path)) {
            return std::nullopt;
        }

        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        if (not ifs) {
            return std::nullopt;
        }

        std::streamsize size = ifs.tellg();
        // Check if file is at least Magic (4) + UUID (min 1) + Checksum (1)
        if (size < static_cast<long>(MAGIC_LEN + 2)) {
            return std::nullopt;
        }

        ifs.seekg(0, std::ios::beg);

        char header[MAGIC_LEN];
        ifs.read(header, MAGIC_LEN);
        if (std::string(header, MAGIC_LEN) != MAGIC) {
            return std::nullopt;
        }

        size_t payload_len = static_cast<size_t>(size) - MAGIC_LEN - 1;
        std::string uuid(payload_len, '\0');
        ifs.read(&uuid[0], payload_len);

        uint8_t stored_checksum;
        ifs.read(reinterpret_cast<char*>(&stored_checksum), sizeof(stored_checksum));

        if (calculate_checksum(uuid) != stored_checksum) {
            return std::nullopt;
        }
        return uuid;
    }
};