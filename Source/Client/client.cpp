#include "ui_includes.hpp"
#include <raylib.h>

// function prototypes
void drawConnectionsTab(const Resolution& res);
void drawAbout(WindowState& state);

//-------------------------------------------------
//
//-------------------------------------------------
int main() {
    WindowState state{
        .show_about=false,
        .is_fullscreen=false,
        .res={ 1200, 800}
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
    const Font customFont = LoadFont("Font.ttf");
    GuiSetFont(customFont);

    // Layout state
    int currentTab = Tab::CONNECTIONS;

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
        if (state.show_about) {
            drawAbout(state);
        } else {
            // --- Tab Bar ---
            Rectangle tabBar = {0, 28, state.res.width, 28};
            GuiGroupBox(tabBar, nullptr);

            float tabWidth = tabBar.width / tabs.size();
            for (size_t i = 0; i < tabs.size(); i++) {
                Rectangle r{ tabWidth * i, 28, tabWidth, 28 };
                if (GuiButton(r, tabs.at(i).c_str())) {
                    currentTab = i;
                }
            }

            // --- Active Tab ---
            switch(currentTab) {
                case Tab::CONNECTIONS:
                    drawConnectionsTab(state.res);
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
void drawConnectionsTab(const Resolution& res) {
    static Vector2 scroll = {0, 0};
    static Rectangle view = {0, 0, 0, 0};
    static Rectangle content = {0, 0, 0, 30 * 25};
    static Vector2 menuPos = {};
    static bool menuVisible = false;
    static int selectedRow = -1;
    static int hoveredRow = -1;

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
        Rectangle menuRect{menuPos.x, menuPos.y, 300, 500};

        GuiPanel(menuRect, GuiIconText(ICON_DEMON, TextFormat("Attack Victim #%d", selectedRow)));

        Rectangle btn1{menuRect.x + 10, menuRect.y + 30, 280, 25};
        Rectangle btn2{menuRect.x + 10, menuRect.y + 60, 280, 25};

        if (GuiButton(btn1, TextFormat("Action 1 (Row %d)", selectedRow))) {
            // handle action
            menuVisible = false;
        }
        if (GuiButton(btn2, "Action 2")) {
            // handle action
            menuVisible = false;
        }
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