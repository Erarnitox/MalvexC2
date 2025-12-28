#pragma once

#include "Beacon.hpp"
#include "CommandDAO.hpp"
#include <RESTClient.hpp>
#include <vector>

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] std::vector<CommandDAO> sendBeacon(cpppwn::RESTClient& rest_client) {

    // send command to commands endpoint
    try{
        BeaconRequest beacon_dao;

        const auto& req = rest_client.post<BeaconRequest>("api/beacon", beacon_dao);

        return {};
    } catch(const std::runtime_error& err) {
        return {};
    }
}