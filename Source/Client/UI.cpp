#include "UI.hpp"
#include "UiShared.hpp"
#include "Client.hpp"
#include "LogManager.hpp"
#include "Util/SafeString.hpp"

int main() {
    WindowState state{
        .show_about=false,
        .is_fullscreen=false,
        .res={ 1200, 800},
        .font={},
        .current_tab=Tab::CONNECTIONS,
        .is_connected=false,
        .user_settings={},
        .implant_settings={},
        .client=Client::instance(),
        .builder=Builder::instance(),
        .wait_for_response=false,
        .login_failed=false
    };

    static_cast<void>(malvex::safe_copy(state.user_settings.username.text, sizeof(state.user_settings.username.text), state.client.getUsername()));
    static_cast<void>(malvex::safe_copy(state.user_settings.password.text, sizeof(state.user_settings.password.text), state.client.getPassword()));
    static_cast<void>(malvex::safe_copy(state.user_settings.server_url.text, sizeof(state.user_settings.server_url.text), state.client.getServerUrl()));
    static_cast<void>(malvex::safe_copy(state.user_settings.default_timeout.text, sizeof(state.user_settings.default_timeout.text), state.client.getTimeout()));
    static_cast<void>(malvex::safe_copy(state.user_settings.output_file_path.text, sizeof(state.user_settings.output_file_path.text), state.client.getOutputPath()));

    static_cast<void>(malvex::safe_copy(state.implant_settings.username.text, sizeof(state.implant_settings.username.text), state.builder.getUsername()));
    static_cast<void>(malvex::safe_copy(state.implant_settings.password.text, sizeof(state.implant_settings.password.text), state.builder.getPassword()));
    static_cast<void>(malvex::safe_copy(state.implant_settings.server_url.text, sizeof(state.implant_settings.server_url.text), state.builder.getServerURL()));
    static_cast<void>(malvex::safe_copy(state.implant_settings.default_timeout.text, sizeof(state.implant_settings.default_timeout.text), state.builder.getTimeout()));
    static_cast<void>(malvex::safe_copy(state.implant_settings.output_file_path.text, sizeof(state.implant_settings.output_file_path.text), state.builder.getOutputDir()));
    static_cast<void>(malvex::safe_copy(state.implant_settings.service_name.text, sizeof(state.implant_settings.service_name.text), state.builder.getServiceName()));
    static_cast<void>(malvex::safe_copy(state.implant_settings.service_description.text, sizeof(state.implant_settings.service_description.text), state.builder.getServiceDesc()));

    ui::Resolution old_res = state.res;
    auto& client = Client::instance();

    const std::string title_bar_label = GuiIconText(ICON_DEMON, "Malvex C2 - GUI Client");

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(state.res.width, state.res.height, "Malvex C2 - GUI Client");
    SetTargetFPS(30);
    apply_malvex_theme();
    const auto bg_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    state.font = LoadFont("Resources/Font.ttf");
    if (state.font.texture.id == 0) {
        TraceLog(LOG_WARNING, "Failed to load Resources/Font.ttf, using default font");
        state.font = GetFontDefault();
    } else {
        SetTextureFilter(state.font.texture, TEXTURE_FILTER_BILINEAR);
    }
    GuiSetFont(state.font);

    if (client.hasServerSession()) {
        state.is_connected = true;
    }

    while (not WindowShouldClose()) {
        poll_login_async(state);

        BeginDrawing();
        ClearBackground(bg_color);

        state.res = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        };

        const auto title_bounds = ui::title_bar(state.res);
        DrawRectangleRec(title_bounds, {32, 32, 40, 255});

        if (GuiButton({ui::kPadding, 8, 90, 22}, GuiIconText(ICON_INFO, "About"))) {
            state.show_about = true;
        }

        const int title_align = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiLabel({0.0f, 8.0f, state.res.width, 22.0f}, title_bar_label.c_str());
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, title_align);

        constexpr float title_button_y = 8.0f;
        constexpr float title_button_h = 22.0f;
        constexpr float close_button_w = 24.0f;
        constexpr float fullscreen_button_w = 120.0f;
        const float close_button_x = state.res.width - ui::kPadding - close_button_w;
        const float fullscreen_button_x = close_button_x - ui::kGap - fullscreen_button_w;

        if (GuiButton({close_button_x, title_button_y, close_button_w, title_button_h}, "X")) {
            break;
        }

        if (GuiButton(
                {fullscreen_button_x, title_button_y, fullscreen_button_w, title_button_h},
                GuiIconText(ICON_CURSOR_SCALE_FILL, "Fullscreen"))) {
            state.is_fullscreen = not state.is_fullscreen;

            if (state.is_fullscreen) {
                int m = GetCurrentMonitor();
                old_res = state.res;
                ui::Resolution new_res{
                    static_cast<float>(GetMonitorWidth(m)),
                    static_cast<float>(GetMonitorHeight(m))
                };

                ToggleFullscreen();
                SetWindowSize(new_res.width, new_res.height);
            } else {
                ToggleFullscreen();
                SetWindowSize(old_res.width, old_res.height);
            }
        }

        if (not state.is_connected) {
            drawLogin(state);
        } else if (state.show_about) {
            drawAbout(state);
        } else {
            const auto tab_bar = ui::tab_bar(state.res);
            ui::draw_rounded_rect(tab_bar, panel_fill, panel_border);

            const float tab_gap = 6.0f;
            const float tab_width = (tab_bar.width - tab_gap * (tabs.size() + 1)) / static_cast<float>(tabs.size());

            for (size_t i = 0; i < tabs.size(); i++) {
                Rectangle tab_button{
                    tab_bar.x + tab_gap + static_cast<float>(i) * (tab_width + tab_gap),
                    tab_bar.y + 6.0f,
                    tab_width,
                    tab_bar.height - 12.0f,
                };

                int original_base = GuiGetStyle(BUTTON, BASE_COLOR_NORMAL);
                int original_text = GuiGetStyle(BUTTON, TEXT_COLOR_NORMAL);

                if (state.current_tab == static_cast<Tab>(i)) {
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt({150, 30, 70, 255}));
                    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt({255, 230, 120, 255}));
                }

                if (GuiButton(tab_button, tabs.at(i).c_str())) {
                    state.current_tab = static_cast<Tab>(i);
                }

                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, original_base);
                GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, original_text);
            }

            switch(state.current_tab) {
                case Tab::CONNECTIONS:
                    drawConnectionsTab(state);
                    break;
                case Tab::LOGS:
                    drawLogsTab(state);
                    break;
                case Tab::SETTINGS:
                    drawSettingsTab(state);
                    break;
                case Tab::BUILDER:
                    drawBuilderTab(state);
                    break;
                case Tab::TERMINAL:
                    drawSessionsTab(state);
                    break;
            }
        }

        if (not state.client.sendLogBuffer()) {
            LogManager::instance().local_log("Failed to send Attack logs to the Server!");
        }

        const auto status_bounds = ui::status_bar(state.res);
        ui::draw_rounded_rect(status_bounds, panel_fill, panel_border);
        GuiStatusBar(ui::inset(status_bounds, 4.0f), client.getStatusText());

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
