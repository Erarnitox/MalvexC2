#pragma once

#include <cstdint>

namespace metamorphic {

enum class PackMode : uint32_t {
    None = 0,
    ConfigOnly = 1,
    BytecodePolymorphic = 2,
    FullMetamorphic = 3,
};

inline constexpr const char* kPackModeComboLabels =
    "None (plaintext config);Config only (encrypted config);Bytecode (polymorphic config);Full metamorphic (clang)";

inline constexpr int kPackModeCount = 4;

[[nodiscard]] inline PackMode pack_mode_from_index(int index) noexcept {
    if (index < 0 || index >= kPackModeCount) {
        return PackMode::FullMetamorphic;
    }
    return static_cast<PackMode>(index);
}

[[nodiscard]] inline int pack_mode_to_index(PackMode mode) noexcept {
    return static_cast<int>(mode);
}

[[nodiscard]] inline const char* pack_mode_name(PackMode mode) noexcept {
    switch (mode) {
    case PackMode::None:
        return "None";
    case PackMode::ConfigOnly:
        return "Config only";
    case PackMode::BytecodePolymorphic:
        return "Bytecode";
    case PackMode::FullMetamorphic:
        return "Full metamorphic";
    }
    return "Unknown";
}

} // namespace metamorphic
