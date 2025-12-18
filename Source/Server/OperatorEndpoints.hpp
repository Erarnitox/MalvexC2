#pragma once

#include "OperatorManager.hpp"
#include <cpppwn.hpp>

//-------------------------------------------------
//
//-------------------------------------------------
static inline void register_victims_endpoints(cpppwn::RESTServer& server) {
    auto& operators = OperatorManager::instance();

    // ===== GET /api/operators - List all victims =====
    server.get("/api/operators", [&](const HttpRequest& req) {
        std::vector<OperatorDAO> operator_list;
        operator_list = operators.getOperators();
        std::string json = to_json_array(operator_list);
        return HttpResponse().set_json(json);
    });

    // ===== GET /api/victims/:id - Get single victim by ID =====
    server.get("/api/operators/", [&](const HttpRequest& req) {
        int64_t id = get_path_param_id(req.path, "/api/victims/");

        if (id == -1) {
            return HttpResponse()
                .set_status(400)
                .set_json(R"({"error":"Invalid victim ID"})");
        }

        auto* victim = victims.getVictim(id);

        if (!victim) {
            return HttpResponse()
                .set_status(404)
                .set_json(R"({"error":"Victim not found"})");
        }

        std::string json = victim->to_json();
        return HttpResponse().set_json(json);
    });

    // ===== PUT /api/victims/:id - Update victim =====
    api_server.put("/api/victims/", [&](const HttpRequest& req) {
        int64_t id = get_path_param_id(req.path, "/api/victims/");

        if (id == -1) {
            return HttpResponse()
                .set_status(400)
                .set_json(R"({"error":"Invalid victim ID"})");
        }

        try {
            // Parse request body
            VictimDAO updated_victim = VictimDAO::from_json(req.body);

            // Update in repository
            auto& repo = VictimRepository::instance();
            auto result = repo.update(id, updated_victim);

            if (!result) {
                return HttpResponse()
                    .set_status(404)
                    .set_json(R"({"error":"Victim not found or update failed"})");
            }

            std::string json = result->to_json();
            return HttpResponse().set_json(json);

        } catch (const std::exception& e) {
            return HttpResponse()
                .set_status(400)
                .set_json(std::string(R"({"error":")") + e.what() + R"("})");
        }
    });

    // ===== DELETE /api/victims/:id - Remove victim =====
    api_server.delete_("/api/victims/", [&](const HttpRequest& req) {
        int64_t id = get_path_param_id(req.path, "/api/victims/");

        if (id == -1) {
            return HttpResponse()
                .set_status(400)
                .set_json(R"({"error":"Invalid victim ID"})");
        }

        bool success = victims.removeVictim(id);

        if (!success) {
            return HttpResponse()
                .set_status(404)
                .set_json(R"({"error":"Victim not found"})");
        }

        return HttpResponse()
            .set_status(204)  // No Content
            .set_json(R"({})");
    });
}