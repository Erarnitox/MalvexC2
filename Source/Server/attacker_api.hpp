#pragma once

#include <RESTServer.hpp>
#include <cpppwn.hpp>

#include <cstdint>

void start_attacker_api(int16_t port) {
    using namespace cpppwn;

    RESTServer attacker_api(port);

    //TODO: implement endpoints

    attacker_api.start();
}