#pragma once

class VictimManager {
public:
    // Delete copy and move constructors/assignments (singleton pattern)
    VictimManager(const VictimManager&) = delete;
    VictimManager& operator=(const VictimManager&) = delete;
    VictimManager(VictimManager&&) = delete;
    VictimManager& operator=(VictimManager&&) = delete;
    explicit VictimManager() = default;

    static VictimManager& instance();

};