#include "UiShared.hpp"

void drawAbout(WindowState& state) {
    static const Texture2D texture = LoadTexture("Resources/Logo.png");
    const Color panel_fill{36, 36, 46, 255};
    const Color panel_border{90, 90, 110, 255};

    Rectangle popupRect = ui::centered_popup(state.res, 640, 320);
    draw_malvex_panel(popupRect, GuiIconText(ICON_DEMON, "About MalvexC2"), panel_fill, panel_border);

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

    const char* popupText =
        "MalvexC2 was written by Erarnitox\n\n\n\n"
        "For Educational Purposes only!\n\n\n\n"
        "This should never be used for anything\n\n\n\n"
        "It is only an example Project!\n\n\n\n\n\n\n\n"
        "SUBSCRIBE TO ERARNITOX ON YOUTUBE!";

    GuiLabel({ popupRect.x + 270, popupRect.y + 300, 300, 30 }, popupText);

    if (GuiButton(Rectangle{ popupRect.x + 250, popupRect.y + popupRect.height - 50, 300, 30 }, "Close")) {
        state.show_about = false;
    }
}
