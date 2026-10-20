#pragma once

// Center column: the speed, the turn signals, the trip and the odometer.

#include "lvgl.h"
#include "screen.h"
#include "widgets/indicators.h"

namespace ui {

class Speedometer {
public:
    explicit Speedometer(lv_obj_t *screen);
    void show(const Screen &screen);

private:
    TurnSignal left_;
    TurnSignal right_;
    lv_obj_t *speed_;
    lv_obj_t *trip_;
    lv_obj_t *odometer_;
};

} // namespace ui
