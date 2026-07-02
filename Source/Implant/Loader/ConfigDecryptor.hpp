#pragma once

#include "Metamorphic/TransformEngine.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace metamorphic {

[[nodiscard]] bool decrypt_buffer(std::span<uint8_t> data, std::span<const uint8_t> bytecode);

using PayloadDecryptFn = void (*)(uint8_t* dst, const uint8_t* src, std::size_t len);

} // namespace metamorphic
