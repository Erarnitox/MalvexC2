#include "Packer/ImplantPacker.hpp"

#include <ImplantConfig.hpp>

#include <elfio/elfio.hpp>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

namespace {

int fail(const std::string& message) {
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

std::set<std::string> section_names(const std::filesystem::path& path) {
    std::set<std::string> names;
    ELFIO::elfio reader;
    if (!reader.load(path.string())) {
        return names;
    }

    for (const auto& sec : reader.sections) {
        names.insert(sec->get_name());
    }
    return names;
}

std::string file_hash(const std::filesystem::path& path) {
    const auto cmd = std::format("sha256sum \"{}\" | awk '{{print $1}}'", path.string());
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {};
    }

    char buffer[128];
    std::string hash;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        hash = buffer;
        if (!hash.empty() && hash.back() == '\n') {
            hash.pop_back();
        }
    }
    pclose(pipe);
    return hash;
}

bool contains_plaintext_url(const std::filesystem::path& path) {
    const auto cmd = std::format("strings \"{}\"", path.string());
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return false;
    }

    char buffer[512];
    bool found = false;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        const std::string line(buffer);
        if (line.find("https://127.0.0.1") != std::string::npos) {
            found = true;
            break;
        }
    }
    pclose(pipe);
    return found;
}

} // namespace

int main(int argc, char** argv) {
    const auto template_path = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path("build/bin/Client/implant");

    if (!std::filesystem::exists(template_path)) {
        return fail("implant template not found: " + template_path.string());
    }

    const auto output_dir = std::filesystem::temp_directory_path() / "malvex_metamorphic_smoke";
    std::error_code ec;
    std::filesystem::create_directories(output_dir, ec);

    const auto out_a = output_dir / "implant_a";
    const auto out_b = output_dir / "implant_b";

    ImplantConfig cfg_a{};
    std::strncpy(cfg_a.username, "user_a", sizeof(cfg_a.username) - 1);
    std::strncpy(cfg_a.password, "pass_a", sizeof(cfg_a.password) - 1);
    std::strncpy(cfg_a.server_url, "https://10.0.0.1:3000", sizeof(cfg_a.server_url) - 1);

    ImplantConfig cfg_b = cfg_a;
    std::strncpy(cfg_b.username, "user_b", sizeof(cfg_b.username) - 1);
    std::strncpy(cfg_b.password, "pass_b", sizeof(cfg_b.password) - 1);
    std::strncpy(cfg_b.server_url, "https://10.0.0.2:3000", sizeof(cfg_b.server_url) - 1);

    const auto pack_a = ImplantPacker::pack(
        metamorphic::PackMode::FullMetamorphic,
        template_path,
        out_a,
        cfg_a);
    if (!pack_a.success) {
        return fail("pack A failed: " + pack_a.error);
    }

    const auto pack_b = ImplantPacker::pack(
        metamorphic::PackMode::FullMetamorphic,
        template_path,
        out_b,
        cfg_b);
    if (!pack_b.success) {
        return fail("pack B failed: " + pack_b.error);
    }

    const auto sections_a = section_names(out_a);
    const auto sections_b = section_names(out_b);

    if (sections_a.contains(".mx_config") || sections_b.contains(".mx_config")) {
        return fail("packed binary still contains .mx_config");
    }

    if (sections_a.contains(".mx_text") || sections_b.contains(".mx_text")) {
        return fail("packed binary still contains .mx_text");
    }

    auto has_random_mx = [](const std::set<std::string>& names) {
        for (const auto& name : names) {
            if (name.starts_with(".mx_") && name != ".mx_stub") {
                return true;
            }
        }
        return false;
    };

    if (!has_random_mx(sections_a) || !has_random_mx(sections_b)) {
        return fail("missing randomized .mx_* sections");
    }

    const auto hash_a = file_hash(out_a);
    const auto hash_b = file_hash(out_b);
    if (hash_a.empty() || hash_b.empty() || hash_a == hash_b) {
        return fail("packed builds must differ (sha256)");
    }

    if (contains_plaintext_url(out_a) || contains_plaintext_url(out_b)) {
        return fail("packed binary contains plaintext server URL");
    }

    std::cout << "metamorphic pack smoke passed\n";
    std::cout << "  A: " << out_a << '\n';
    std::cout << "  B: " << out_b << '\n';
    return 0;
}
