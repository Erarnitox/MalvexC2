#pragma once

#include "Config.hpp"
#include "LogManager.hpp"
#include <ImplantConfig.hpp>

#include <string>

class Builder {
    ImplantConfig m_config;
    Config& m_conf;
    LogManager& m_log_man;

public:
    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;
    Builder(Builder&&) = delete;
    Builder& operator=(Builder&&) = delete;
    explicit Builder(const std::string& db_path = "client.db");

    static Builder& instance(const std::string& db_path = "client.db");

    void setUsername(const std::string& username);
    void setPassword(const std::string& password);
    void setTimeout(const std::string& timeout);
    void setServerURL(const std::string& server_url);
    void setServiceName(const std::string& service_name);
    void setServiceDesc(const std::string& service_description);

    [[nodiscard]] std::string getUsername() const noexcept;
    [[nodiscard]] std::string getPassword() const noexcept;
    [[nodiscard]] std::string getTimeout() const noexcept;
    [[nodiscard]] std::string getServerURL() const noexcept;
    [[nodiscard]] std::string getServiceName() const noexcept;
    [[nodiscard]] std::string getServiceDesc() const noexcept;

    bool buildImplant(const std::string& output_path);
};