#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct ResultDAO : public IDao<ResultDAO> {
    int64_t result_id{0};
    UUID result_uid;
    std::string data;
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<ResultDAO> {
    using T = ResultDAO;
    static constexpr auto value = object(
        "result_id", &T::result_id,
        "result_uid", &T::result_uid,
        "data", &T::data
    );
};