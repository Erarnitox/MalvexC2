#pragma once

#include "UI.hpp"

#include <raylib.h>

void apply_malvex_theme();
void draw_panel_title(Rectangle bounds, const char* title);
void draw_malvex_panel(Rectangle bounds, const char* title, Color fill, Color border);
void draw_malvex_section_header(Rectangle content, const char* title);

void drawConnectionsTab(WindowState& state);
void drawLogsTab(WindowState& state);
void drawSettingsTab(WindowState& state);
void drawBuilderTab(WindowState& state);
void drawSessionsTab(WindowState& state);
void drawAbout(WindowState& state);
void drawLogin(WindowState& state);

void poll_login_async(WindowState& state);
int ui_scroll_bar(Rectangle bounds, int value, int minValue, int maxValue);
