#include "DecryptorCodegen.hpp"

#include <elfio/elfio.hpp>

#include <cstdlib>
#include <format>
#include <fstream>
#include <random>
#include <sstream>

namespace {

std::string random_ident(unsigned int nonce, const char* prefix) {
    return std::format("{}_{:x}", prefix, nonce);
}

std::string reverse_op(metamorphic::TransformOp op) {
    switch (op) {
    case metamorphic::TransformOp::Xor: return "^";
    case metamorphic::TransformOp::Add: return "-";
    case metamorphic::TransformOp::Sub: return "+";
    case metamorphic::TransformOp::Rol: return "ror";
    case metamorphic::TransformOp::Ror: return "rol";
    }
    return "^";
}

} // namespace

std::string DecryptorCodegen::generate_source(
    const metamorphic::TransformProgram& program,
    unsigned int nonce,
    std::string_view function_name) {
    std::ostringstream out;
    const auto fn = function_name;
    const auto var_i = random_ident(nonce, "i");
    const auto var_v = random_ident(nonce, "v");

    out << "#include <stdint.h>\n#include <stddef.h>\n\n";
    out << "__attribute__((noinline)) void " << fn
        << "(uint8_t* dst, const uint8_t* src, size_t len) {\n";

    if (nonce % 2 == 0) {
        out << "    volatile size_t " << random_ident(nonce, "junk") << " = len ^ 0x"
            << std::hex << program.seed << std::dec << ";\n";
    }

    out << "    for (size_t " << var_i << " = 0; " << var_i << " < len; ++" << var_i << ") {\n";
    out << "        uint8_t " << var_v << " = src[" << var_i << "];\n";
    out << "        " << var_v << " ^= (uint8_t)((" << program.seed << "U >> ((" << var_i
        << " % 4) * 8)) & 0xFF);\n";

    for (auto it = program.steps.rbegin(); it != program.steps.rend(); ++it) {
        const auto shift = static_cast<unsigned>(it->key_byte & 7);
        if (it->op == metamorphic::TransformOp::Rol) {
            out << "        " << var_v << " = (uint8_t)((" << var_v << " >> " << shift
                << ") | (" << var_v << " << " << (8 - shift) << "));\n";
        } else if (it->op == metamorphic::TransformOp::Ror) {
            out << "        " << var_v << " = (uint8_t)((" << var_v << " << " << shift
                << ") | (" << var_v << " >> " << (8 - shift) << "));\n";
        } else {
            const auto op = reverse_op(it->op);
            out << "        " << var_v << " = (uint8_t)(" << var_v << " " << op << " "
                << static_cast<unsigned>(it->key_byte) << ");\n";
        }
    }

    out << "        dst[" << var_i << "] = " << var_v << ";\n";
    out << "    }\n}\n";
    return out.str();
}

bool DecryptorCodegen::compile_to_object(
    const std::filesystem::path& source_path,
    const std::filesystem::path& object_path,
    std::string& error) {
    const auto cmd = std::format(
        "clang -c -O2 -fPIC -fno-stack-protector -o \"{}\" \"{}\" 2>&1",
        object_path.string(),
        source_path.string());

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        error = "failed to launch clang";
        return false;
    }

    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    const int rc = pclose(pipe);
    if (rc != 0) {
        error = output.empty() ? "clang compilation failed" : output;
        return false;
    }

    return true;
}

bool DecryptorCodegen::extract_function_bytes(
    const std::filesystem::path& object_path,
    std::string_view function_name,
    std::vector<uint8_t>& machine_code,
    std::string& error) {
    ELFIO::elfio reader;
    if (!reader.load(object_path.string())) {
        error = "failed to load object file";
        return false;
    }

    ELFIO::section* symtab = nullptr;
    for (const auto& sec : reader.sections) {
        if (sec->get_type() == ELFIO::SHT_SYMTAB) {
            symtab = sec.get();
            break;
        }
    }

    if (!symtab) {
        error = "no symbol table in object file";
        return false;
    }

    ELFIO::section* text = nullptr;
    for (const auto& sec : reader.sections) {
        if (sec->get_name() == ".text") {
            text = sec.get();
            break;
        }
    }

    if (!text || text->get_size() == 0) {
        error = "no .text in object file";
        return false;
    }

    ELFIO::symbol_section_accessor symbols(reader, symtab);
    const auto count = symbols.get_symbols_num();

    for (unsigned int i = 0; i < count; ++i) {
        std::string name;
        ELFIO::Elf64_Addr value = 0;
        ELFIO::Elf_Xword size = 0;
        unsigned char bind = 0;
        unsigned char type = 0;
        ELFIO::Elf_Half section_index = 0;
        unsigned char other = 0;

        symbols.get_symbol(i, name, value, size, bind, type, section_index, other);
        if (name != function_name || type != ELFIO::STT_FUNC || size == 0) {
            continue;
        }

        if (section_index >= reader.sections.size()) {
            continue;
        }

        const ELFIO::section* sym_section = reader.sections[section_index];
        if (sym_section->get_name() != ".text") {
            continue;
        }

        if (value + size > text->get_size()) {
            error = "decryptor function exceeds .text bounds";
            return false;
        }

        machine_code.assign(
            reinterpret_cast<const uint8_t*>(text->get_data()) + value,
            reinterpret_cast<const uint8_t*>(text->get_data()) + value + size);
        return true;
    }

    error = std::format("decryptor symbol not found: {}", function_name);
    return false;
}

bool DecryptorCodegen::generate_decryptor(
    const metamorphic::TransformProgram& program,
    unsigned int nonce,
    std::vector<uint8_t>& machine_code,
    std::string& error) {
    const auto function_name = random_ident(nonce, "mx_decrypt");
    const auto tmp_dir = std::filesystem::temp_directory_path();
    const auto source_path = tmp_dir / std::format("mx_decrypt_{}.c", nonce);
    const auto object_path = tmp_dir / std::format("mx_decrypt_{}.o", nonce);

    {
        std::ofstream source(source_path);
        if (!source) {
            error = "failed to write generated source";
            return false;
        }
        source << generate_source(program, nonce, function_name);
    }

    if (!compile_to_object(source_path, object_path, error)) {
        std::error_code ec;
        std::filesystem::remove(source_path, ec);
        return false;
    }

    const bool ok = extract_function_bytes(object_path, function_name, machine_code, error);

    std::error_code ec;
    std::filesystem::remove(source_path, ec);
    std::filesystem::remove(object_path, ec);

    return ok;
}
