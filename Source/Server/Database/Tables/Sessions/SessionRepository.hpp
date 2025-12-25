#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "SessionDAO.hpp"

#include <string>
#include <memory>

class SessionRepository : public IRepository<SessionDAO> {
public:
    explicit SessionRepository(const std::string& db_path);

    ~SessionRepository() override = default;

    std::vector<SessionDAO> list() const override;
    std::optional<SessionDAO> get(int64_t id) const override;
    std::optional<SessionDAO> get(const UUID& uid) const override;
    SessionDAO create(const SessionDAO& op) override;
    std::optional<SessionDAO> update(int64_t id, const SessionDAO& op) override;
    bool remove(int64_t id) override;

    std::vector<SessionDAO> list_by_port(int port);
    [[nodiscard]] bool remove_by_port(int port);
    [[nodiscard]] int count();

    void commit();

private:
    void ensure_table();
    std::unique_ptr<Database> db_;
};