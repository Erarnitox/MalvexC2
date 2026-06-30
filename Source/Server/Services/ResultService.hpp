#pragma once

#include "ResultDAO.hpp"
#include "IManager.hpp"

class IResultService {
public:
    virtual ~IResultService() = default;
    virtual ResultDAO store_command_result(const UUID& command_uid, const std::string& data) = 0;
};

class ResultService final : public IResultService {
public:
    explicit ResultService(Manager<ResultDAO, ResultRepository>& manager) : manager_(manager) {}

    ResultDAO store_command_result(const UUID& command_uid, const std::string& data) override {
        ResultDAO result;
        result.command_uid = command_uid;
        result.data = data;
        return manager_.create(result);
    }

private:
    Manager<ResultDAO, ResultRepository>& manager_;
};
