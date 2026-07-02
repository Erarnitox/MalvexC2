#pragma once

#include <elfio/elfio.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct ElfSectionView {
    std::string name;
    ELFIO::section* section{nullptr};
};

class ElfPacker {
public:
    [[nodiscard]] bool load(const std::string& path);
    [[nodiscard]] bool save(const std::string& path);

    [[nodiscard]] ELFIO::section* find_section(std::string_view name);
    [[nodiscard]] std::optional<uint64_t> find_symbol_offset(std::string_view symbol_name, std::string_view section_name);

    [[nodiscard]] bool write_section_data(ELFIO::section* section, const std::vector<uint8_t>& data);
    [[nodiscard]] bool rename_section(ELFIO::section* section, const std::string& new_name);

    [[nodiscard]] ELFIO::elfio& elf() noexcept { return elf_; }

private:
    ELFIO::elfio elf_;
};
