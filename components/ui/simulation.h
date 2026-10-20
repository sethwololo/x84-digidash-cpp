#pragma once

// Fake data, in place of the CAN bus: for the bench and the computer. Engine speed and road speed sweep
// continuously, and every 12 s the rest switches between the three scenarios of the design (normal driving,
// high-RPM warning, low fuel).

#include <optional>

#include "screen.h"

namespace ui {

class Simulation {
public:
    static constexpr int SCENARIOS = 3;

    // Stays on one scenario (0 to SCENARIOS - 1), frozen at its first instant, as for a screenshot.
    void pin(int scenario) { pinned_ = scenario % SCENARIOS; }

    // What the dashboard shows at `now_ms` (any monotonic clock).
    Screen screen(uint32_t now_ms);

private:
    std::optional<int> pinned_;
    std::optional<uint32_t> start_ms_;
    uint32_t last_ms_ = 0;
    float trip_km_ = 482.6f;
};

} // namespace ui
