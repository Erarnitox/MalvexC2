#include "UiShared.hpp"
#include "FileBrowser.hpp"
#include "Util/SafeString.hpp"

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
        state.client.setTimeout(settings.default_timeout.text);
        state.client.setOutputPath(settings.output_file_path.text);
    }

    fb.render();

    if (not fb.is_open() && not fb.get_selected_path().empty()) {
        static_cast<void>(malvex::safe_copy(settings.output_file_path.text, sizeof(settings.output_file_path.text), fb.get_selected_path()));
        fb.clear();
    }
}
