#pragma once

#include "UiShared.hpp"
#include <raylib.h>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

class FileBrowser {
public:
    enum class Mode {
        OPEN_FILE,
        SAVE_FILE,
        SELECT_DIRECTORY
    };

    FileBrowser(Mode mode = Mode::OPEN_FILE)
        : mode_(mode),
          is_open_(false),
          scroll_index_(0),
          selected_index_(-1) {
        current_path_ = fs::current_path();
        refresh_entries();
    }

    // Open the file browser
    void open() {
        is_open_ = true;
        refresh_entries();
    }

    // Close the file browser
    void close() {
        is_open_ = false;
    }

    void clear() {
        selected_path_.clear();
    }

    // Check if browser is open
    bool is_open() const {
        return is_open_;
    }

    // Get selected path (empty if none selected)
    std::string get_selected_path() const {
        return selected_path_;
    }

    // Render the file browser
    void render() {
        if (not is_open_) return;

        // Semi-transparent background overlay
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                     Fade(BLACK, 0.7f));

        // Browser window
        float width = 700;
        float height = 500;
        float x = (GetScreenWidth() - width) / 2;
        float y = (GetScreenHeight() - height) / 2;

        Rectangle browser_rect = { x, y, width, height };
        GuiPanel(browser_rect, "Select File");

        // Current path display
        float padding = 10;
        float line_height = 25;
        float y_offset = y + 30;

        GuiLabel(Rectangle{ x + padding, y_offset, 100, line_height },
                "Current Path:");

        // Path text box (read-only display)
        char path_buffer[512];
        strncpy(path_buffer, current_path_.string().c_str(), sizeof(path_buffer) - 1);
        path_buffer[sizeof(path_buffer) - 1] = '\0';

        GuiSetState(STATE_DISABLED);
        GuiTextBox(Rectangle{ x + padding + 110, y_offset, width - 130 - padding, line_height },
                  path_buffer, 512, false);
        GuiSetState(STATE_NORMAL);

        y_offset += line_height + 5;

        // Up directory button
        if (GuiButton(Rectangle{ x + padding, y_offset, 120, line_height },
                     GuiIconText(ICON_ARROW_UP, "Parent Dir"))) {
            go_up_directory();
        }

        y_offset += line_height + 5;

        // File list area
        float list_height = height - (y_offset - y) - 80;
        Rectangle list_rect = { x + padding, y_offset, width - 2 * padding, list_height };

        render_file_list(list_rect);

        y_offset += list_height + 5;

        // Selected file name input (for save mode)
        if (mode_ == Mode::SAVE_FILE) {
            GuiLabel(Rectangle{ x + padding, y_offset, 80, line_height }, "File Name:");

            static char filename_buffer[256] = "";
            if (GuiTextBox(Rectangle{ x + padding + 90, y_offset,
                                     width - 200 - padding, line_height },
                          filename_buffer, 256, true)) {
                // Text box clicked
            }

            y_offset += line_height + 5;
        }

        // Buttons
        float button_width = 100;
        float button_spacing = 10;
        float buttons_y = y + height - 40;

        // Cancel button
        if (GuiButton(Rectangle{ x + width - 2 * button_width - button_spacing - padding,
                                buttons_y, button_width, 30 }, "Cancel")) {
            close();
        }

        // Select/Open button
        const char* button_text = (mode_ == Mode::SELECT_DIRECTORY) ? "Select" :
                                 (mode_ == Mode::SAVE_FILE) ? "Save" : "Open";

        if (GuiButton(Rectangle{ x + width - button_width - padding,
                                buttons_y, button_width, 30 }, button_text)) {
            if (selected_index_ >= 0 && selected_index_ < static_cast<int>(entries_.size())) {
                auto& entry = entries_[selected_index_];

                if (entry.is_directory && mode_ != Mode::SELECT_DIRECTORY) {
                    // Navigate into directory
                    current_path_ = entry.path;
                    refresh_entries();
                } else {
                    // Select this entry
                    selected_path_ = entry.path.string();
                    close();
                }
            } else if (mode_ == Mode::SELECT_DIRECTORY) {
                // Select current directory
                selected_path_ = current_path_.string();
                close();
            }
        }
    }

private:
    struct Entry {
        fs::path path;
        std::string name;
        bool is_directory;
        size_t size;
    };

    void refresh_entries() {
        entries_.clear();
        selected_index_ = -1;

        try {
            for (const auto& entry : fs::directory_iterator(current_path_)) {
                Entry e;
                e.path = entry.path();
                e.name = entry.path().filename().string();
                e.is_directory = entry.is_directory();
                e.size = e.is_directory ? 0 :
                        (entry.is_regular_file() ? entry.file_size() : 0);

                entries_.push_back(e);
            }

            // Sort: directories first, then alphabetically
            std::sort(entries_.begin(), entries_.end(),
                     [](const Entry& a, const Entry& b) {
                if (a.is_directory != b.is_directory)
                    return a.is_directory;
                return a.name < b.name;
            });

        } catch (const fs::filesystem_error& e) {
            // Handle permission errors, etc.
        }
    }

    void go_up_directory() {
        if (current_path_.has_parent_path()) {
            current_path_ = current_path_.parent_path();
            refresh_entries();
        }
    }

    void render_file_list(Rectangle rect) {
        const float item_height = 25;
        const int visible_items = static_cast<int>(rect.height / item_height);

        // Scrollbar
        bool need_scrollbar = static_cast<int>(entries_.size()) > visible_items;
        float content_width = need_scrollbar ? rect.width - 20 : rect.width;

        if (need_scrollbar) {
            Rectangle scrollbar_rect = {
                rect.x + rect.width - 15,
                rect.y,
                15,
                rect.height
            };

            int max_scroll = std::max(0, static_cast<int>(entries_.size()) - visible_items);
            scroll_index_ = ui_scroll_bar(scrollbar_rect, scroll_index_, 0, max_scroll);
        }

        // Draw file list with clipping
        BeginScissorMode(rect.x, rect.y, rect.width, rect.height);

        for (int i = scroll_index_; i < static_cast<int>(entries_.size()) &&
             i < scroll_index_ + visible_items; i++) {
            float item_y = rect.y + (i - scroll_index_) * item_height;
            Rectangle item_rect = { rect.x, item_y, content_width, item_height };

            // Check if mouse is hovering
            Vector2 mouse = GetMousePosition();
            bool is_hovered = CheckCollisionPointRec(mouse, item_rect);
            bool is_selected = (i == selected_index_);

            // Background
            Color bg_color = is_selected ? ColorAlpha(SKYBLUE, 0.5f) :
                           is_hovered ? ColorAlpha(LIGHTGRAY, 0.3f) :
                           BLANK;
            DrawRectangleRec(item_rect, bg_color);

            // Icon and text
            auto& entry = entries_[i];
            const char* icon = entry.is_directory ? "#1#" : "#7#";  // Folder/File icons

            std::string display_text = std::string(icon) + " " + entry.name;
            if (!entry.is_directory && entry.size > 0) {
                display_text += " (" + format_size(entry.size) + ")";
            }

            GuiLabel(Rectangle{ rect.x + 5, item_y, content_width - 10, item_height },
                    display_text.c_str());

            // Handle click
            if (is_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (selected_index_ == i) {
                    // Double click - open directory or select file
                    if (entry.is_directory) {
                        current_path_ = entry.path;
                        refresh_entries();
                    } else {
                        selected_path_ = entry.path.string();
                        close();
                    }
                } else {
                    selected_index_ = i;
                }
            }
        }

        // Scroll with mouse wheel
        float wheel = GetMouseWheelMove();
        if (wheel != 0 && CheckCollisionPointRec(GetMousePosition(), rect)) {
            scroll_index_ = std::max(0, std::min(
                (int)entries_.size() - visible_items,
                scroll_index_ - (int)wheel * 3
            ));
        }

        EndScissorMode();
    }

    std::string format_size(size_t bytes) const {
        const char* units[] = { "B", "KB", "MB", "GB" };
        int unit_index = 0;
        double size = bytes;

        while (size >= 1024 && unit_index < 3) {
            size /= 1024;
            unit_index++;
        }

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit_index]);
        return buffer;
    }

    Mode mode_;
    bool is_open_;
    fs::path current_path_;
    std::vector<Entry> entries_;
    int scroll_index_;
    int selected_index_;
    std::string selected_path_;
};