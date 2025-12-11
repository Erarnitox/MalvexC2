#pragma once
#include <string>
#include "i_repository.hpp"
#include "database.hpp"
#include <memory>

class Repository : public IRepository<Res> {
public:
    explicit Repository(const std::string& db_path);

    ~Repository() override = default;

    std::vector<Res> list() override;
    std::optional<Res> get(int64_t id) override;
    Res create(Res& res) override;
    std::optional<Res> update(int64_t id, const Res& res) override;
    bool remove(int64_t id) override;

private:
    void ensure_table();
    std::unique_ptr<Database> db_;
};