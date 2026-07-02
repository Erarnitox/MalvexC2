#include "Config.hpp"

#include <Metamorphic/ConfigEnvelope.hpp>

MALVEX_CONFIG_SECTION uint8_t mx_config_padding[metamorphic::kConfigPasswordSize + metamorphic::kConfigEnvelopeSize
                                                 - sizeof(ImplantConfig)] = {};

MALVEX_CONFIG_SECTION ImplantConfig config = {
    .username="vicky",
    .password="victim",
    .default_timeout="5",
    .server_url="https://127.0.0.1:3000",
    .service_name="malvex_implant",
    .service_desc="Malvex C2 Implant (written by Erarnitox)"
};
