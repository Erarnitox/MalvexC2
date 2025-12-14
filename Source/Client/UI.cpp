#include "UI.hpp"

#include <raylib.h>
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
        .is_connected=false
    };

    Resolution old_res = state.res;

    const std::string title{ GuiIconText(ICON_DEMON, "Malvex C2 - GUI Client") };

    // Set up the window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(state.res.width, state.res.height, title.c_str());
    SetTargetFPS(30);
    GuiLoadStyleDark();
    const auto bg_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));

    // Set custom font
    state.font = LoadFont("Font.ttf");
    GuiSetFont(state.font);

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

        // --- Status Bar ---
        Rectangle status = {0, state.res.height - 24, state.res.width, 24};
        GuiStatusBar(status, "Version: 1.0 | Connections: 3 | Ports: 1");

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

    const auto& res{ state.res };

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "List of Victim Connections");

    if (GuiButton({3, 58, 120, 20}, GuiIconText(ICON_REPEAT_FILL, "Refresh List"))) {
        //TODO: Fetch clients from server
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

    int clients = 50;
    const char* values[] = {
        "1", "Laptop", "192.168.0.108", "8.8.8.8", "Linux",
        "erarnitox", "Online"
    };

    colWidth = (tableRect.width-10) / colCount;
    for (int client_id{ 0 }; client_id < clients; ++client_id) {
        auto line_color = client_id % 2 == 0 ? Color{150, 20, 70, 255} : DARKGRAY;

        // Hover highlight
        if (client_id == hoveredRow) {
            line_color = RED;
        }

        for (int i = 0; i < colCount; i++) {
            Rectangle r{tableRect.x + i * colWidth, tableRect.y + scroll.y + 26 + (25*client_id), colWidth, 24};
            GuiDrawRectangle(r, 1, BLACK, line_color);
            GuiLabel(r, values[i]);
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

        if (GuiButton(btn1, TextFormat("Timeout Client %d for %d min", selectedRow, 10))) {
            // handle action
            menuVisible = false;
        }
        if (GuiButton(btn2, TextFormat("Open Shell (Port: %d)", 4444))) {
            // handle action
            menuVisible = false;
            state.current_tab = Tab::TERMINAL;
        }
        if (GuiButton(btn3, "Close open Shells")) {
            // handle action
            menuVisible = false;
        }
        if (GuiButton(btn4, "Take Screenshot")) {
            // handle action
            menuVisible = false;
        }
        if (GuiButton(btn5, "Loot Everything!")) {
            // handle action
            menuVisible = false;
        }
        if (GuiButton(btn6, "Start Keylogger")) {
            // handle action
            menuVisible = false;
        }
        if (GuiButton(btn7, "Stop Keylogger")) {
            // handle action
            menuVisible = false;
        }
        if (GuiButton(btn8, "Uninstall Implant")) {
            // handle action
            menuVisible = false;
        }
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawLogin(WindowState& state) {
    static const Texture2D texture = LoadTexture("Logo.png");

    Rectangle popupRect = { state.res.width/2 - 300, state.res.height/2 - 130, 600, 250 };
    GuiPanel(popupRect, GuiIconText(ICON_DEMON, "Connect to MalvexC2 Server"));

    // Draw image inside popup
    float imageX = popupRect.x + 10;
    float imageY = popupRect.y + 40;
    float imageWidth = 200;
    float imageHeight = 200;

    DrawTexturePro(
        texture,
        (Rectangle){ 0, 0, (float)texture.width, (float)texture.height },
        (Rectangle){ imageX, imageY, imageWidth, imageHeight },
        (Vector2){ 0, 0 },
        0.0f,
        WHITE
    );

    // Username field
    GuiLabel((Rectangle){ popupRect.x + 250, popupRect.y + 40, 90, 20 }, "Username:");
    if (GuiTextBox((Rectangle){ popupRect.x + 350, popupRect.y + 40, 200, 20 },
        "", MAX_INPUT_CHARS, false)) {
        //settings.username.edited = !settings.username.edited;
    }

    // Password field
    GuiLabel((Rectangle){ popupRect.x + 250, popupRect.y + 10 + 30*2, 90, 20 }, "Password:");
    if (GuiTextBox((Rectangle){ popupRect.x + 350, popupRect.y + 10 + 30*2, 200, 20 },
        "", MAX_INPUT_CHARS, false)) {
        //settings.username.edited = !settings.username.edited;
    }

    // Server URL field
    GuiLabel((Rectangle){ popupRect.x + 250, popupRect.y + 10 + 30*3, 90, 20 }, "Server:");
    if (GuiTextBox((Rectangle){ popupRect.x + 350, popupRect.y + 10 + 30*3, 200, 20 },
        "", MAX_INPUT_CHARS, false)) {
        //settings.username.edited = !settings.username.edited;
    }

    // Login Button
    if (GuiButton((Rectangle){ popupRect.x + 250, popupRect.y + popupRect.height - 90, 300, 30 }, "Login")) {
        state.is_connected = true;
    }

    // Local Server Button
    if (GuiButton((Rectangle){ popupRect.x + 250, popupRect.y + popupRect.height - 50, 300, 30 }, "Start Local Server")) {
        state.is_connected = true;
    }

}

//-------------------------------------------------
//
//-------------------------------------------------
void drawAbout(WindowState& state) {
    static const Texture2D texture = LoadTexture("Logo.png");

    Rectangle popupRect = { state.res.width/2 - 300, state.res.height/2 - 130, 600, 250 };
    GuiPanel(popupRect, GuiIconText(ICON_DEMON, "About MalvexC2"));

    // Draw image inside popup
    float imageX = popupRect.x + 10;
    float imageY = popupRect.y + 40;
    float imageWidth = 200;
    float imageHeight = 200;

    DrawTexturePro(
        texture,
        (Rectangle){ 0, 0, (float)texture.width, (float)texture.height },
        (Rectangle){ imageX, imageY, imageWidth, imageHeight },
        (Vector2){ 0, 0 },
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
    if (GuiButton((Rectangle){ popupRect.x + 250, popupRect.y + popupRect.height - 50, 300, 30 }, "Close")) {
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

    auto& res = state.res;

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "C2 Event Log");

    if (GuiButton({3, 58, 120, 20}, GuiIconText(ICON_REPEAT_FILL, "Refresh Logs"))) {
        //TODO: Fetch logs from server
    }

    Rectangle viewRect{0, 80, res.width, res.height - 90};
    GuiPanel(viewRect, "");

    // Add log entries on button press
    if (IsKeyPressed(KEY_SPACE)) {
        char newEntry[64];
        snprintf(newEntry, sizeof(newEntry), "Log entry at frame %d\n", GetFrameTime());

        // Append to log (with size check)
        if (strlen(logText) + strlen(newEntry) < MAX_LOG_SIZE - 1) {
            strcat(logText, newEntry);
        }
    }

    // Use GuiScrollPanel for scrollable content
    GuiScrollPanel(
        viewRect,
        NULL,  // No title
        (Rectangle){ 0, 0, viewRect.width - 20, 2000 },  // Content area (height estimated)
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

    DrawTextEx(state.font, logText, (Vector2){
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
    static MalvexSettings settings{
        .username={"user123"},
        .password={"123456"},
        .default_timeout={"5"},
        .server_url={"https://api.example.com"},
        .output_file_path={"/tmp/output"}
    };
    static char displayPassword[MAX_INPUT_CHARS] = {0};
    static bool showPasswordAsText{ false };

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
    if (GuiTextBox((Rectangle){ inputX, startY, inputWidth, inputHeight },
                    settings.username.text, MAX_INPUT_CHARS, settings.username.edited)) {
        settings.username.edited = !settings.username.edited;
    }

    // Password field
    GuiLabel({labelX, startY + 5 + spacing, labelWidth, labelHeight }, "Password:");
    if (GuiTextBox((Rectangle){ inputX, startY + spacing, inputWidth, inputHeight },
                    settings.password.text, MAX_INPUT_CHARS, settings.password.edited)) {
        settings.password.edited = !settings.password.edited;
    }

    // Timeout field
    GuiLabel({labelX, startY + 5 + spacing*2, labelWidth, labelHeight }, "Timeout:");
    if (GuiTextBox((Rectangle){ inputX, startY + spacing*2, inputWidth, inputHeight },
                    settings.default_timeout.text, MAX_INPUT_CHARS, settings.default_timeout.edited)) {
        settings.default_timeout.edited = !settings.default_timeout.edited;
    }

    // Server URL field
    GuiLabel({labelX, startY + 5 + spacing*3, labelWidth, labelHeight }, "Server:");
    if (GuiTextBox((Rectangle){ inputX, startY + spacing*3, inputWidth, inputHeight },
                    settings.server_url.text, MAX_INPUT_CHARS, settings.server_url.edited)) {
        settings.server_url.edited = !settings.server_url.edited;
    }

    // File path field with browse button
    GuiLabel({ labelX, startY + spacing*4 + 5, labelWidth, labelHeight }, "Output Dir:");
    if (GuiTextBox((Rectangle){ inputX, startY + spacing*4, inputWidth - 110, inputHeight },
                    settings.output_file_path.text, MAX_INPUT_CHARS, settings.output_file_path.edited)) {
        settings.output_file_path.edited = !settings.output_file_path.edited;
    }

    // Browse button
    if (GuiButton((Rectangle){ inputX + inputWidth - 100, startY + spacing*4, 100, inputHeight },"Browse...")) {
        // In a real application, you would open a file dialog here
        // For demonstration, we'll just show it was clicked
        printf("Browse button clicked!\n");
        // You could use a library like tinyfiledialogs for actual file selection
    }

    // Save button
    Rectangle saveButtonRect = { inputX, startY + spacing*5, 200, 30 };
    if (GuiButton(saveButtonRect, "Save Settings")) {
        printf("Settings saved!\n");
        printf("Username: %s\n", settings.username.text);
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void drawBuilderTab(WindowState& state) {
    const auto& res = state.res;

    GuiLabel({res.width/2 - 100, 50, 200, 30}, "Implant Builder");

    Rectangle viewRect{0, 80, res.width, res.height - 90};
    GuiPanel(viewRect, "");
}

//TODO: remove
struct Session {
    std::string client;
    int16_t port;
    bool open;
};

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

    static std::vector<Session> sessions {
        {"userName", 4444, true },
        {"user2", 4444, true },
        {"user3", 4444, true },
        {"user4", 4444, true },
        {"user5", 4444, true }
    };

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
    GuiPanel(viewRect, TextFormat("Reverse Shell to Client %s on port %d", session.client.c_str(), session.port));

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
                (Vector2){ outputRect.x + 5, outputRect.y + 5 - scrollOffset },
                16,
                1,
                PINK);  // Green terminal text

    EndScissorMode();

    // Command prompt area
    DrawText(">", 10, state.res.height - 60, 30, RED);

    Rectangle commandRect = { 30, res.height - 60, res.width - 200, 30 };

    // Execute button
    if (GuiButton((Rectangle){ res.width - 150, res.height - 60, 55, 30 }, "Run") ||
       GuiTextBox(commandRect, commandInput, 1024, commandEditMode)) {
        if (strlen(commandInput) > 0) {
            //AddToHistory(&history, commandInput);
            run_terminal_command(commandInput, terminalOutput, 4096);
            commandInput[0] = '\0';

            // Auto-scroll to bottom after command
            Vector2 newTextSize = MeasureTextEx(guiFont, terminalOutput, 16, 1);
            float newMaxScroll = newTextSize.y - outputRect.height;
            if (newMaxScroll > 0)
            {
                scrollOffset = newMaxScroll;
            }
        }
        commandEditMode = true;
    }

    // Clear button
    if (GuiButton((Rectangle){ res.width - 75, res.height - 60, 55, 30 }, "Clear")) {
        terminalOutput[0] = '\0';
        strcat(terminalOutput, "Terminal cleared.\n\n");
        scrollOffset = 0;
    }

}