#pragma once

#include "lvgl.h"

namespace benchmark {

// Builds the benchmark screen and logs the frame rate every second. Call with the display lock held.
void start(lv_display_t *display);

} // namespace benchmark
