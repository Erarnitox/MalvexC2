#include "VictimManager.hpp"

VictimManager& VictimManager::instance() {
    static VictimManager instance;
    return instance;
}

const std::vector<Victim>& VictimManager::getVictims() const noexcept {
    std::lock_guard lock(m_mtx);
    return m_victims;
}

void VictimManager::addVictim(const Victim& victim) noexcept {
    std::lock_guard lock(m_mtx);
    m_victims.push_back(victim);
}

void VictimManager::setList(std::vector<Victim>&& victim_list) noexcept {
    std::lock_guard lock(m_mtx);
    m_victims = std::move(victim_list);
}

const Victim VictimManager::getVictim(int64_t id) const noexcept {
    std::lock_guard lock(m_mtx);
    auto it = std::find_if(m_victims.begin(), m_victims.end(), [id](const Victim& v) {
        return v.id == id;
    });

    if (it != m_victims.end()) {
        return *it;
    }

    return Victim{};
}
