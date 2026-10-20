#include "simulation.h"

#include <cmath>

namespace ui {

namespace {

constexpr uint32_t SCENARIO_MS = 12000;

// Per scenario: normal driving, high-RPM warning, low fuel.
constexpr int OUTSIDE_TEMP_C[] = {18, 24, 9};
constexpr int COOLANT_TEMP_C[] = {89, 118, 91};
constexpr int BATTERY_DECIVOLTS[] = {126, 138, 119};
constexpr int FUEL_LITERS[] = {31, 27, 4};

// A plausible gear for the 5-speed manual gearbox at this speed (km/h).
int gear_for(int speed)
{
    return speed <= 0 ? x84::NEUTRAL : speed <= 20 ? 1 : speed <= 40 ? 2 : speed <= 65 ? 3 : speed <= 95 ? 4 : 5;
}

x84::Lamp lamp(bool on)
{
    return on ? x84::Lamp::on : x84::Lamp::off;
}

} // namespace

Screen Simulation::screen(uint32_t now_ms)
{
    if (!start_ms_) {
        start_ms_ = now_ms;
        last_ms_ = now_ms;
    }
    const float t = pinned_ ? 0.0f : static_cast<float>(now_ms - *start_ms_) / 1000.0f;
    const float dt = static_cast<float>(now_ms - last_ms_) / 1000.0f;
    last_ms_ = now_ms;

    const float rpm = 3900.0f + 3100.0f * std::sin(t * 0.8f);
    const float speed = 100.0f + 90.0f * std::sin(t * 0.3f);
    trip_km_ += speed * dt / 3600.0f;

    const int scenario = pinned_ ? *pinned_ : static_cast<int>((now_ms - *start_ms_) / SCENARIO_MS) % SCENARIOS;
    const bool normal = scenario == 0;
    const bool warning = scenario == 1;
    const bool low_fuel = scenario == 2;

    Screen screen;
    x84::View &view = screen.view;
    view.online = true;
    // Not rounded like the car's: changing 3 or 4 digits every frame is the worst case to render.
    view.rpm = static_cast<int>(rpm);
    view.speed_kmh = static_cast<int>(speed);
    view.gear = gear_for(static_cast<int>(speed));
    view.outside_temp_c = OUTSIDE_TEMP_C[scenario];
    view.coolant_temp_c = COOLANT_TEMP_C[scenario];
    view.coolant_temp_alert = warning;
    view.battery_decivolts = BATTERY_DECIVOLTS[scenario];
    view.fuel_liters = FUEL_LITERS[scenario];
    view.fuel_low = low_fuel;
    view.oil_level = low_fuel ? 4 : 5;
    view.turn_right = warning;
    view.turn_left = low_fuel;
    if (warning) view.notice = x84::Notice::severe_injection_fault;
    if (low_fuel) view.notice = x84::Notice::card_battery_low;
    view.odometer_km = 187412;
    if (normal) {
        view.cruise.mode = x84::CruiseMode::limiter;
        view.cruise.set_speed_kmh = 118;
        view.cruise.active = true;
    }
    view.doors.rear_left = low_fuel;
    view.doors.trunk = low_fuel;

    x84::Lamps &lamps = view.lamps;
    lamps.headlights = normal ? x84::Headlights::low_beam : warning ? x84::Headlights::high_beam : x84::Headlights::position;
    // As the car does (view.cpp): night, with a dimmer backlight, with the low or high beam on.
    view.night = !low_fuel;
    view.backlight_percent = view.night ? 70 : 100;
    lamps.stop = lamp(warning);
    lamps.engine = lamp(warning);
    lamps.airbag = lamp(warning);
    lamps.oil_pressure = lamp(low_fuel);
    lamps.battery = lamp(low_fuel);
    lamps.service = lamp(low_fuel);
    lamps.seat_belt = warning ? x84::Lamp::blinking : x84::Lamp::off;
    lamps.front_fog = lamp(normal);

    screen.trip_hectometers = static_cast<int>(trip_km_ * 10.0f);
    screen.trip_km_per_liter_tenths = 124;
    // The bench: the bus is fine, no firmware version.
    screen.can_state = CanState::ok;
    screen.firmware_version = "SIM";
    return screen;
}

} // namespace ui
