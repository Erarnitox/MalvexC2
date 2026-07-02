#pragma once

#include "ImplantPacker.hpp"

#include <ImplantConfig.hpp>

#include <filesystem>
#include <string>

class MetamorphicPacker {
public:
    using PackResult = ImplantPacker::PackResult;

    [[nodiscard]] static PackResult pack(
        const std::filesystem::path& template_path,
        const std::filesystem::path& output_path,
        const ImplantConfig& implant_config) {
        return ImplantPacker::pack(
            metamorphic::PackMode::FullMetamorphic,
            template_path,
            output_path,
            implant_config);
    }
};
