#pragma once

#include "TransformProgram.hpp"
#include "Types.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace metamorphic {

namespace detail {

[[nodiscard]] inline uint8_t apply_forward(uint8_t value, const TransformStep& step) {
    switch (step.op) {
    case TransformOp::Xor:
        return static_cast<uint8_t>(value ^ step.key_byte);
    case TransformOp::Add:
        return static_cast<uint8_t>(value + step.key_byte);
    case TransformOp::Sub:
        return static_cast<uint8_t>(value - step.key_byte);
    case TransformOp::Rol: {
        const auto shift = static_cast<unsigned>(step.key_byte & 7);
        return static_cast<uint8_t>((value << shift) | (value >> (8 - shift)));
    }
    case TransformOp::Ror: {
        const auto shift = static_cast<unsigned>(step.key_byte & 7);
        return static_cast<uint8_t>((value >> shift) | (value << (8 - shift)));
    }
    }
    return value;
}

[[nodiscard]] inline uint8_t apply_reverse(uint8_t value, const TransformStep& step) {
    switch (step.op) {
    case TransformOp::Xor:
        return static_cast<uint8_t>(value ^ step.key_byte);
    case TransformOp::Add:
        return static_cast<uint8_t>(value - step.key_byte);
    case TransformOp::Sub:
        return static_cast<uint8_t>(value + step.key_byte);
    case TransformOp::Rol: {
        const auto shift = static_cast<unsigned>(step.key_byte & 7);
        return static_cast<uint8_t>((value >> shift) | (value << (8 - shift)));
    }
    case TransformOp::Ror: {
        const auto shift = static_cast<unsigned>(step.key_byte & 7);
        return static_cast<uint8_t>((value << shift) | (value >> (8 - shift)));
    }
    }
    return value;
}

} // namespace detail

class TransformEngine {
public:
    [[nodiscard]] static TransformProgram generate() {
        TransformProgram program;
        program.seed = static_cast<uint32_t>(generate_nonce());

        const auto step_count = kMinTransformSteps + (generate_nonce() % (kMaxTransformSteps - kMinTransformSteps + 1));
        program.steps.reserve(step_count);

        for (std::size_t i = 0; i < step_count; ++i) {
            TransformStep step;
            step.op = static_cast<TransformOp>(generate_nonce() % 5);
            step.key = static_cast<uint32_t>(generate_nonce());
            step.key_byte = static_cast<uint8_t>(step.key & 0xFF);
            if (step.key_byte == 0) {
                step.key_byte = static_cast<uint8_t>((step.key >> 8) & 0xFF);
            }
            if (step.key_byte == 0) {
                step.key_byte = 1;
            }
            program.steps.push_back(step);
        }

        return program;
    }

    static void encrypt(std::span<uint8_t> data, const TransformProgram& program) {
        for (std::size_t i = 0; i < data.size(); ++i) {
            uint8_t value = data[i];
            for (const auto& step : program.steps) {
                value = detail::apply_forward(value, step);
            }
            value ^= static_cast<uint8_t>((program.seed >> ((i % 4) * 8)) & 0xFF);
            data[i] = value;
        }
    }

    static void decrypt(std::span<uint8_t> data, const TransformProgram& program) {
        for (std::size_t i = 0; i < data.size(); ++i) {
            uint8_t value = data[i];
            value ^= static_cast<uint8_t>((program.seed >> ((i % 4) * 8)) & 0xFF);
            for (auto it = program.steps.rbegin(); it != program.steps.rend(); ++it) {
                value = detail::apply_reverse(value, *it);
            }
            data[i] = value;
        }
    }

    [[nodiscard]] static std::vector<uint8_t> serialize_bytecode(const TransformProgram& program) {
        std::vector<uint8_t> out;
        out.reserve(serialized_bytecode_size(program));

        auto append_u32 = [&](uint32_t v) {
            out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
            out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
            out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(v & 0xFF));
        };

        append_u32(program.seed);
        append_u32(static_cast<uint32_t>(program.steps.size()));

        for (const auto& step : program.steps) {
            out.push_back(static_cast<uint8_t>(step.op));
            append_u32(step.key);
            out.push_back(step.key_byte);
        }

        return out;
    }

    [[nodiscard]] static bool deserialize_bytecode(std::span<const uint8_t> data, TransformProgram& program) {
        if (data.size() < 8) {
            return false;
        }

        auto read_u32 = [&](std::size_t offset) -> uint32_t {
            return (static_cast<uint32_t>(data[offset]) << 24) |
                   (static_cast<uint32_t>(data[offset + 1]) << 16) |
                   (static_cast<uint32_t>(data[offset + 2]) << 8) |
                   static_cast<uint32_t>(data[offset + 3]);
        };

        program.seed = read_u32(0);
        const auto step_count = read_u32(4);
        if (step_count > kMaxTransformSteps || data.size() < 8 + step_count * 6) {
            return false;
        }

        program.steps.clear();
        program.steps.reserve(step_count);

        std::size_t offset = 8;
        for (uint32_t i = 0; i < step_count; ++i) {
            TransformStep step;
            step.op = static_cast<TransformOp>(data[offset++]);
            step.key = read_u32(offset);
            offset += 4;
            step.key_byte = data[offset++];
            program.steps.push_back(step);
        }

        return true;
    }
};

} // namespace metamorphic
