#pragma once

// Everything the dashboard shows: the view of the car, plus the trip and the state of the CAN bus.

#include <optional>

#include "view.h"

namespace ui {

// The CAN bus, in the footer.
enum class CanState {
    silent, // no frame in the last second
    ok,
    errors, // the controller sees bus errors (error warning or passive)
    bus_off,
};

struct Screen {
    x84::View view;
    int trip_hectometers = 0;                       // distance since the trip reset, in tenths of a km
    std::optional<int> trip_km_per_liter_tenths;    // average consumption since the reset
    CanState can_state = CanState::silent;
    int can_rate = 0;                               // frames per second
    const char *firmware_version = "";
};

} // namespace ui
