#pragma once

#include <raylib.h>

#include <algorithm>
#include <cmath>

//-------------------------------------------------
//
//-------------------------------------------------
namespace ui {

//-------------------------------------------------
//
//-------------------------------------------------
struct Resolution {
    float width;
    float height;
};

//-------------------------------------------------
//
//-------------------------------------------------
constexpr float kPadding = 16.0f;
constexpr float kGap = 12.0f;
constexpr float kCornerRadius = 0.08f;
constexpr float kCornerSegments = 10.0f;

constexpr float kTitleBarHeight = 36.0f;
constexpr float kTabBarHeight = 38.0f;
constexpr float kStatusBarHeight = 30.0f;
constexpr float kSectionHeaderHeight = 40.0f;
constexpr float kControlHeight = 30.0f;
constexpr float kLabelWidth = 120.0f;
constexpr float kFormRowGap = 12.0f;

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle inset(Rectangle rect, float padding) {
    return {
        rect.x + padding,
        rect.y + padding,
        std::max(0.0f, rect.width - padding * 2.0f),
        std::max(0.0f, rect.height - padding * 2.0f),
    };
}

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle content_area(const Resolution& res) {
    const float top = kTitleBarHeight + kGap + kTabBarHeight + kGap;
    const float bottom = kStatusBarHeight + kPadding;
    return {
        kPadding,
        top,
        std::max(0.0f, res.width - kPadding * 2.0f),
        std::max(0.0f, res.height - top - bottom),
    };
}

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle title_bar(const Resolution& res) {
    return {0.0f, 0.0f, res.width, kTitleBarHeight};
}

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle tab_bar(const Resolution& res) {
    return {
        kPadding,
        kTitleBarHeight + kGap,
        std::max(0.0f, res.width - kPadding * 2.0f),
        kTabBarHeight,
    };
}

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle status_bar(const Resolution& res) {
    return {
        kPadding,
        res.height - kStatusBarHeight - (kPadding * 0.5f),
        std::max(0.0f, res.width - kPadding * 2.0f),
        kStatusBarHeight,
    };
}

//-------------------------------------------------
//
//-------------------------------------------------
inline void draw_rounded_rect(Rectangle bounds, Color fill, Color border, float thickness = 1.0f) {
    DrawRectangleRounded(bounds, kCornerRadius, static_cast<int>(kCornerSegments), fill);
    if (thickness > 0.0f) {
        DrawRectangleRoundedLines(bounds, kCornerRadius, static_cast<int>(kCornerSegments), border);
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
inline void draw_panel_frame(Rectangle bounds, Color fill, Color border) {
    draw_rounded_rect(bounds, fill, border);
}

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle panel_body(Rectangle panel_bounds, bool has_title = true) {
    const float top = has_title ? 36.0f : kPadding;
    return inset({panel_bounds.x, panel_bounds.y + top, panel_bounds.width, panel_bounds.height - top}, kPadding);
}

//-------------------------------------------------
//
//-------------------------------------------------
inline void draw_section_divider(Rectangle content) {
    const Rectangle divider{
        content.x,
        content.y + kSectionHeaderHeight - 2.0f,
        content.width,
        1.0f,
    };
    DrawRectangleRec(divider, {70, 70, 86, 255});
}

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle section_content(Rectangle content) {
    return {
        content.x,
        content.y + kSectionHeaderHeight + kGap,
        content.width,
        std::max(0.0f, content.height - kSectionHeaderHeight - kGap),
    };
}

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle toolbar_button(float x, float y, float width) {
    return {x, y, width, kControlHeight};
}

//-------------------------------------------------
//
//-------------------------------------------------
struct FormLayout {
    Rectangle bounds;
    float cursor_y;
    float label_width;
    float field_x;
    float field_width;
    float row_height;
    float row_gap;

    explicit FormLayout(Rectangle area, float label_w = kLabelWidth)
        : bounds(area),
          cursor_y(area.y),
          label_width(label_w),
          field_x(area.x + label_w + kGap),
          field_width(std::max(0.0f, area.width - label_w - kGap)),
          row_height(kControlHeight),
          row_gap(kFormRowGap) {}

    Rectangle label_rect() const {
        return {bounds.x, cursor_y + 5.0f, label_width, row_height};
    }

    Rectangle field_rect(float width_ratio = 1.0f) const {
        return {field_x, cursor_y, field_width * width_ratio, row_height};
    }

    void next_row() {
        cursor_y += row_height + row_gap;
    }
};

//-------------------------------------------------
//
//-------------------------------------------------
inline Rectangle centered_popup(const Resolution& res, float width, float height) {
    return {
        (res.width - width) * 0.5f,
        (res.height - height) * 0.5f,
        width,
        height,
    };
}

} // namespace ui
