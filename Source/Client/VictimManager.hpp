#pragma once

#include <Model.hpp>

class VictimManager {
    std::vector<Victim> m_victims;
public:
    // Delete copy and move constructors/assignments (singleton pattern)
    VictimManager(const VictimManager&) = delete;
    VictimManager& operator=(const VictimManager&) = delete;
    VictimManager(VictimManager&&) = delete;
    VictimManager& operator=(VictimManager&&) = delete;
    explicit VictimManager() = default;

    static VictimManager& instance();

    const std::vector<Victim>& getVictims() const noexcept;
    void addVictim(const Victim& victim) noexcept;
    void setList(std::vector<Victim>&& victim_list) noexcept;
};