#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct ElfSectionInfo {
    std::string name;
    uint64_t file_offset{0};
    uint64_t size{0};
    uint64_t vaddr{0};
};

class ElfSelfParser {
public:
    [[nodiscard]] bool load_from_self();

    [[nodiscard]] const std::vector<ElfSectionInfo>& sections() const noexcept {
        return sections_;
    }

    [[nodiscard]] std::optional<ElfSectionInfo> find_by_name(std::string_view name) const;
    [[nodiscard]] std::optional<ElfSectionInfo> find_by_offset(uint64_t offset) const;

    [[nodiscard]] bool read_file_bytes(uint64_t offset, uint64_t size, uint8_t* out) const;

private:
    std::vector<uint8_t> image_;
    std::vector<ElfSectionInfo> sections_;
};
