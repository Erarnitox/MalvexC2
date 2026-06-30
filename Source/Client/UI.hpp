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
#include <string>

//--------------------------------
//
//--------------------------------
inline std::array<std::string, 5> tabs {
    GuiIconText(ICON_MONITOR, "Connections"),
    GuiIconText(ICON_TEXT_NOTES, "Logs"),
    GuiIconText(ICON_GEAR, "Settings"),
    GuiIconText(ICON_PENCIL_BIG, "Builder"),
    GuiIconText(ICON_WINDOW, "Terminal")
};

//--------------------------------
//
//--------------------------------
enum Tab {
    CONNECTIONS = 0,
    LOGS = 1,
    SETTINGS = 2,
    BUILDER = 3,
    TERMINAL = 4
};

//--------------------------------
//
//--------------------------------
constexpr size_t MAX_INPUT_CHARS{ 256 };

//--------------------------------
//
//--------------------------------
struct InputField {
    char text[MAX_INPUT_CHARS];
    bool edit = false;
};

//--------------------------------
//
//--------------------------------
struct MalvexSettings {
    InputField username;
    InputField password;
    InputField default_timeout;
    InputField server_url;
    InputField output_file_path;
};

//--------------------------------
//
//--------------------------------
struct BuilderSettings {
    InputField username;
    InputField password;
    InputField default_timeout;
    InputField server_url;
    InputField output_file_path;
    InputField service_name;
    InputField service_description;
};

//--------------------------------
//
//--------------------------------
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
};

//--------------------------------
//
//--------------------------------
inline void run_terminal_command(const char* command, std::string& output, const SessionDAO& session) {
    static SessionManager& sessionMan = SessionManager::instance();

    if (strnlen(command, 5) < 2) return;

    if (sessionMan.getBridgeState(session) != SessionBridgeState::Ready) {
        output += std::format("\n> {}\n<Session not ready>\n", command);
        return;
    }

    if (strcmp(command, "mlvx_help") == 0) {
        output += std::format(
            "> {}\nAvailable commands:\n"
            "  mlvx_help     - Show this help message\n",
            command);
    } else if (strcmp(command, "clear") == 0 || strcmp(command, "cls") == 0) {
        output.clear();
    } else {
        sessionMan.discardPendingOutput(session);
        output += std::format("\n> {}\n{}", command, sessionMan.execute(session, command));
    }
}