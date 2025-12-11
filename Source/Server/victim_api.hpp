#pragma once

#include <cpppwn.hpp>

#include <cstdint>

void start_victim_api(int16_t port) {
    using namespace cpppwn;

    RESTServer attacker_api(port);

    //TODO: implement Endpoints

    attacker_api.start();
}