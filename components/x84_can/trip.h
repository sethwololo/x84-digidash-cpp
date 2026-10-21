#pragma once

// Trip distance and fuel, summed from the rolling counters of 0x354 (distance) and 0x551 (fuel injected), the
// average consumption from both, and the reset by holding the trip computer button (0x60D "Trip computer scroll
// request").

#include <cstdint>
#include <optional>

namespace x84 {

class Trip {
public:
    // How long the button must be held to reset the trip.
    static constexpr uint32_t RESET_HOLD_MS = 2000;
    // One step of the fuel counter (0x551 Fuel consumption), and one turn of it (256 steps, 20.48 mL).
    static constexpr uint32_t FUEL_UNIT_MM3 = 80;
    static constexpr uint32_t FUEL_TURN_MM3 = 256 * FUEL_UNIT_MM3;

    Trip(uint32_t decimeters, uint32_t fuel_mm3) : decimeters_(decimeters), fuel_mm3_(fuel_mm3) {}

    uint32_t decimeters() const { return decimeters_; }
    uint32_t fuel_mm3() const { return fuel_mm3_; }

    // Average consumption since the reset, in tenths of km/L. Empty over the first 400 m, where it jumps around.
    std::optional<int> km_per_liter_tenths() const;

    // The latest value of the 0.1 m counter, or nothing when its frame is not valid.
    void update_distance(std::optional<uint16_t> counter);
    // The latest value of the fuel counter, or nothing when its frame is not valid.
    void update_fuel(std::optional<uint8_t> counter);
    // Whether the button is pressed (either direction), at `now_ms` on a monotonic clock. True once, when the trip
    // is reset: after the button has been held for RESET_HOLD_MS.
    bool update_button(bool pressed, uint32_t now_ms);

private:
    uint32_t decimeters_;
    uint32_t fuel_mm3_;
    std::optional<uint16_t> last_counter_;
    std::optional<uint8_t> last_fuel_counter_;
    std::optional<uint32_t> held_since_ms_;
    bool reset_done_ = false;
};

} // namespace x84
