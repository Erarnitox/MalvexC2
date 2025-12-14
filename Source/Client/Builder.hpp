#pragma once

#include <ImplantConfig.hpp>

#include <string>

class Builder {
    ImplantConfig m_config;
public:
    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;
    Builder(Builder&&) = delete;
    Builder& operator=(Builder&&) = delete;
    Builder() = default;

    static Builder& instance();

    void setUsername(const std::string& username);
    void setPassword(const std::string& password);
    void setTimeout(const std::string& timeout);
    void setServerURL(const std::string& server_url);
    void setServiceName(const std::string& service_name);
    void setServiceDesc(const std::string& service_description);

    bool buildImplant(const std::string& output_path);
};