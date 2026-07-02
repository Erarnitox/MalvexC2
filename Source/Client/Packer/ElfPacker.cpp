#include "ElfPacker.hpp"

#include <cstring>

bool ElfPacker::load(const std::string& path) {
    return elf_.load(path);
}

bool ElfPacker::save(const std::string& path) {
    return elf_.save(path);
}

ELFIO::section* ElfPacker::find_section(std::string_view name) {
    for (const auto& sec : elf_.sections) {
        if (sec->get_name() == name) {
            return sec.get();
        }
    }
    return nullptr;
}

std::optional<uint64_t> ElfPacker::find_symbol_offset(
    std::string_view symbol_name,
    std::string_view section_name) {
    ELFIO::section* symtab = nullptr;

    for (const auto& sec : elf_.sections) {
        if (sec->get_type() == ELFIO::SHT_SYMTAB) {
            symtab = sec.get();
        }
    }

    if (!symtab) {
        return std::nullopt;
    }

    ELFIO::symbol_section_accessor symbols(elf_, symtab);
    const auto count = symbols.get_symbols_num();

    ELFIO::section* target = find_section(section_name);
    if (!target) {
        return std::nullopt;
    }

    for (unsigned int i = 0; i < count; ++i) {
        std::string name;
        ELFIO::Elf64_Addr value = 0;
        ELFIO::Elf_Xword size = 0;
        unsigned char bind = 0;
        unsigned char type = 0;
        ELFIO::Elf_Half section_index = 0;
        unsigned char other = 0;

        symbols.get_symbol(i, name, value, size, bind, type, section_index, other);
        if (name != symbol_name) {
            continue;
        }

        if (section_index >= elf_.sections.size()) {
            continue;
        }

        const ELFIO::section* sym_section = elf_.sections[section_index];
        if (sym_section->get_name() != section_name) {
            continue;
        }

        if (value < target->get_address()) {
            return std::nullopt;
        }

        return value - target->get_address();
    }

    return std::nullopt;
}

bool ElfPacker::write_section_data(ELFIO::section* section, const std::vector<uint8_t>& data) {
    if (!section) {
        return false;
    }

    if (section->get_size() < data.size()) {
        return false;
    }

    std::memcpy(const_cast<char*>(section->get_data()), data.data(), data.size());
    return true;
}

bool ElfPacker::rename_section(ELFIO::section* section, const std::string& new_name) {
    if (!section) {
        return false;
    }

    section->set_name(new_name);

    const ELFIO::Elf_Half str_index = elf_.get_section_name_str_index();
    if (str_index == ELFIO::SHN_UNDEF || str_index >= elf_.sections.size()) {
        return false;
    }

    ELFIO::section* string_table = elf_.sections[str_index];
    if (!string_table) {
        return false;
    }

    ELFIO::string_section_accessor str_writer(string_table);
    const ELFIO::Elf_Word pos = str_writer.add_string(new_name);
    section->set_name_string_offset(pos);
    return true;
}
