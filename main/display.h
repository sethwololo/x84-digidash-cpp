#pragma once

// The Waveshare 8.8" DSI panel (480x1920, portrait) driven by LVGL as a 1920x480 landscape screen.

#include <cstdint>

#include "lvgl.h"

namespace display {

constexpr int32_t WIDTH = 1920;
constexpr int32_t HEIGHT = 480;

// Brings up the panel and starts LVGL's task. The backlight stays off: see `set_brightness`.
lv_display_t *start();

// 0 to 100 %.
void set_brightness(int percent);

// LVGL is not thread safe: hold this lock around any lv_* call made outside LVGL's own task (its timers and
// event callbacks already run with it held).
class Lock {
public:
    Lock();
    ~Lock();
    Lock(const Lock &) = delete;
    Lock &operator=(const Lock &) = delete;
};

} // namespace display
