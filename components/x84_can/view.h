#pragma once

// What the dashboard shows, derived from the car's CAN frames (view.cpp decides it). Plain C++, without ESP-IDF
// or LVGL, so it can be tested on the computer. A value the car did not send is an empty std::optional: the
// screen shows "--".

#include <cstdint>
#include <optional>

namespace x84 {

enum class Lamp { off, on, blinking };

// The headlight chip shows the brightest lights on.
enum class Headlights { off, position, low_beam, high_beam };

enum class CruiseMode { none, limiter, regulator };

// Gears are numbers: 1 to 5, and these two.
constexpr int NEUTRAL = 0;
constexpr int REVERSE = -1;

// Oil level segments on the dashboard. The signal goes up to 8; the car shows 6.
constexpr int OIL_SEGMENTS = 6;

// A message in the header, in place of the system status: what the original cluster's display shows (0x60D
// Affichage informations VSC, mostly about the keyless card), then a few faults.
enum class Notice {
    welcome,
    insert_card,
    card_not_detected,
    hands_free_fault,
    press_stop_twice,
    card_reader_fault,
    brake_and_start,
    remove_card,
    restricted_card_mode,
    brake_and_start_or_clutch,
    neutral_or_clutch,
    trunk_open_insert_card,
    confirm_engine_stop,
    starting,
    pedal_not_held,
    card_battery_low,
    column_not_locked,
    column_not_unlocked,
    child_lock_fault,
    wiper_fault,
    injection_fault,
    severe_injection_fault,
};

// How a notice is shown: an instruction or information, a fault that does not stop the car (SERVICE color), or
// one that does (fault color).
enum class Severity { info, service, fault };

Severity severity(Notice notice);

struct Doors {
    bool front_left = false;
    bool front_right = false;
    bool rear_left = false;
    bool rear_right = false;
    bool trunk = false;

    bool any_open() const { return front_left || front_right || rear_left || rear_right || trunk; }
};

struct Cruise {
    CruiseMode mode = CruiseMode::none;
    // The set speed, remembered while `mode` is not none: the car keeps it while suspended (pedal, clutch or
    // brake), and only drops it when the driver clears it with the reset button.
    std::optional<int> set_speed_kmh;
    bool active = false; // actively regulating or limiting, as opposed to suspended
    bool fault = false;  // "demandé et en défaut"
    bool blink = false;  // the set speed blinks, as the car asks
};

struct Lamps {
    Headlights headlights = Headlights::off;
    Lamp seat_belt = Lamp::off;
    Lamp front_fog = Lamp::off;
    Lamp rear_fog = Lamp::off;
    Lamp brake = Lamp::off; // parking brake or brake circuit incident
    Lamp oil_pressure = Lamp::off;
    Lamp engine = Lamp::off; // emission control (MIL)
    Lamp battery = Lamp::off;
    Lamp abs = Lamp::off;
    Lamp airbag = Lamp::off;
    Lamp stop = Lamp::off;
    Lamp service = Lamp::off; // Renault SERVICE lamp (non-critical fault)
};

struct View {
    bool online = false;            // at least one valid frame
    std::optional<int> rpm;         // rounded to view.cpp's RPM_STEP
    std::optional<int> speed_kmh;   // what the original cluster shows when the car sends it; else the wheel speed
    std::optional<int> coolant_temp_c;
    bool coolant_temp_alert = false;
    std::optional<int> outside_temp_c;
    std::optional<int> battery_decivolts;
    bool battery_low = false;
    std::optional<int> fuel_liters;
    bool fuel_low = false;
    std::optional<int> oil_level;   // 0 to OIL_SEGMENTS
    bool oil_alert = false;         // the car reports "alerte": a known level of 0, in the fault color
    Doors doors;
    bool turn_left = false;         // the turn signals follow the lamps: the car toggles them
    bool turn_right = false;
    std::optional<int> gear;        // empty with the clutch pressed, nearly stopped, or matching no gear
    bool clutch_pressed = false;    // for GearDebounce
    Cruise cruise;
    Lamps lamps;
    int backlight_percent = 100;    // 100 by day; at night (position lights on), the cluster's night dimmer
    std::optional<uint16_t> distance_counter; // 0x354 rolling distance counter (0.1 m), for the trip
    std::optional<uint8_t> fuel_counter;      // 0x551 rolling fuel counter (80 mm³), for the trip
    bool trip_button = false;       // the trip computer button is pressed (either direction)
    std::optional<Notice> notice;   // the message in the header, if any
    std::optional<uint32_t> odometer_km; // the original cluster's odometer (0x5C5 Totalisation générale)
};

} // namespace x84
