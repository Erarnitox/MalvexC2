#pragma once

#include "ImplantConfig.hpp"
#include "TransformProgram.hpp"

#include <cstring>

namespace metamorphic {

struct ConfigEnvelope {
    char config_section_name[kSectionNameMax]{};
    char payload_section_name[kSectionNameMax]{};
    ImplantConfig implant{};
};

inline constexpr std::size_t kConfigEnvelopeSize = sizeof(ConfigEnvelope);

inline void init_envelope_strings(ConfigEnvelope& env, const char* cfg_name, const char* payload_name) {
    std::strncpy(env.config_section_name, cfg_name, kSectionNameMax - 1);
    std::strncpy(env.payload_section_name, payload_name, kSectionNameMax - 1);
}

} // namespace metamorphic
