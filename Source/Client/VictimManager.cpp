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

bool VictimManager::removeVictim(const UUID& uid) noexcept {
    if (uid.empty()) {
        return false;
    }

    std::lock_guard lock(m_mtx);
    const auto it = std::find_if(m_victims.begin(), m_victims.end(), [&uid](const Victim& victim) {
        return victim.uid == uid;
    });

    if (it == m_victims.end()) {
        return false;
    }

    m_victims.erase(it);
    return true;
}

bool VictimManager::removeVictim(int64_t id) noexcept {
    if (id <= 0) {
        return false;
    }

    std::lock_guard lock(m_mtx);
    const auto it = std::find_if(m_victims.begin(), m_victims.end(), [id](const Victim& victim) {
        return victim.id == id;
    });

    if (it == m_victims.end()) {
        return false;
    }

    m_victims.erase(it);
    return true;
}
