#pragma once

#include <cpppwn.hpp>

void register_beacon_endpoint(cpppwn::RESTServer& server);
void register_all_beacon_endpoints(cpppwn::RESTServer& server);
