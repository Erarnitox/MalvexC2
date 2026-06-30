#include <raylib.h>
#include "UiLayout.hpp"
#include "UiShared.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnarrowing"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#endif

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_STYLE_DARK
#include <raygui.h>
#include <styles/dark/style_dark.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

void apply_malvex_theme() {
    GuiLoadStyleDark();

    const int text_padding = 10;
    const int border_width = 1;

    const int controls[] = {
        DEFAULT, LABEL, BUTTON, TOGGLE, SLIDER, PROGRESSBAR, CHECKBOX,
        COMBOBOX, DROPDOWNBOX, TEXTBOX, VALUEBOX, LISTVIEW, COLORPICKER,
        SCROLLBAR, STATUSBAR,
    };

    for (const int control : controls) {
        GuiSetStyle(control, TEXT_PADDING, text_padding);
        GuiSetStyle(control, BORDER_WIDTH, border_width);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, 4);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt({28, 28, 34, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt({42, 42, 52, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt({56, 56, 70, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt({72, 72, 88, 255}));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt({90, 90, 110, 255}));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt({180, 60, 90, 255}));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt({70, 70, 86, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt({230, 230, 240, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt({255, 255, 255, 255}));

    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(STATUSBAR, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    // Bright neon-green text for typed input (login, settings, terminal command, etc.)
    using ui::theme::kInputText;
    using ui::theme::kInputTextFocused;
    using ui::theme::kInputTextActive;
    using ui::theme::kInputTextDisabled;
    using ui::theme::kInputBorderFocused;

    GuiSetStyle(TEXTBOX, TEXT_COLOR_NORMAL, ColorToInt(kInputText));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_FOCUSED, ColorToInt(kInputTextFocused));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED, ColorToInt(kInputTextActive));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_DISABLED, ColorToInt(kInputTextDisabled));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_FOCUSED, ColorToInt(kInputBorderFocused));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, ColorToInt(kInputBorderFocused));
}

void draw_panel_title(Rectangle bounds, const char* title) {
    if (title != nullptr && title[0] != '\0') {
        GuiLabel({bounds.x + ui::kPadding, bounds.y + 8.0f, bounds.width - ui::kPadding * 2.0f, 22.0f}, title);
    }
}

void draw_malvex_panel(Rectangle bounds, const char* title, Color fill, Color border) {
    ui::draw_panel_frame(bounds, fill, border);
    draw_panel_title(bounds, title);
}

void draw_malvex_section_header(Rectangle content, const char* title) {
    GuiLabel({content.x, content.y + 4.0f, content.width, 24.0f}, title);
    ui::draw_section_divider(content);
}

int ui_scroll_bar(Rectangle bounds, int value, int minValue, int maxValue) {
    return GuiScrollBar(bounds, value, minValue, maxValue);
}
