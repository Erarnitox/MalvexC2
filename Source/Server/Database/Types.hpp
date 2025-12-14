#pragma once
#include <string>
#include <chrono>
#include <cstdint>

using UUID = std::string;
using Timestamp = std::chrono::system_clock::time_point;

// Helper functions for UUID generation
inline UUID generate_uuid() { //TODO: implement
    return "00000000-0000-0000-0000-000000000000";
}