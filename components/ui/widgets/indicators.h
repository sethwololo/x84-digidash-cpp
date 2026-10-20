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

// A door (or the trunk lid) drawn as a thick line over the car: along the body when closed, swung out on its hinge
// and in the fault color when open.
class Door {
public:
    // `closed` and `open` are the line's two points in each state, in the car drawing's coordinates.
    Door(lv_obj_t *parent, lv_point_precise_t closed_from, lv_point_precise_t closed_to, lv_point_precise_t open_from,
         lv_point_precise_t open_to);
    // The line points into this object: it cannot be copied.
    Door(const Door &) = delete;
    Door &operator=(const Door &) = delete;
    void set(bool open);

private:
    lv_obj_t *line_;
    lv_point_precise_t closed_[2];
    lv_point_precise_t open_[2];
    bool is_open_ = false;
};

// Top view of the car (images/img_car_top, from assets/images/car-top.svg) in a 178x178 box, with its doors.
class DoorDiagram {
public:
    DoorDiagram(lv_obj_t *parent, int32_t x, int32_t y);
    void set(const x84::Doors &doors);

private:
    lv_obj_t *box_;
    Door front_left_;
    Door front_right_;
    Door rear_left_;
    Door rear_right_;
    Door trunk_;
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
