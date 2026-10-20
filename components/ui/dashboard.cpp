#include "dashboard.h"

#include "theme.h"

namespace ui {

Dashboard::Content::Content(lv_obj_t *screen)
    : frame(screen), tachometer(screen), speedometer(screen), telemetry(screen)
{
}

Dashboard::Dashboard()
{
    build();
}

void Dashboard::build()
{
    lv_obj_t *screen = lv_screen_active();
    content_.reset();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, theme::palette().background, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    content_ = std::make_unique<Content>(screen);
}

void Dashboard::show(const Screen &screen)
{
    const theme::Palette &wanted = screen.view.night ? theme::DARK : theme::LIGHT;
    if (theme::current != &wanted) {
        theme::current = &wanted;
        build();
    }

    // Every blinking lamp or value follows the same clock, so they blink together.
    const bool blink_on = (lv_tick_get() / theme::BLINK_MS) % 2 == 0;

    const x84::Lamps &lamps = screen.view.lamps;
    const bool alert = content_->tachometer.alert(screen) || content_->telemetry.alert(screen) ||
                       lamps.stop != x84::Lamp::off || lamps.oil_pressure != x84::Lamp::off ||
                       lamps.engine != x84::Lamp::off || lamps.battery != x84::Lamp::off;

    content_->frame.show(screen, alert);
    content_->tachometer.show(screen, blink_on);
    content_->speedometer.show(screen);
    content_->telemetry.show(screen, blink_on);
}

} // namespace ui
