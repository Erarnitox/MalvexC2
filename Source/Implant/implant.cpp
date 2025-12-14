#include <cpppwn.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

#include "Installer.hpp"
#include "Config.hpp"

int main(int argc, char* argv[]) {
    bool is_installation = false;

    // parse arguments
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--install") {
            is_installation = true;
            break;
        }
    }

    // Installation
    if (is_installation) {
        if (installSystemService(config.service_name, config.service_desc)) {
            return 0; // everything went fine!
        } else {
            return 1; // installation not successful! Probably missing permissions
        }
    }

    // If we are not in Installation mode

    return 0;
}