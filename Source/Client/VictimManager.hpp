#pragma once

#include <Model.hpp>
#include <mutex>
#include <vector>

class VictimManager {
    mutable std::mutex m_mtx;
    std::vector<Victim> m_victims;

public:
    VictimManager(const VictimManager&) = delete;
    VictimManager& operator=(const VictimManager&) = delete;
    VictimManager(VictimManager&&) = delete;
    VictimManager& operator=(VictimManager&&) = delete;
    explicit VictimManager() = default;

    static VictimManager& instance();

    const std::vector<Victim>& getVictims() const noexcept;
    const Victim getVictim(int64_t id) const noexcept;
    [[nodiscard]] bool removeVictim(const UUID& uid) noexcept;
    [[nodiscard]] bool removeVictim(int64_t id) noexcept;
    void addVictim(const Victim& victim) noexcept;
    void setList(std::vector<Victim>&& victim_list) noexcept;
};
