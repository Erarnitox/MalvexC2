#include "Metamorphic/ConfigEnvelope.hpp"
#include "Metamorphic/TransformEngine.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

int g_failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

using metamorphic::TransformEngine;

void test_transform_round_trip() {
    const auto program = TransformEngine::generate();
    std::vector<uint8_t> data(256);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(i * 7 + 13);
    }

    const auto original = data;
    TransformEngine::encrypt(data, program);
    expect(data != original, "encrypt should modify data");
    TransformEngine::decrypt(data, program);
    expect(data == original, "decrypt should restore data");
}

void test_bytecode_round_trip() {
    const auto program = TransformEngine::generate();
    const auto bytecode = TransformEngine::serialize_bytecode(program);

    metamorphic::TransformProgram restored;
    expect(TransformEngine::deserialize_bytecode(bytecode, restored), "bytecode deserialize");
    expect(restored.seed == program.seed, "seed match");
    expect(restored.steps.size() == program.steps.size(), "step count match");
}

void test_config_envelope_size() {
    expect(sizeof(metamorphic::ConfigEnvelope) == metamorphic::kConfigEnvelopeSize, "envelope size stable");
    metamorphic::ConfigEnvelope env{};
    metamorphic::init_envelope_strings(env, ".mx_abc12345", ".mx_def67890");
    expect(std::strcmp(env.config_section_name, ".mx_abc12345") == 0, "cfg name");
    expect(std::strcmp(env.payload_section_name, ".mx_def67890") == 0, "payload name");
}

} // namespace

int main() {
    test_transform_round_trip();
    test_bytecode_round_trip();
    test_config_envelope_size();

    if (g_failures == 0) {
        std::cout << "All metamorphic unit tests passed\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed\n";
    return 1;
}
