#pragma once

// Numbers as the dashboard writes them: a decimal comma and dots between thousands, "--" for a missing value.

#include <optional>
#include <string>

namespace ui {

// 126 -> "12,6": values from the car come in integer tenths.
std::string tenths(int value);

// 187412 -> "187.412".
std::string thousands(long value);

// The value followed by `suffix` ("89" + "°"), or "--" when it is missing.
std::string reading(const std::optional<int> &value, const char *suffix = "");

} // namespace ui
