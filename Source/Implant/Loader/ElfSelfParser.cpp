#include "ElfSelfParser.hpp"

#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <fstream>

bool ElfSelfParser::load_from_self() {
    std::ifstream self("/proc/self/exe", std::ios::binary);
    if (!self) {
        return false;
    }

    image_.assign(std::istreambuf_iterator<char>(self), std::istreambuf_iterator<char>());
    if (image_.size() < sizeof(Elf64_Ehdr)) {
        return false;
    }

    const auto* ehdr = reinterpret_cast<const Elf64_Ehdr*>(image_.data());
    if (std::memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 || ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        return false;
    }

    if (ehdr->e_shoff == 0 || ehdr->e_shentsize != sizeof(Elf64_Shdr)) {
        return false;
    }

    const auto* shdrs = reinterpret_cast<const Elf64_Shdr*>(image_.data() + ehdr->e_shoff);
    if (ehdr->e_shstrndx >= ehdr->e_shnum) {
        return false;
    }

    const auto* shstr = reinterpret_cast<const char*>(image_.data() + shdrs[ehdr->e_shstrndx].sh_offset);

    sections_.clear();
    for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
        const auto& sh = shdrs[i];
        if (sh.sh_type == SHT_NULL) {
            continue;
        }

        ElfSectionInfo info;
        info.name = shstr + sh.sh_name;
        info.file_offset = sh.sh_offset;
        info.size = sh.sh_size;
        info.vaddr = sh.sh_addr;
        sections_.push_back(std::move(info));
    }

    return true;
}

std::optional<ElfSectionInfo> ElfSelfParser::find_by_name(std::string_view name) const {
    for (const auto& section : sections_) {
        if (section.name == name) {
            return section;
        }
    }
    return std::nullopt;
}

std::optional<ElfSectionInfo> ElfSelfParser::find_by_offset(uint64_t offset) const {
    for (const auto& section : sections_) {
        if (section.file_offset == offset) {
            return section;
        }
    }
    return std::nullopt;
}

bool ElfSelfParser::read_file_bytes(uint64_t offset, uint64_t size, uint8_t* out) const {
    if (offset + size > image_.size()) {
        return false;
    }
    std::memcpy(out, image_.data() + offset, size);
    return true;
}
