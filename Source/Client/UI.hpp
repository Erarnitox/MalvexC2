#pragma once

#include "Builder.hpp"
#include "Client.hpp"
#include "SessionDAO.hpp"
#include "SessionManager.hpp"
#include "UiLayout.hpp"
#include <atomic>
#include <cstring>
#include <raylib.h>
#include <raygui.h>

#include <array>
#include <format>
#include <optional>
#include <string>

inline std::array<std::string, 6> tabs {
    GuiIconText(ICON_MONITOR, "Connections"),
    GuiIconText(ICON_TEXT_NOTES, "Logs"),
    GuiIconText(ICON_GEAR, "Settings"),
    GuiIconText(ICON_PENCIL_BIG, "Builder"),
    GuiIconText(ICON_FILE_SAVE, "Artifacts"),
    GuiIconText(ICON_WINDOW, "Terminal")
};

enum Tab {
    CONNECTIONS = 0,
    LOGS = 1,
    SETTINGS = 2,
    BUILDER = 3,
    ARTIFACTS = 4,
    TERMINAL = 5
};

constexpr size_t MAX_INPUT_CHARS{ 256 };

struct InputField {
    char text[MAX_INPUT_CHARS];
    bool edit = false;
};

struct MalvexSettings {
    InputField username;
    InputField password;
    InputField default_timeout;
    InputField server_url;
    InputField output_file_path;
};

struct BuilderSettings {
    InputField username;
    InputField password;
    InputField default_timeout;
    InputField server_url;
    InputField output_file_path;
    InputField service_name;
    InputField service_description;
};

struct WindowState {
    bool show_about = false;
    bool is_fullscreen = false;
    ui::Resolution res;
    Font font;
    Tab current_tab;
    bool is_connected;
    MalvexSettings user_settings;
    BuilderSettings implant_settings;
    Client& client;
    Builder& builder;
    std::atomic<bool> wait_for_response;
    bool login_failed;
    std::optional<std::string> highlight_command_uid;
    int artifacts_poll_counter{0};
};
