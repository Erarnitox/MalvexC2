#pragma once

#include "Util/SafeLogger.hpp"
#include "VictimTemplateDAO.hpp"
#include "VictimTemplateRepository.hpp"
#include <cpppwn.hpp>

#include <IManager.hpp>

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

std::optional<int64_t> extract_id(const HttpRequest& req, const std::string& param = "id");
HttpResponse error_response(int status, const std::string& message);
HttpResponse success_response(const std::string& message = "success");

template<typename DAO, typename REPO>
class RESTEndpoints {
public:
    using ManagerType = Manager<DAO, REPO>;

    struct Config {
        std::string base_path;
        std::string resource_name;
        ManagerType& manager;
    };

    static void register_endpoints(cpppwn::RESTServer& server, const Config& config) {
        register_list(server, config);
        register_get(server, config);
        register_create(server, config);
        register_delete(server, config);
    }

private:
    static void register_list(cpppwn::RESTServer& server, const Config& config);
    static void register_get(cpppwn::RESTServer& server, const Config& config);
    static void register_create(cpppwn::RESTServer& server, const Config& config);
    static void register_delete(cpppwn::RESTServer& server, const Config& config);
};

void register_operator_endpoints(cpppwn::RESTServer& server);
void register_victim_endpoints(cpppwn::RESTServer& server);
void register_command_endpoints(cpppwn::RESTServer& server);
void register_log_endpoints(cpppwn::RESTServer& server);
void register_result_endpoints(cpppwn::RESTServer& server);
void register_victim_template_endpoints(cpppwn::RESTServer& server);
void register_attacker_endpoints(cpppwn::RESTServer& server);
