#pragma once

#include <cstdint>
#include <vector>
#include <optional>

template <typename Res>
struct IRepository {
    virtual ~IRepository() = default;

    virtual std::vector<Res> list() = 0;
    virtual std::optional<Res> get(int64_t id) = 0;
    virtual Res create(const Res& res) = 0;
    virtual std::optional<Res> update(int64_t id, const Res& res) = 0;
    virtual bool remove(int64_t id) = 0;
};