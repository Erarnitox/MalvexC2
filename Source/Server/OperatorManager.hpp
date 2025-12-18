#pragma once
#include "OperatorDAO.hpp"
#include "OperatorRepository.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>

//-------------------------------------------------
//
//-------------------------------------------------
class OperatorManager {
public:
    //-------------------------------------------------
    //
    //-------------------------------------------------
    static OperatorManager& instance(const std::string& db_path = "server.db") {
        static OperatorManager instance(db_path);
        return instance;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::vector<OperatorDAO> getOperators() {
        std::lock_guard<std::mutex> lock(mutex_);
        invalidate_cache();
        return repo_->list();
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    OperatorDAO* getOperator(int64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        // cache lookup
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

    //-------------------------------------------------
    //
    //-------------------------------------------------
    OperatorDAO* getOperator(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);

        // cache lookup
        auto it = std::find_if(cache_.begin(), cache_.end(), [username](const auto& it){
            return it.second.username == username;
        });

        if (it != cache_.end()) {
            return &it->second;
        }

        auto opt = repo_->get(username);
        if (opt) {
            cache_[opt->operator_id] = *opt;
            return &cache_[opt->operator_id];
        }

        return nullptr;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    bool removeOperator(int64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(id);
        return repo_->remove(id);
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    void invalidate_cache() {
        cache_.clear();
    }

    OperatorManager(const OperatorManager&) = delete;
    OperatorManager& operator=(const OperatorManager&) = delete;

private:
    explicit OperatorManager(const std::string& db_path)
        : db_(std::make_shared<Database>(db_path)),
          repo_(std::make_unique<OperatorRepository>(db_path)) {}

    std::shared_ptr<Database> db_;
    std::unique_ptr<OperatorRepository> repo_;
    std::unordered_map<int64_t, OperatorDAO> cache_;
    std::mutex mutex_;
};