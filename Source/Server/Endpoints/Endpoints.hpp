#pragma once

#include <cpppwn.hpp>

#include <IManager.hpp>

#include <stacktrace>
#include <string>
#include <optional>
#include <functional>
#include <stdexcept>

//-------------------------------------------------
// Helper Functions
//-------------------------------------------------
inline std::optional<int64_t> extract_id(const HttpRequest& req, const std::string& param = "id") {
    try {
        if (req.query_params.find(param) != req.query_params.end()) {
            return std::stoll(req.query_params.at(param));
        }
    } catch (...) {
        return std::nullopt;
    }
    return std::nullopt;
}

//-------------------------------------------------
//
//-------------------------------------------------
inline HttpResponse error_response(int status, const std::string& message) {
    return HttpResponse()
        .set_status(status)
        .set_json(R"({"error":")" + message + R"("})");
}

//-------------------------------------------------
//
//-------------------------------------------------
inline HttpResponse success_response(const std::string& message = "success") {
    return HttpResponse()
        .set_json(R"({"message":")" + message + R"("})");
}

//-------------------------------------------------
// Templated REST Endpoint Registration
//-------------------------------------------------
template<typename DAO, typename REPO>
class RESTEndpoints {
public:
    using ManagerType = Manager<DAO, REPO>;

    struct Config {
        std::string base_path;          // e.g., "/api/operators"
        std::string resource_name;      // e.g., "operator"
        ManagerType& manager;
    };

    //-------------------------------------------------
    //
    //-------------------------------------------------
    static void register_endpoints(cpppwn::RESTServer& server, const Config& config) {
        register_list(server, config);
        register_get(server, config);
        register_create(server, config);
        register_delete(server, config);
    }

private:
    //-------------------------------------------------
    // GET /api/resources - List all
    //-------------------------------------------------
    static void register_list(cpppwn::RESTServer& server, const Config& config) {
        server.get(config.base_path, [config](const HttpRequest& req) {
            (void)req;

            try {
                auto items = config.manager.get_all();
                std::string json = to_json_array(items);
                return HttpResponse().set_json(json);
            } catch (const std::exception& e) {
                return error_response(500, std::string("Internal error: ") + e.what());
            }
        });
    }

    //-------------------------------------------------
    // GET /api/resource?id=X - Get by ID
    //-------------------------------------------------
    static void register_get(cpppwn::RESTServer& server, const Config& config) {
        std::string path = config.base_path.substr(0, config.base_path.length() - 1);

        server.get(path, [config](const HttpRequest& req) {
            auto id = extract_id(req);

            if (not id) {
                return error_response(400, "Invalid " + config.resource_name + " ID");
            }

            try {
                auto item = config.manager.get(*id);

                if (not item) {
                    return error_response(404, config.resource_name + " not found");
                }

                return HttpResponse().set_json(item->to_json());
            } catch (const std::exception& e) {
                return error_response(500, std::string("Internal error: ") + e.what());
            }
        });
    }

    //-------------------------------------------------
    // POST /api/resources - Create new
    //-------------------------------------------------
    static void register_create(cpppwn::RESTServer& server, const Config& config) {
        server.post(config.base_path, [config](const HttpRequest& req) {
            try {
                DAO item = DAO::from_json(req.body);
                auto created = config.manager.create(item);

                return HttpResponse()
                    .set_status(201)
                    .set_json(created.to_json());
            } catch (const std::exception& e) {
                return error_response(400, std::string("Invalid request: ") + e.what());
            }
        });
    }

    //-------------------------------------------------
    // DELETE /api/resource?id=X - Delete
    //-------------------------------------------------
    static void register_delete(cpppwn::RESTServer& server, const Config& config) {
        std::string path = config.base_path.substr(0, config.base_path.length() - 1);

        server.del(path, [config](const HttpRequest& req) {
            auto id = extract_id(req);

            if (not id) {
                return error_response(400, "Invalid " + config.resource_name + " ID");
            }

            try {
                bool success = config.manager.remove(*id);

                if (not success) {
                    return error_response(404, config.resource_name + " not found");
                }

                return HttpResponse().set_status(204).set_json("{}");
            } catch (const std::exception& e) {
                return error_response(500, std::string("Internal error: ") + e.what());
            }
        });
    }
};

//-------------------------------------------------
// Usage: Register Operator endpoints
//-------------------------------------------------
inline void register_operator_endpoints(cpppwn::RESTServer& server) {
    auto& operators = OperatorManager::instance();

    RESTEndpoints<OperatorDAO, OperatorRepository>::register_endpoints(server, {
        .base_path = "/api/operators",
        .resource_name = "operator",
        .manager = operators
    });
}

//-------------------------------------------------
// Usage: Register Victim endpoints
//-------------------------------------------------
inline void register_victim_endpoints(cpppwn::RESTServer& server) {
    auto& victims = VictimManager::instance();

    RESTEndpoints<VictimDAO, VictimRepository>::register_endpoints(server, {
        .base_path = "/api/victims",
        .resource_name = "victim",
        .manager = victims
    });
}

//-------------------------------------------------
// Usage: Register Command endpoints
//-------------------------------------------------
inline void register_command_endpoints(cpppwn::RESTServer& server) {
    auto& commands = CommandManager::instance();

    RESTEndpoints<CommandDAO, CommandRepository>::register_endpoints(server, {
        .base_path = "/api/commands",
        .resource_name = "command",
        .manager = commands
    });
}

//-------------------------------------------------
// Usage: Register Session endpoints
//-------------------------------------------------
inline void register_session_endpoints(cpppwn::RESTServer& server) {
    auto& sessions = SessionManager::instance();

    RESTEndpoints<SessionDAO, SessionRepository>::register_endpoints(server, {
        .base_path = "/api/sessions",
        .resource_name = "session",
        .manager = sessions
    });
}

//-------------------------------------------------
// Usage: Register Log endpoints
//-------------------------------------------------
inline void register_log_endpoints(cpppwn::RESTServer& server) {
    auto& logs = LogManager::instance();

    RESTEndpoints<LogDAO, LogRepository>::register_endpoints(server, {
        .base_path = "/api/logs",
        .resource_name = "log",
        .manager = logs
    });
}

//-------------------------------------------------
// Usage: Register Result endpoints
//-------------------------------------------------
inline void register_result_endpoints(cpppwn::RESTServer& server) {
    auto& results = ResultManager::instance();

    RESTEndpoints<ResultDAO, ResultRepository>::register_endpoints(server, {
        .base_path = "/api/results",
        .resource_name = "result",
        .manager = results
    });
}

//-------------------------------------------------
// Register All Endpoints
//-------------------------------------------------
inline void register_attacker_endpoints(cpppwn::RESTServer& server) {
    register_operator_endpoints(server);
    register_victim_endpoints(server);
    register_command_endpoints(server);
    register_session_endpoints(server);
    register_log_endpoints(server);
    register_result_endpoints(server);
}