#pragma once

// Left column: the tachometer (digits, bar and scale) and the row of readouts (gear, cruise, temperatures, battery).

#include "lvgl.h"
#include "screen.h"
#include "widgets/bars.h"
#include "widgets/decorations.h"
#include "widgets/readout.h"

namespace ui {

class Tachometer {
public:
    explicit Tachometer(lv_obj_t *screen);
    void show(const Screen &screen, bool blink_on);
    // Something here is in alarm: the red line, the engine temperature or the battery.
    bool alert(const Screen &screen) const;

private:
    lv_obj_t *rpm_;
    lv_obj_t *rpm_unit_;
    LevelBar bar_;
    AlertFrame redline_;
    Readout gear_;
    Readout cruise_;
    Readout engine_temp_;
    Readout battery_;
    Readout outside_temp_;
};

} // namespace ui
