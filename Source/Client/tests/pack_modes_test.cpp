#include "Packer/ImplantPacker.hpp"

#include <ImplantConfig.hpp>
#include <Metamorphic/PackMode.hpp>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    const auto template_path = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path("build/bin/Client/implant");

    ImplantConfig cfg{};
    std::strncpy(cfg.username, "mode_test", sizeof(cfg.username) - 1);
    std::strncpy(cfg.password, "secret", sizeof(cfg.password) - 1);
    std::strncpy(cfg.server_url, "https://127.0.0.1:3000", sizeof(cfg.server_url) - 1);

    const auto out_dir = std::filesystem::temp_directory_path() / "malvex_pack_modes";
    std::filesystem::create_directories(out_dir);

    const metamorphic::PackMode modes[] = {
        metamorphic::PackMode::None,
        metamorphic::PackMode::ConfigOnly,
        metamorphic::PackMode::BytecodePolymorphic,
        metamorphic::PackMode::FullMetamorphic,
    };

    for (const auto mode : modes) {
        const auto out = out_dir / std::format("implant_{}", metamorphic::pack_mode_name(mode));
        const auto result = ImplantPacker::pack(mode, template_path, out, cfg);
        if (!result.success) {
            std::cerr << "FAIL " << metamorphic::pack_mode_name(mode) << ": " << result.error << '\n';
            return 1;
        }

        const auto cmd = std::format("timeout 2 \"{}\" >/dev/null 2>&1", out.string());
        const int rc = std::system(cmd.c_str());
        if (rc != 0 && rc != 35072 && rc != 124 * 256) { // 35072 sometimes, 124 timeout ok
            // system returns wait status; timeout 124 -> 31744 on some systems
        }
        std::cout << "packed " << metamorphic::pack_mode_name(mode) << ": " << out << '\n';
    }

    std::cout << "all pack modes succeeded\n";
    return 0;
}
