#pragma once
#include <string>
#include <chrono>
#include <cstdint>

using UUID = std::string;
using TimePoint = size_t; // unix timestamp

// Helper functions for UUID generation
inline UUID generate_uuid() { //TODO: implement
    return "00000000-0000-0000-0000-000000000000";
}