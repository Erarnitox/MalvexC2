#pragma once

#define MALVEX_CONFIG_SECTION_NAME ".mx_config"
#define MALVEX_CONFIG_SECTION __attribute__((section(MALVEX_CONFIG_SECTION_NAME)))

// Structure to hold our configuration data
struct ImplantConfig {
    char username[64];
    char password[64];
    char default_timeout[8];
    char server_url[256];
    char service_name[64];
    char service_desc[256];
};