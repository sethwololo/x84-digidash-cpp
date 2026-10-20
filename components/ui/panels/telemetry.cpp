#include "telemetry.h"

#include <string>

#include "base.h"
#include "theme.h"

namespace ui {

namespace {

constexpr int32_t COLUMN_X = theme::SCREEN_WIDTH - theme::MARGIN - theme::SIDE_WIDTH;
// A gap after the rule with the speed on the left; up to the screen margin on the right.
constexpr int32_t X = COLUMN_X + 25;
constexpr int32_t WIDTH = theme::SIDE_WIDTH - 25;

// Three blocks spread with equal space above, between and below them (4 gaps).
constexpr int32_t FUEL_HEIGHT = 66;
constexpr int32_t OIL_HEIGHT = 50;
constexpr int32_t OPENINGS_HEIGHT = 178;
constexpr int32_t GAP = (theme::CONTENT_HEIGHT - FUEL_HEIGHT - OIL_HEIGHT - OPENINGS_HEIGHT) / 4;
constexpr int32_t FUEL_Y = theme::CONTENT_TOP + GAP;
constexpr int32_t OIL_Y = FUEL_Y + FUEL_HEIGHT + GAP;
constexpr int32_t OPENINGS_Y = OIL_Y + OIL_HEIGHT + GAP;

// A block's header starts at the top of its glyphs, 6 px below the top of the text.
constexpr int32_t HEADER_OFFSET = -6;

// Warning lights: 4 columns of 68x54 chips, 8 px apart, flush right.
constexpr int32_t CHIP_WIDTH = 68;
constexpr int32_t CHIP_HEIGHT = 54;
constexpr int32_t CHIP_GAP = 8;
constexpr int32_t GRID_X = X + WIDTH - (4 * CHIP_WIDTH + 3 * CHIP_GAP);

int32_t chip_x(int column)
{
    return GRID_X + column * (CHIP_WIDTH + CHIP_GAP);
}

int32_t chip_y(int row)
{
    return OPENINGS_Y + row * (CHIP_HEIGHT + CHIP_GAP);
}

} // namespace

Telemetry::Telemetry(lv_obj_t *screen)
    : fuel_header_(screen, X, FUEL_Y + HEADER_OFFSET, WIDTH, "COMBUSTÍVEL"),
      fuel_bar_(screen, X, FUEL_Y + 24, WIDTH, 20),
      reserve_(screen, X - 8, FUEL_Y - 13, WIDTH + 16, 83, "RESERVA"),
      oil_header_(screen, X, OIL_Y + HEADER_OFFSET, WIDTH, "NÍVEL DO ÓLEO"),
      oil_bar_(screen, X, OIL_Y + 24, WIDTH, 18, x84::OIL_SEGMENTS),
      doors_(screen, X, OPENINGS_Y),
      headlights_(screen, chip_x(0), chip_y(0), CHIP_WIDTH, CHIP_HEIGHT, "FAROL"),
      seat_belt_(screen, chip_x(1), chip_y(0), CHIP_WIDTH, CHIP_HEIGHT, "CINTO"),
      front_fog_(screen, chip_x(2), chip_y(0), CHIP_WIDTH, CHIP_HEIGHT, "NEB-D"),
      rear_fog_(screen, chip_x(3), chip_y(0), CHIP_WIDTH, CHIP_HEIGHT, "NEB-T"),
      brake_(screen, chip_x(0), chip_y(1), CHIP_WIDTH, CHIP_HEIGHT, "FREIO"),
      oil_pressure_(screen, chip_x(1), chip_y(1), CHIP_WIDTH, CHIP_HEIGHT, "ÓLEO"),
      engine_(screen, chip_x(2), chip_y(1), CHIP_WIDTH, CHIP_HEIGHT, "MOTOR"),
      battery_(screen, chip_x(3), chip_y(1), CHIP_WIDTH, CHIP_HEIGHT, "BAT"),
      abs_(screen, chip_x(0), chip_y(2), CHIP_WIDTH, CHIP_HEIGHT, "ABS"),
      stop_(screen, chip_x(1), chip_y(2), CHIP_WIDTH, CHIP_HEIGHT, "STOP"),
      service_(screen, chip_x(2), chip_y(2), CHIP_WIDTH, CHIP_HEIGHT, "SVC"),
      airbag_(screen, chip_x(3), chip_y(2), CHIP_WIDTH, CHIP_HEIGHT, "AIRBAG")
{
    scale(screen, X, FUEL_Y + 48, WIDTH, {"V", "C"});
    // Rule between the speed and this column.
    fill(screen, COLUMN_X, theme::CONTENT_TOP, 1, theme::CONTENT_HEIGHT, theme::BORDER);
}

bool Telemetry::alert(const Screen &screen) const
{
    const x84::View &view = screen.view;
    return view.fuel_low || view.doors.any_open() || view.oil_alert;
}

void Telemetry::show(const Screen &screen, bool blink_on)
{
    const x84::View &view = screen.view;

    fuel_header_.set(view.fuel_liters ? (std::to_string(*view.fuel_liters) + " L").c_str() : "--", false);
    fuel_bar_.set(view.fuel_liters ? static_cast<float>(*view.fuel_liters) / theme::FUEL_CAPACITY : 0.0f, view.fuel_low);
    reserve_.show(view.fuel_low);

    // "alerte" is a known level of 0, with an empty bar.
    oil_header_.set(view.oil_level ? (std::to_string(*view.oil_level) + "/" + std::to_string(x84::OIL_SEGMENTS)).c_str() : "--",
                    view.oil_alert);
    oil_bar_.set(view.oil_level.value_or(0));

    doors_.set(view.doors);

    const x84::Lamps &lamps = view.lamps;
    // The headlight chip shows the brightest lights on: yellow, green or blue.
    headlights_.set(lamps.headlights == x84::Headlights::off ? x84::Lamp::off : x84::Lamp::on,
                    lamps.headlights == x84::Headlights::high_beam  ? theme::HIGH_BEAM
                    : lamps.headlights == x84::Headlights::low_beam ? theme::SIGNAL
                                                                    : theme::SERVICE,
                    blink_on);
    seat_belt_.set(lamps.seat_belt, theme::FAULT, blink_on);
    front_fog_.set(lamps.front_fog, theme::SIGNAL, blink_on);
    rear_fog_.set(lamps.rear_fog, theme::SIGNAL, blink_on);
    brake_.set(lamps.brake, theme::FAULT, blink_on);
    oil_pressure_.set(lamps.oil_pressure, theme::FAULT, blink_on);
    engine_.set(lamps.engine, theme::FAULT, blink_on);
    battery_.set(lamps.battery, theme::FAULT, blink_on);
    abs_.set(lamps.abs, theme::SERVICE, blink_on);
    stop_.set(lamps.stop, theme::FAULT, blink_on);
    service_.set(lamps.service, theme::SERVICE, blink_on);
    airbag_.set(lamps.airbag, theme::SERVICE, blink_on);
}

} // namespace ui
