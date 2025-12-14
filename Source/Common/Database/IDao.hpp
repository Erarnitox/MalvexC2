#pragma once

#include <vector>
#include <optional>

template<typename Res>
struct IDao {
    virtual ~IDao() = default;
    virtual std::vector<Res> list() = 0;
    virtual std::optional<Res> get(long long id) = 0;
    virtual Res create(const Res& res) = 0;
    virtual std::optional<Res> update(long long id, const Res& res) = 0;
    virtual bool remove(long long id) = 0;
};