#pragma once

#include "PackMode.hpp"
#include "TransformProgram.hpp"

#include <cstdint>

namespace metamorphic {

inline constexpr const char* kConfigSectionTemplate = ".mx_config";
inline constexpr const char* kPayloadSectionTemplate = ".mx_text";
inline constexpr const char* kStubSectionName = ".mx_stub";

struct BootParams {
    uint32_t magic{0};
    uint32_t pack_mode{0};
    uint64_t config_file_offset{0};
    uint32_t config_file_size{0};
    uint32_t bytecode_size{0};
    uint32_t decryptor_size{0};
    uint64_t payload_file_offset{0};
    uint32_t payload_file_size{0};
    uint32_t payload_entry_offset{0};
};

// Layout in .mx_stub: BootParams | bytecode | decryptor machine code
[[nodiscard]] inline std::size_t stub_region_size(
    std::size_t bytecode_size,
    std::size_t decryptor_size) {
    return sizeof(BootParams) + bytecode_size + decryptor_size;
}

} // namespace metamorphic
