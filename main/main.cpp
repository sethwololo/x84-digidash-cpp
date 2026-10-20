// The dashboard: data from the car's CAN bus (or the simulation, CONFIG_X84_SIMULATION) shown on the display.
//
// Everything after the start runs in LVGL's task, on a timer: read the data, show it, follow the backlight.

#include "can.h"
#include "dashboard.h"
#include "display.h"
#include "esp_log.h"
#include "intro.h"
#include "nvs_flash.h"
#include "simulation.h"
#include "vehicle.h"

namespace {

// How often the dashboard gets new data. LVGL draws at most every 15 ms (CONFIG_LV_DEF_REFR_PERIOD); the CAN frames
// used come every 10 to 100 ms.
constexpr uint32_t UPDATE_MS = 10;

ui::Dashboard *dashboard;
Intro intro;
int backlight = -1;
#if CONFIG_X84_SIMULATION
ui::Simulation simulation;
#else
can::Bus bus;
Vehicle *vehicle;
#endif

ui::Screen current_screen(uint32_t now_ms)
{
#if CONFIG_X84_SIMULATION
    return simulation.screen(now_ms);
#else
    return vehicle->update(now_ms);
#endif
}

// Runs in LVGL's task, with the display lock held.
void update(lv_timer_t *)
{
    const uint32_t now = lv_tick_get();
    const ui::Screen screen = current_screen(now);
    dashboard->show(screen);

    const int wanted = intro.backlight(screen.view.backlight_percent, now);
    if (wanted != backlight) {
        backlight = wanted;
        display::set_brightness(backlight);
    }
}

// The backlight fades in once the first frame is on screen.
void on_first_frame(lv_event_t *)
{
    intro.start(lv_tick_get());
}

void init_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

} // namespace

extern "C" void app_main()
{
    init_nvs();
#if !CONFIG_X84_SIMULATION
    if (bus.start() != ESP_OK) ESP_LOGE("main", "CAN controller not started");
    vehicle = new Vehicle(bus);
#endif

    lv_display_t *screen = display::start();
    display::Lock lock;
    dashboard = new ui::Dashboard();
    update(nullptr);
    lv_timer_create(update, UPDATE_MS, nullptr);
    lv_display_add_event_cb(screen, on_first_frame, LV_EVENT_REFR_READY, nullptr);
    display::log_frame_rate(screen);
}
