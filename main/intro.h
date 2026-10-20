#pragma once

// Start-up animation: the backlight fades in from black once the first frame is on screen. It costs nothing to
// render, since it is the panel's backlight and not the picture that changes. The backlight goes through I2C, so
// the fade moves in 20 ms steps instead of on every update; quadratic, because brightness looks more even to the
// eye than a linear ramp.

#include <cstdint>
#include <optional>

class Intro {
public:
    // Starts the fade. Call once the first frame is on screen, so the fade shows a finished picture.
    void start(uint32_t now_ms);

    // The backlight to use now, given the one asked for once the fade is over.
    int backlight(int target, uint32_t now_ms) const;

private:
    std::optional<uint32_t> started_ms_;
};
