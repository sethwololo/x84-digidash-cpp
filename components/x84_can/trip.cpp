#include "trip.h"

namespace x84 {

namespace {

// Largest believable step of the distance counter between two readings: 100 m, far more than the car covers in the
// 500 ms a frame stays valid. A bigger jump means the counter restarted (e.g. the ABS unit reset), and is not
// counted.
constexpr uint16_t MAX_STEP_DECIMETERS = 1000;
// Largest believable step of the fuel counter: half its range, 10 mL. Even at full load the engine burns a fraction
// of that in 500 ms; a bigger jump means the counter restarted.
constexpr uint8_t MAX_FUEL_STEP = 128;
// The average is only shown after this distance.
constexpr uint32_t MIN_AVERAGE_DECIMETERS = 10000;

} // namespace

std::optional<int> Trip::km_per_liter_tenths() const
{
    if (decimeters_ < MIN_AVERAGE_DECIMETERS || fuel_mm3_ == 0) return std::nullopt;
    // km / L = (dm / 10_000) / (mm³ / 1_000_000) = dm * 100 / mm³; in tenths, dm * 1000 / mm³.
    return static_cast<int>(static_cast<uint64_t>(decimeters_) * 1000 / fuel_mm3_);
}

void Trip::update_distance(std::optional<uint16_t> counter)
{
    if (counter && last_counter_) {
        // uint16_t arithmetic wraps around like the counter.
        const uint16_t step = static_cast<uint16_t>(*counter - *last_counter_);
        if (step <= MAX_STEP_DECIMETERS) decimeters_ += step;
    }
    last_counter_ = counter;
}

void Trip::update_fuel(std::optional<uint8_t> counter)
{
    if (counter && last_fuel_counter_) {
        const uint8_t step = static_cast<uint8_t>(*counter - *last_fuel_counter_);
        if (step <= MAX_FUEL_STEP) fuel_mm3_ += step * FUEL_UNIT_MM3;
    }
    last_fuel_counter_ = counter;
}

bool Trip::update_button(bool pressed, uint32_t now_ms)
{
    if (!pressed) {
        held_since_ms_.reset();
        reset_done_ = false;
        return false;
    }
    if (!held_since_ms_) held_since_ms_ = now_ms;
    if (!reset_done_ && now_ms - *held_since_ms_ >= RESET_HOLD_MS) {
        reset_done_ = true;
        decimeters_ = 0;
        fuel_mm3_ = 0;
        return true;
    }
    return false;
}

} // namespace x84
