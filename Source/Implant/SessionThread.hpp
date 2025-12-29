#pragma once

#include <Remote.hpp>
#include <Shell.hpp>
#include <cstdint>
#include <string>

#include <cpppwn.hpp>

//-------------------------------------------------
//
//-------------------------------------------------
void inline session_thread(const std::string& host, uint16_t port) {
    using namespace cpppwn;

    Remote conn(host, port, true);
    connect_shell(conn);
}
