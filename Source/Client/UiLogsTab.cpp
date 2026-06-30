#include "UiShared.hpp"
#include "LogManager.hpp"

void drawLogsTab(WindowState& state) {
    static const size_t MAX_LOG_SIZE{4096};
    static std::string logText = "Log started...\n";
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
            const auto newEntry = log + '\n';
            if (logText.size() + newEntry.size() < MAX_LOG_SIZE - 1) {
                logText += newEntry;
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
        logText.c_str(),
        Vector2{log_body.x + ui::kPadding, log_body.y + ui::kPadding + scrollOffset.y},
        16.0f,
        1.0f,
        RAYWHITE);

    EndScissorMode();
}
