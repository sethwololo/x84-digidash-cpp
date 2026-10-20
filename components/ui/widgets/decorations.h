#pragma once

// Corner brackets of the screen, and the red frame around a value in alarm.

#include "lvgl.h"

namespace ui {

// 24x24 bracket in a corner of the screen; `flip_x`/`flip_y` mirror the top-left one.
void corner(lv_obj_t *parent, int32_t x, int32_t y, bool flip_x, bool flip_y);

// Red frame with hazard stripes in two corners, and an optional label in the bottom right one. Hidden until shown.
class AlertFrame {
public:
    AlertFrame(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, const char *label = nullptr);
    // Shown over whatever it surrounds.
    void show(bool shown);

private:
    lv_obj_t *group_; // the frame and the stripes, which stick out of it
};

} // namespace ui
