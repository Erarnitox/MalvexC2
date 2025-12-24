#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct ResultDAO : public IDao<ResultDAO> {
    int64_t id{0};
    UUID uid;
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
        "data", &T::data
    );
};