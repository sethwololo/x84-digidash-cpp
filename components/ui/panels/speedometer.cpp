#include "speedometer.h"

#include <string>

#include "base.h"
#include "format.h"
#include "theme.h"

namespace ui {

namespace {

constexpr int32_t X = theme::MARGIN + theme::SIDE_WIDTH;
constexpr int32_t WIDTH = theme::CENTER_WIDTH;
constexpr int32_t HEIGHT = 332;
// Centered between the rules.
constexpr int32_t Y = theme::CONTENT_TOP + (theme::CONTENT_HEIGHT - HEIGHT) / 2;

constexpr int32_t SPEED_SIZE = 190;
// The digits start 34 px down, 0.16 em into their box.
constexpr int32_t DIGITS_TOP = Y + 34 + static_cast<int32_t>(0.16f * SPEED_SIZE);

// A text across the column, centered.
lv_obj_t *centered(lv_obj_t *screen, const theme::Font &font, lv_color_t color, int32_t y, const char *content = "")
{
    lv_obj_t *label = text(screen, font, color, X, y, content);
    lv_obj_set_width(label, WIDTH);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    return label;
}

} // namespace

Speedometer::Speedometer(lv_obj_t *screen)
    : left_(screen, X + 70, Y + 44, false),
      right_(screen, X + WIDTH - 70 - 36, Y + 44, true),
      speed_(centered(screen, theme::TEXT_SPEED, theme::VALUE, DIGITS_TOP - static_cast<int32_t>(theme::DIGIT_TOP * SPEED_SIZE), "--"))
{
    centered(screen, theme::TEXT_VALUE, theme::LABEL, Y + 236, "KM/H");

    // The trip between two short rules, the three centered as one row.
    lv_obj_t *row = box(screen, X, Y + 282, WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);
    fill(row, 0, 0, 28, 1, theme::BORDER);
    trip_ = text(row, theme::TEXT_LABEL, theme::LABEL, 0, 0);
    fill(row, 0, 0, 28, 1, theme::BORDER);

    odometer_ = centered(screen, theme::TEXT_LABEL, theme::LABEL, Y + 308);
}

void Speedometer::show(const Screen &screen)
{
    const x84::View &view = screen.view;
    left_.set(view.turn_left);
    right_.set(view.turn_right);
    set_text(speed_, reading(view.speed_kmh).c_str());

    const std::string km_per_liter = screen.trip_km_per_liter_tenths ? tenths(*screen.trip_km_per_liter_tenths) : "--";
    set_text(trip_, ("TRIP " + tenths(screen.trip_hectometers) + " KM · " + km_per_liter + " KM/L").c_str());
    set_text(odometer_, ("ODO " + (view.odometer_km ? thousands(*view.odometer_km) : std::string("--")) + " KM").c_str());
}

} // namespace ui
