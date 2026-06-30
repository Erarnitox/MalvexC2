#include "UiShared.hpp"
#include "Util/SafeString.hpp"

#include <future>
#include <optional>

namespace {

std::optional<std::future<bool>> login_future;

} // namespace

void poll_login_async(WindowState& state) {
    if (!login_future.has_value()) {
        return;
    }

    if (login_future->wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return;
    }

    state.is_connected = login_future->get();
    if (not state.is_connected) {
        state.login_failed = true;
    }
    state.wait_for_response = false;
    login_future.reset();
}

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

        login_future = std::async(std::launch::async, [&client = state.client]() {
            return client.login();
        });
    }

    if (state.login_failed) {
        GuiLabel(Rectangle{form_x, popupRect.y + popupRect.height - 36.0f, form_w, 24.0f}, "Login Failed!");
    } else if (GuiButton(Rectangle{form_x, popupRect.y + popupRect.height - 36.0f, form_w, ui::kControlHeight}, "Start Local Server")) {
        static_cast<void>(malvex::safe_copy(state.user_settings.server_url.text, sizeof(state.user_settings.server_url.text), "https://127.0.0.1:1337"));
        state.client.setServerUrl(state.user_settings.server_url.text);
        state.client.setUsername(state.user_settings.username.text);
        state.client.setPassword(state.user_settings.password.text);

        std::system("./server --local");

        state.is_connected = true;
    }
}
