#pragma once

#include <cstdint>
#include <vector>

namespace metamorphic {

inline constexpr uint32_t kBootMagic = 0x58564C4D; // 'MLVX' little-endian
inline constexpr std::size_t kConfigPasswordSize = 32;
inline constexpr std::size_t kSectionNameMax = 32;
inline constexpr std::size_t kMinTransformSteps = 4;
inline constexpr std::size_t kMaxTransformSteps = 16;

enum class TransformOp : uint8_t {
    Xor = 0,
    Add = 1,
    Sub = 2,
    Rol = 3,
    Ror = 4,
};

struct TransformStep {
    TransformOp op{TransformOp::Xor};
    uint32_t key{0};
    uint8_t key_byte{0};
};

struct TransformProgram {
    uint32_t seed{0};
    std::vector<TransformStep> steps;
};

[[nodiscard]] inline std::size_t serialized_bytecode_size(const TransformProgram& program) {
    return sizeof(uint32_t) * 2 + program.steps.size() * (sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t));
}

} // namespace metamorphic
