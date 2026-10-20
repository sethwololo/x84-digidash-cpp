#include "format.h"

namespace ui {

std::string tenths(int value)
{
    return std::to_string(value / 10) + "," + std::to_string(value % 10);
}

std::string thousands(long value)
{
    const std::string digits = std::to_string(value);
    std::string text;
    for (size_t i = 0; i < digits.size(); i++) {
        if (i > 0 && (digits.size() - i) % 3 == 0) text += '.';
        text += digits[i];
    }
    return text;
}

std::string reading(const std::optional<int> &value, const char *suffix)
{
    return value ? std::to_string(*value) + suffix : "--";
}

} // namespace ui
