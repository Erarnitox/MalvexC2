#pragma once

#include <raylib.h>
#include <raygui.h>

#define RAYGUI_STYLE_DARK
#include <styles/dark/style_dark.h>

#include <array>
#include <string>

struct Resolution {
    float width;
    float height;
};

std::array<std::string, 5> tabs {
    GuiIconText(ICON_MONITOR, "Connections"),
    GuiIconText(ICON_TEXT_NOTES, "Logs"),
    GuiIconText(ICON_GEAR, "Settings"),
    GuiIconText(ICON_PENCIL_BIG, "Builder"),
    GuiIconText(ICON_WINDOW, "Terminal")
};

enum Tab {
    CONNECTIONS = 0,
    LOGS = 1,
    SETTINGS = 2,
    BUILDER = 3,
    TERMINAL = 4
};

struct WindowState {
    bool show_about = false;
    bool is_fullscreen = false;
    Resolution res;
};