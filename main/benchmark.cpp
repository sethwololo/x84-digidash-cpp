#include "benchmark.h"

#include <cmath>
#include <cstdio>

#include "esp_log.h"
#include "esp_timer.h"
#include "fonts.h"

// Phase 1 of the port: the worst case of the Rust dashboard's simulation, to compare the frame rate. RPM and speed
// sweep and change every frame (3 or 4 digits at a time), at their places on the real layout, with the RPM bar.
// The Rust/Slint version renders this in about 17 ms per frame (about 30 fps).

namespace benchmark {

namespace {

constexpr const char *TAG = "benchmark";

lv_obj_t *rpm_label;
lv_obj_t *rpm_bar;
lv_obj_t *speed_label;

// Time spent refreshing frames (render, rotation and copy to the panel), and their count, since the last report.
int64_t refresh_started_us;
int64_t refresh_total_us;
int frames;
int64_t report_us;

lv_obj_t *text(lv_obj_t *parent, const lv_font_t *font, lv_color_t color, int32_t x, int32_t y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

void update(lv_timer_t *)
{
    const float t = static_cast<float>(esp_timer_get_time()) / 1e6f;
    const int rpm = static_cast<int>(3900.0f + 3100.0f * std::sin(t * 0.8f));
    const int speed = static_cast<int>(100.0f + 90.0f * std::sin(t * 0.3f));
    lv_label_set_text_fmt(rpm_label, "%d", rpm);
    lv_bar_set_value(rpm_bar, rpm, LV_ANIM_OFF);
    lv_label_set_text_fmt(speed_label, "%d", speed);
}

void on_refresh(lv_event_t *event)
{
    const int64_t now = esp_timer_get_time();
    if (lv_event_get_code(event) == LV_EVENT_REFR_START) {
        refresh_started_us = now;
        return;
    }
    refresh_total_us += now - refresh_started_us;
    frames++;
    if (now - report_us >= 1000000) {
        ESP_LOGI(TAG, "%d fps | refresh %.1f ms/frame", frames,
                 frames ? static_cast<double>(refresh_total_us) / frames / 1000.0 : 0.0);
        frames = 0;
        refresh_total_us = 0;
        report_us = now;
    }
}

} // namespace

void start(lv_display_t *display)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Positions of the Rust layout: the RPM digits start 25 px above the gauge (y 97), the speed is centered in
    // the 800 px middle column.
    rpm_label = text(screen, &plex_bold_80, lv_color_white(), 40, 72);
    text(screen, &plex_regular_18, lv_color_hex(0x9a9a9a), 244, 135);
    lv_label_set_text(lv_obj_get_child(screen, 1), "RPM");

    rpm_bar = lv_bar_create(screen);
    lv_obj_set_pos(rpm_bar, 40, 179);
    lv_obj_set_size(rpm_bar, 495, 26);
    lv_bar_set_range(rpm_bar, 0, 7000);
    lv_obj_set_style_radius(rpm_bar, 0, 0);
    lv_obj_set_style_radius(rpm_bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(rpm_bar, lv_color_black(), 0);
    lv_obj_set_style_border_width(rpm_bar, 1, 0);
    lv_obj_set_style_border_color(rpm_bar, lv_color_hex(0x5a5a5a), 0);
    lv_obj_set_style_pad_all(rpm_bar, 1, 0);
    lv_obj_set_style_bg_color(rpm_bar, lv_color_white(), LV_PART_INDICATOR);

    speed_label = text(screen, &plex_bold_190, lv_color_white(), 560, 79);
    lv_obj_set_width(speed_label, 800);
    lv_obj_set_style_text_align(speed_label, LV_TEXT_ALIGN_CENTER, 0);

    update(nullptr);
    lv_timer_create(update, 1, nullptr);
    lv_display_add_event_cb(display, on_refresh, LV_EVENT_REFR_START, nullptr);
    lv_display_add_event_cb(display, on_refresh, LV_EVENT_REFR_READY, nullptr);
}

} // namespace benchmark
