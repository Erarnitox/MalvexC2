#pragma once
#include "Database.hpp"
#include <VictimDAO.hpp>
#include <VictimRepository.hpp>

#include <memory>
#include <mutex>
#include <unordered_map>

class VictimManager {
public:
    static VictimManager& instance(const std::string& db_path = "c2.db") {
        static VictimManager instance(db_path);
        return instance;
    }

    // High-level operations
    std::vector<VictimDAO> getOnlineVictims() {
        std::lock_guard<std::mutex> lock(mutex_);
        return repo_->list_by_status(1);  // 1 = online
    }

    std::vector<VictimDAO> getAllVictims() {
        std::lock_guard<std::mutex> lock(mutex_);
        invalidate_cache();
        return repo_->list();
    }

    VictimDAO* getVictim(int64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(id);
        if (it != cache_.end()) {
            return &it->second;
        }

        auto opt = repo_->get(id);
        if (opt) {
            cache_[id] = *opt;
            return &cache_[id];
        }

        return nullptr;
    }

    VictimDAO* getVictimByUID(const UUID& uid) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto opt = repo_->get_by_uid(uid);
        if (opt) {
            cache_[opt->victim_id] = *opt;
            return &cache_[opt->victim_id];
        }

        return nullptr;
    }

    bool registerVictim(const std::string& internal_ip, const std::string& external_ip,
                       const std::string& hostname, const std::string& username,
                       const std::string& os) {
        std::lock_guard<std::mutex> lock(mutex_);

        VictimDAO victim;
        victim.internal_ip = internal_ip;
        victim.external_ip = external_ip;
        victim.hostname = hostname;
        victim.username = username;
        victim.operating_system = os;
        victim.last_update = 0;
        victim.status = 1;  // Online

        try {
            auto created = repo_->create(victim);
            cache_[created.victim_id] = created;
            return true;
        } catch (...) {
            return false;
        }
    }

    bool updateVictimStatus(int64_t id, int status) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto opt = repo_->get(id);
        if (!opt) return false;

        opt->status = status;
        opt->last_update = std::chrono::system_clock::now();

        auto updated = repo_->update(id, *opt);
        if (updated) {
            cache_[id] = *updated;
            return true;
        }

        return false;
    }

    bool removeVictim(int64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(id);
        return repo_->remove(id);
    }

    void invalidate_cache() {
        cache_.clear();
    }

    VictimManager(const VictimManager&) = delete;
    VictimManager& operator=(const VictimManager&) = delete;

private:
    explicit VictimManager(const std::string& db_path)
        : db_(std::make_shared<Database>(db_path)),
          repo_(std::make_unique<VictimRepository>(db_)) {}

    std::shared_ptr<Database> db_;
    std::unique_ptr<VictimRepository> repo_;
    std::unordered_map<int64_t, VictimDAO> cache_;
    std::mutex mutex_;
};