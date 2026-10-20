#include "dashboard.h"
#include "display.h"
#include "simulation.h"

// How often the dashboard is updated with new data. LVGL draws at most every 15 ms (CONFIG_LV_DEF_REFR_PERIOD).
constexpr uint32_t UPDATE_MS = 10;

namespace {

ui::Dashboard *dashboard;
ui::Simulation simulation;

// Runs in LVGL's task, with the display lock held.
void update(lv_timer_t *)
{
    dashboard->show(simulation.screen(lv_tick_get()));
}

} // namespace

extern "C" void app_main()
{
    lv_display_t *screen = display::start();
    {
        display::Lock lock;
        dashboard = new ui::Dashboard();
        update(nullptr);
        lv_timer_create(update, UPDATE_MS, nullptr);
        display::log_frame_rate(screen);
    }
    display::set_brightness(85);
}
