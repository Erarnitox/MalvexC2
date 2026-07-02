#include "ConfigDecryptor.hpp"

namespace metamorphic {

bool decrypt_buffer(std::span<uint8_t> data, std::span<const uint8_t> bytecode) {
    TransformProgram program;
    if (!TransformEngine::deserialize_bytecode(bytecode, program)) {
        return false;
    }
    TransformEngine::decrypt(data, program);
    return true;
}

} // namespace metamorphic
