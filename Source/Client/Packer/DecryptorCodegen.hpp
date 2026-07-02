#pragma once

#include "Metamorphic/TransformProgram.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

class DecryptorCodegen {
public:
    [[nodiscard]] static std::string generate_source(
        const metamorphic::TransformProgram& program,
        unsigned int nonce,
        std::string_view function_name);

    [[nodiscard]] static bool compile_to_object(
        const std::filesystem::path& source_path,
        const std::filesystem::path& object_path,
        std::string& error);

    [[nodiscard]] static bool extract_function_bytes(
        const std::filesystem::path& object_path,
        std::string_view function_name,
        std::vector<uint8_t>& machine_code,
        std::string& error);

    [[nodiscard]] static bool generate_decryptor(
        const metamorphic::TransformProgram& program,
        unsigned int nonce,
        std::vector<uint8_t>& machine_code,
        std::string& error);
};
