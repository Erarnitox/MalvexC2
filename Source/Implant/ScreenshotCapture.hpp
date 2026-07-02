#pragma once

#include <optional>
#include <string>
#include <vector>

namespace screenshot_capture {

[[nodiscard]] std::optional<std::string> capture_base64_ppm();

} // namespace screenshot_capture
