#pragma once

// The dashboard: builds the screen and shows a `Screen` on it. The only way the rest of the firmware touches the UI.
//
//   theme.h       colors, type scale, scales of the gauges and the layout
//   base.h        building blocks: plain boxes, texts and images, setters that skip unchanged values
//   widgets/      reusable pieces (bars, readouts, warning lights, alert frames...)
//   panels/       the screen frame and the three columns, built from the widgets
//
// Every value is written on every update; the setters only make LVGL repaint what changed. By day (no low or high
// beam) the dashboard uses the light palette, at night the dark one: on a change it rebuilds itself in the new one.

#include <memory>

#include "lvgl.h"
#include "panels/frame.h"
#include "panels/speedometer.h"
#include "panels/tachometer.h"
#include "panels/telemetry.h"
#include "screen.h"

namespace ui {

class Dashboard {
public:
    // Builds the dashboard on LVGL's active screen. Call with the display lock held, like `show`.
    Dashboard();
    void show(const Screen &screen);

private:
    // Everything on the screen, built in the current palette.
    struct Content {
        explicit Content(lv_obj_t *screen);
        Frame frame;
        Tachometer tachometer;
        Speedometer speedometer;
        Telemetry telemetry;
    };

    void build();

    std::unique_ptr<Content> content_;
};

} // namespace ui
