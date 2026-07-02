#pragma once

#include "UI.hpp"

#include <raylib.h>

namespace ui::theme {
inline constexpr Color kInputText{120, 255, 100, 255};
inline constexpr Color kInputTextFocused{170, 255, 150, 255};
inline constexpr Color kInputTextActive{205, 255, 190, 255};
inline constexpr Color kInputTextDisabled{72, 190, 68, 255};
inline constexpr Color kInputBorderFocused{80, 220, 90, 255};
} // namespace ui::theme

void apply_malvex_theme();
void draw_panel_title(Rectangle bounds, const char* title);
void draw_malvex_panel(Rectangle bounds, const char* title, Color fill, Color border);
void draw_malvex_section_header(Rectangle content, const char* title);

void drawConnectionsTab(WindowState& state);
void drawLogsTab(WindowState& state);
void drawSettingsTab(WindowState& state);
void drawBuilderTab(WindowState& state);
void drawArtifactsTab(WindowState& state);
void drawSessionsTab(WindowState& state);
void drawAbout(WindowState& state);
void drawLogin(WindowState& state);

void poll_login_async(WindowState& state);
int ui_scroll_bar(Rectangle bounds, int value, int minValue, int maxValue);
