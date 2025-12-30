#pragma once

#include "Builder.hpp"
#include "Client.hpp"
#include "SessionDAO.hpp"
#include "SessionManager.hpp"
#include <atomic>
#include <raylib.h>
#include <raygui.h>

#define RAYGUI_STYLE_DARK
#include <styles/dark/style_dark.h>

#include <array>
#include <string>

//--------------------------------
//
//--------------------------------
struct Resolution {
    float width;
    float height;
};

//--------------------------------
//
//--------------------------------
std::array<std::string, 5> tabs {
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
    Resolution res;
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
void inline run_terminal_command(const char* command, char* output, size_t outputSize, const SessionDAO& session) {
    static SessionManager& sessionMan = SessionManager::instance();
    char tempOutput[1024];

    // Built in Commands
    if (strcmp(command, "mlvx_help") == 0) {
        snprintf(tempOutput, sizeof(tempOutput),
                 "> %s\nAvailable commands:\n"
                 "  mlvx_help     - Show this help message\n",
                 command);
    } else {
        // Execute Remote Shell Commands
        snprintf(tempOutput, sizeof(tempOutput), "> %s\n%s", command, sessionMan.execute(session, command).c_str());
    }


    // Append to output
    if (strlen(output) + strlen(tempOutput) < outputSize - 1) {
        strcat(output, tempOutput);
    }
}