#include "tachometer.h"

#include <string>

#include "base.h"
#include "format.h"
#include "theme.h"

namespace ui {

namespace {

constexpr int32_t X = theme::MARGIN;
constexpr int32_t Y = theme::CONTENT_TOP;
// Up to the screen margin on the left; a gap before the rule with the speed on the right.
constexpr int32_t WIDTH = theme::SIDE_WIDTH - 25;

// The two blocks are spread with equal space above, between and below them.
constexpr int32_t GAUGE_HEIGHT = 138;
constexpr int32_t READOUTS_HEIGHT = 112;
constexpr int32_t GAP = (theme::CONTENT_HEIGHT - GAUGE_HEIGHT - READOUTS_HEIGHT) / 3;
constexpr int32_t GAUGE_Y = Y + GAP;   // top of the RPM digits
constexpr int32_t READOUTS_Y = GAUGE_Y + GAUGE_HEIGHT + GAP;

constexpr int32_t RPM_SIZE = 80;
// The top of the digits' text box: the digits themselves start at GAUGE_Y.
constexpr int32_t RPM_TOP = GAUGE_Y - static_cast<int32_t>(theme::DIGIT_TOP * RPM_SIZE);
constexpr int32_t BAR_Y = GAUGE_Y + 82;
constexpr int32_t BAR_HEIGHT = 26;
// The readouts' boxes: 5 of them, the first and last flush with the column, 22 px below the block's top (their
// captions go above).
constexpr int32_t BOX_Y = READOUTS_Y + 22;
constexpr int32_t BOX_STEP = (WIDTH - 90) / 4;

// Where "RPM" goes: right after the last digit (the font is monospaced, 0.6 em per glyph), on their baseline.
int32_t unit_x(size_t digits)
{
    return X + static_cast<int32_t>(digits * 0.6f * RPM_SIZE) + 12;
}

} // namespace

Tachometer::Tachometer(lv_obj_t *screen)
    : rpm_(text(screen, theme::TEXT_RPM, theme::VALUE, X, RPM_TOP, "--")),
      // On the digits' baseline.
      rpm_unit_(text(screen, theme::TEXT_VALUE, theme::LABEL, unit_x(2),
                     RPM_TOP + static_cast<int32_t>(theme::BASELINE * (RPM_SIZE - theme::TEXT_VALUE.size)), "RPM")),
      bar_(screen, X, BAR_Y, WIDTH, BAR_HEIGHT),
      // Around the digits, the bar and the scale under it.
      redline_(screen, X - 8, GAUGE_Y - 12, WIDTH + 16, 156),
      gear_(screen, X, BOX_Y, theme::TEXT_GEAR),
      cruise_(screen, X + BOX_STEP, BOX_Y, theme::TEXT_READOUT),
      engine_temp_(screen, X + 2 * BOX_STEP, BOX_Y, theme::TEXT_READOUT),
      battery_(screen, X + 3 * BOX_STEP, BOX_Y, theme::TEXT_VALUE_BOLD),
      outside_temp_(screen, X + 4 * BOX_STEP, BOX_Y, theme::TEXT_READOUT)
{
    scale(screen, X, BAR_Y + BAR_HEIGHT + 12, WIDTH, {"0", "1", "2", "3", "4", "5", "6", "7"});
    // Rule between this column and the speed.
    fill(screen, X + theme::SIDE_WIDTH - 1, Y, 1, theme::CONTENT_HEIGHT, theme::BORDER);

    gear_.set_label("MARCHA");
    engine_temp_.set_label("TMP MOTOR");
    battery_.set_label("BATERIA");
    outside_temp_.set_label("TMP EXT");
}

bool Tachometer::alert(const Screen &screen) const
{
    const x84::View &view = screen.view;
    const bool redline = view.rpm && *view.rpm >= theme::RPM_REDLINE;
    return redline || view.coolant_temp_alert || view.battery_low;
}

void Tachometer::show(const Screen &screen, bool blink_on)
{
    const x84::View &view = screen.view;

    const bool redline = view.rpm && *view.rpm >= theme::RPM_REDLINE;
    const std::string rpm = reading(view.rpm);
    set_text(rpm_, rpm.c_str());
    set_text_color(rpm_, redline ? theme::FAULT : theme::VALUE);
    set_x(rpm_unit_, unit_x(rpm.size()));
    bar_.set(view.rpm ? static_cast<float>(*view.rpm) / theme::RPM_MAX : 0.0f, redline);
    redline_.show(redline);

    const auto &gear = view.gear;
    gear_.set(!gear ? "-" : *gear == x84::NEUTRAL ? "N" : *gear == x84::REVERSE ? "R" : std::to_string(*gear).c_str());

    const x84::Cruise &cruise = view.cruise;
    cruise_.set_label(cruise.mode == x84::CruiseMode::limiter     ? "LIMITADOR"
                      : cruise.mode == x84::CruiseMode::regulator ? "REGULADOR"
                                                                  : "REG/LIM");
    // "---" only once the memory is really cleared (reset button); suspended shows the number, inactive.
    const Readout::State cruise_state = cruise.fault                              ? Readout::State::fault
                                        : cruise.set_speed_kmh && !cruise.active ? Readout::State::inactive
                                                                                 : Readout::State::normal;
    cruise_.set(cruise.set_speed_kmh ? std::to_string(*cruise.set_speed_kmh).c_str() : "---", cruise_state,
                cruise.blink && !blink_on);

    engine_temp_.set(reading(view.coolant_temp_c, "°").c_str(),
                     view.coolant_temp_alert ? Readout::State::fault : Readout::State::normal);
    battery_.set(view.battery_decivolts ? (tenths(*view.battery_decivolts) + "V").c_str() : "--",
                 view.battery_low ? Readout::State::fault : Readout::State::normal);
    outside_temp_.set(reading(view.outside_temp_c, "°").c_str());
}

} // namespace ui
