#include "dashboard.h"

#include "theme.h"

namespace ui {

namespace {

lv_obj_t *prepare_screen()
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, theme::BACKGROUND, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    return screen;
}

} // namespace

Dashboard::Dashboard()
    : frame_(prepare_screen()), tachometer_(lv_screen_active()), speedometer_(lv_screen_active()),
      telemetry_(lv_screen_active())
{
}

void Dashboard::show(const Screen &screen)
{
    // Every blinking lamp or value follows the same clock, so they blink together.
    const bool blink_on = (lv_tick_get() / theme::BLINK_MS) % 2 == 0;

    const x84::Lamps &lamps = screen.view.lamps;
    const bool alert = tachometer_.alert(screen) || telemetry_.alert(screen) || lamps.stop != x84::Lamp::off ||
                       lamps.oil_pressure != x84::Lamp::off || lamps.engine != x84::Lamp::off ||
                       lamps.battery != x84::Lamp::off;

    frame_.show(screen, alert);
    tachometer_.show(screen, blink_on);
    speedometer_.show(screen);
    telemetry_.show(screen, blink_on);
}

} // namespace ui
