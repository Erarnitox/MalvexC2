#pragma once

#include "OperatorDAO.hpp"
#include "OperatorManager.hpp"
#include <RESTServer.hpp>
#include <cpppwn.hpp>
#include <print>

//-------------------------------------------------
//
//-------------------------------------------------
static inline void register_operator_endpoints(cpppwn::RESTServer& server) {
    auto& operators = OperatorManager::instance();

    // ===== GET /api/operators - List all victims =====
    server.get("/api/operators", [&](const HttpRequest& req) {
        (void) req;
        std::vector<OperatorDAO> operator_list;
        operator_list = operators.getOperators();
        std::string json = to_json_array(operator_list);
        return HttpResponse().set_json(json);
    });

    // ===== GET /api/operator/:id - Get single operator by ID =====
    server.get("/api/operator", [&](const HttpRequest& req) {
        int16_t id;
        try{
            id = std::atol(req.query_params.at("id").c_str());
        } catch (...) {
            id = -1;
        }

        if (id == -1) {
            return HttpResponse()
                .set_status(400)
                .set_json(R"({"error":"Invalid opearator ID"})");
        }

        OperatorDAO* oper = operators.getOperator(id);

        if (not oper) {
            return HttpResponse()
                .set_status(404)
                .set_json(R"({"error":"Operator not found"})");
        }

        return HttpResponse().set_json(oper->to_json());
    });

    // ===== DELETE /api/operator/:id - Remove operator =====
    server.del("/api/operator", [&](const HttpRequest& req) {
        int16_t id;
        try{
            id = std::atol(req.query_params.at("id").c_str());
        } catch (...) {
            id = -1;
        }

        if (id == -1) {
            return HttpResponse()
                .set_status(400)
                .set_json(R"({"error":"Invalid operator ID"})");
        }

        bool success = operators.removeOperator(id);

        if (not success) {
            return HttpResponse()
                .set_status(404)
                .set_json(R"({"error":"Operator not found"})");
        }

        return HttpResponse()
            .set_status(204)  // No Content
            .set_json(R"({})");
    });
}