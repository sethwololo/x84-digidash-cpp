#pragma once

// Right column: fuel, oil level, the doors and the warning lights.

#include "lvgl.h"
#include "screen.h"
#include "widgets/bars.h"
#include "widgets/decorations.h"
#include "widgets/indicators.h"

namespace ui {

class Telemetry {
public:
    explicit Telemetry(lv_obj_t *screen);
    void show(const Screen &screen, bool blink_on);
    // Something here is in alarm: the fuel reserve, an open door or the oil level.
    bool alert(const Screen &screen) const;

private:
    BarHeader fuel_header_;
    LevelBar fuel_bar_;
    AlertFrame reserve_;
    BarHeader oil_header_;
    SegmentBar oil_bar_;
    DoorDiagram doors_;
    // The warning lights, row by row.
    IconChip headlights_, seat_belt_, front_fog_, rear_fog_;
    IconChip brake_, oil_pressure_, engine_, battery_;
    IconChip abs_, stop_, service_, airbag_;
};

} // namespace ui
