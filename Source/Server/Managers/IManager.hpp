#pragma once

#include "Types.hpp"
#include <IDao.hpp>
#include <IRepository.hpp>

// DAOs
#include <CommandDAO.hpp>
#include <LogDAO.hpp>
#include <OperatorDAO.hpp>
#include <ResultDAO.hpp>
#include <SessionDAO.hpp>
#include <VictimDAO.hpp>
#include <VictimTemplateDAO.hpp>

// Repos
#include <CommandRepository.hpp>
#include <LogRepository.hpp>
#include <OperatorRepository.hpp>
#include <ResultRepository.hpp>
#include <SessionRepository.hpp>
#include <VictimRepository.hpp>
#include <VictimTemplateRepository.hpp>

// STL includes
#include <memory>
#include <mutex>
#include <unordered_map>

//-------------------------------------------------
//
//-------------------------------------------------
template <typename DAO, typename REPO>
class Manager {
public:
    //-------------------------------------------------
    //
    //-------------------------------------------------
    static Manager& instance(const std::string& db_path = "server.db") {
        static Manager instance(db_path);
        return instance;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::vector<DAO> get_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        invalidate_cache();
        return repo_->list();
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::vector<DAO> find(std::function<bool(const DAO&)> predicate) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<DAO> results;

        auto all = repo_->list();
        for (const auto& item : all) {
            if (predicate(item)) {
                results.push_back(item);
            }
        }
        return results;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::optional<DAO> get(int64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (auto it = cache_.find(id); it != cache_.end()) {
            return it->second;
        }

        if (auto opt = repo_->get(id)) {
            cache_[id] = *opt;
            return *opt;
        }

        return std::nullopt;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::optional<DAO> get_by_uid(UUID uid) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (auto opt = repo_->get(uid)) {
            cache_[opt.value().id] = opt.value();
            return opt.value();
        }

        return std::nullopt;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::optional<DAO> update(int64_t id, DAO dao) {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto updated = repo_->update(id, dao).value();
        if (auto it = cache_.find(id); it != cache_.end()) {
            cache_[id] = updated;
        }

        return updated;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    bool remove(int64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(id);
        return repo_->remove(id);
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    DAO create(const DAO& obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto& res = repo_->create(obj);
        cache_[obj.id] = obj;
        return res;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    const REPO* get_repo() {
        return repo_.get();
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    void invalidate_cache() {
        cache_.clear();
    }

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

private:
    explicit Manager(const std::string& db_path)
        : db_(std::make_shared<Database>(db_path)),
          repo_(std::make_unique<REPO>(db_path)) {}

    std::shared_ptr<Database> db_;
    std::unique_ptr<REPO> repo_;
    std::unordered_map<int64_t, DAO> cache_;
    std::mutex mutex_;
};

//-------------------------------------------------
//
//-------------------------------------------------
using CommandManager = Manager<CommandDAO, CommandRepository>;
using LogManager = Manager<LogDAO, LogRepository>;
using OperatorManager = Manager<OperatorDAO, OperatorRepository>;
using ResultManager = Manager<ResultDAO, ResultRepository>;
using SessionManager = Manager<SessionDAO, SessionRepository>;
using VictimManager = Manager<VictimDAO, VictimRepository>;
using VictimTemplateManager = Manager<VictimTemplateDAO, VictimTemplateRepository>;
