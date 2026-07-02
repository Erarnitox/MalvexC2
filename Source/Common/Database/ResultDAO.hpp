#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct ResultDAO : public IDao<ResultDAO> {
    int64_t id{0};
    UUID uid;
    UUID command_uid;
    UUID victim_uid;
    std::string kind;
    int status{1};
    int chunk_index{0};
    int chunk_total{1};
    int64_t created_at{0};
    std::string data;
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<ResultDAO> {
    using T = ResultDAO;
    static constexpr auto value = object(
        "result_id", &T::id,
        "result_uid", &T::uid,
        "command_uid", &T::command_uid,
        "victim_uid", &T::victim_uid,
        "kind", &T::kind,
        "status", &T::status,
        "chunk_index", &T::chunk_index,
        "chunk_total", &T::chunk_total,
        "created_at", &T::created_at,
        "data", &T::data
    );
};