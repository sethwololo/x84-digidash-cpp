#pragma once

// Warning lights, the door diagram and the turn signal arrows.

#include "lvgl.h"
#include "view.h"

namespace ui {

// Warning light: dim outline and text when off, in its color when on. A blinking lamp alternates between the two.
class IconChip {
public:
    IconChip(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, const char *label);
    // `blink_on`: the current half of the blink cycle, shared by every blinking widget (see Dashboard).
    void set(x84::Lamp lamp, lv_color_t color, bool blink_on);

private:
    lv_obj_t *frame_;
    lv_obj_t *label_;
};

// Top view of the car in a 178x178 box; open doors and trunk light up.
class DoorDiagram {
public:
    DoorDiagram(lv_obj_t *parent, int32_t x, int32_t y);
    void set(const x84::Doors &doors);

private:
    lv_obj_t *front_left_;
    lv_obj_t *front_right_;
    lv_obj_t *rear_left_;
    lv_obj_t *rear_right_;
    lv_obj_t *trunk_;
};

// Turn signal arrow, 36x36.
class TurnSignal {
public:
    TurnSignal(lv_obj_t *parent, int32_t x, int32_t y, bool right);
    void set(bool lit);

private:
    lv_obj_t *arrow_;
};

} // namespace ui
