#pragma once

#include <ImplantConfig.hpp>
#include <Metamorphic/PackMode.hpp>

#include <filesystem>
#include <string>

class ImplantPacker {
public:
    struct PackResult {
        bool success{false};
        std::string output_path;
        std::string error;
    };

    [[nodiscard]] static PackResult pack(
        metamorphic::PackMode mode,
        const std::filesystem::path& template_path,
        const std::filesystem::path& output_path,
        const ImplantConfig& implant_config);
};
