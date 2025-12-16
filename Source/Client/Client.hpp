#pragma once

#include "Builder.hpp"
#include "Config.hpp"
#include "LogManager.hpp"
#include "CommandManager.hpp"
#include "VictimManager.hpp"
#include "SessionManager.hpp"

#include <cpppwn.hpp>

#include <string>


class Client {
private:
    Config& m_config;
    Builder& m_builder;

    LogManager& m_log_man;
    CommandManager& m_cmd_man;
    VictimManager& m_vic_man;
    SessionManager& m_sess_man;

    cpppwn::RESTClient m_rest_client;
    std::string m_status_text;

    mutable size_t victim_count;

    void updateStatusText() noexcept;

public:
    // Delete copy and move constructors/assignments (singleton pattern)
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
    Client(const std::string& db_path = "client.db");

    static Client& instance(const std::string& db_path = "client.db");

    [[nodiscard]]
    bool hasServerSession() const noexcept;

    [[nodiscard]]
    std::string getUsername() const noexcept;

    [[nodiscard]]
    std::string getPassword() const noexcept;

    [[nodiscard]]
    std::string getServerUrl() const noexcept;

    [[nodiscard]]
    std::string getOutputPath() const noexcept;

    void setUsername(const std::string& username) noexcept;

    void setPassword(const std::string& password) noexcept;

    void setServerUrl(const std::string& server_url) noexcept;

    void setOutputPath(const std::string& output_path) noexcept;

    const char* getStatusText() const noexcept;

    const std::vector<Victim>& getVictims() const noexcept;

    // REST Methods
    [[nodiscard]] bool login() noexcept;

    [[nodiscard]] bool fetchVictims() noexcept;
};