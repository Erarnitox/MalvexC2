#include <raylib.h>
#include <raygui.h>

#define RAYGUI_STYLE_DARK
#include <styles/dark/style_dark.h>

int main() {
    int oldW = 1200;
    int oldH = 800;
    bool fullscreen = false;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(oldW, oldH, "Malvex C2 - GUI Client");
    SetTargetFPS(30);

    // Layout state
    int currentTab = 0;
    constexpr int tabCount = 5;
    const char* tabs[] = { "Connections", "Logs", "Settings", "Builder", "Terminal" };

    GuiLoadStyleDark();
    auto bg_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));

    Vector2 scroll = {0, 0};
    Rectangle view = {0, 0, 0, 0};
    Rectangle content = {0, 0, 0, 30 * 25};

    while (not WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(bg_color);

        // --- Menu Bar ---
        GuiDummyRec({0, 0, (float)GetScreenWidth(), 24}, nullptr); // Background
        GuiButton({8, 4, 40, 16}, "File");
        GuiButton({60, 4, 90, 16}, "Preferences");
        GuiButton({160, 4, 70, 16}, "Window");
        GuiButton({240, 4, 40, 16}, "Help");

        if (GuiButton({(float)GetScreenWidth() - 20, 4, 16, 16,}, "X")) {
            CloseWindow();
        }

        if (GuiButton({(float)GetScreenWidth() - 120, 4, 90, 16,}, "Fullscreen")) {
            fullscreen = not fullscreen;

            if (fullscreen) {
                int m = GetCurrentMonitor();
                oldW = GetScreenWidth();
                oldH = GetScreenHeight();
                int newW = GetMonitorWidth(m);
                int newH = GetMonitorHeight(m);

                ToggleFullscreen();
                SetWindowSize(newW, newH);
            } else {
                ToggleFullscreen();
                SetWindowSize(oldW, oldH);
            }
        }

        // --- Tab Bar ---
        Rectangle tabBar = {0, 28, (float)GetScreenWidth(), 28};
        GuiGroupBox(tabBar, nullptr);

        float tabWidth = tabBar.width / tabCount;
        for (int i = 0; i < tabCount; i++) {
            Rectangle r{ tabWidth * i, 28, tabWidth, 28 };
            if (GuiButton(r, tabs[i]))
                currentTab = i;
        }

        // --- Victim Table Area ---
        if (currentTab == 0) {
            GuiLabel({10, 50, 300, 30}, "List of Victim Connections");
            Rectangle tableRect{0, 80, (float)GetScreenWidth(), (float)GetScreenHeight() - 90};
            GuiPanel(tableRect, "");

            // Table header
            const char* headers[] = {
                "ID", "WAN", "LAN", "Con. Type", "Computer",
                "User Name", "Acc. Type", "OS", "CPU", "RAM"
            };
            constexpr int colCount = 10;

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
                (float)GetScreenWidth(),
                (float)GetScreenHeight() - panelTop
            };
            GuiScrollPanel(panelRect, nullptr, content, &scroll, &view);

            BeginScissorMode(panelRect.x, panelRect.y, panelRect.width, panelRect.height);

            int clients = 30;
            colWidth = (tableRect.width-10) / colCount;
            for (int client_id = 0; client_id < clients; ++client_id) {
                auto line_color = client_id % 2 == 0 ? DARKGRAY : DARKPURPLE;

                for (int i = 0; i < colCount; i++) {
                    Rectangle r{tableRect.x + i * colWidth, tableRect.y + scroll.y + 26 + (25*client_id), colWidth, 24};
                    GuiDrawRectangle(r, 1, BLACK, line_color);
                    GuiLabel(r, headers[i]);
                }
            }

            EndScissorMode();
        }

        // --- Status Bar ---
        Rectangle status = {0, (float)GetScreenHeight() - 24, (float)GetScreenWidth(), 24};
        GuiStatusBar(status, "Version: 1.0 | Connections: 0 | Ports: 1");

        EndDrawing();
    }

    CloseWindow();
    return 0;
}