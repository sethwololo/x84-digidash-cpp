#pragma once

// The car's CAN bus (500 kbit/s) through the TWAI controller, in listen-only mode: the dashboard never transmits nor
// acknowledges, so it cannot disturb the car's network.
//
// The driver's receive interrupt keeps the latest frame of each identifier the dashboard decodes (x84::id::ALL).
// The UI copies them with `snapshot` and decodes the new ones (vehicle.cpp): the interrupt only copies bytes.

#include <array>
#include <cstdint>
#include <iterator>

#include "esp_twai.h"
#include "freertos/FreeRTOS.h"
#include "frames.h"

namespace can {

constexpr size_t SLOTS = std::size(x84::id::ALL);

// The latest frame of one identifier.
struct Received {
    uint32_t sequence = 0; // frames of this identifier since start: a new value is a new frame; 0 = none yet
    int64_t at_us = 0;     // when it arrived (esp_timer)
    uint8_t length = 0;
    uint8_t data[8] = {};
};

using Snapshot = std::array<Received, SLOTS>; // in x84::id::ALL order

// State of the controller, from the driver's state changes.
enum class BusState {
    ok,      // error active: receiving normally
    errors,  // error warning or passive: many bus errors (wrong bitrate, bad wiring)
    bus_off,
};

class Bus {
public:
    // Starts the controller. TX on GPIO3 (D pin of the SN65HVD230, kept recessive by listen-only), RX on GPIO2 (R).
    esp_err_t start();

    Snapshot snapshot() const;

    // Frames seen on the bus (used or not) since the previous call, and the controller's state.
    struct Stats {
        uint32_t frames;
        BusState state;
    };
    Stats take_stats();

private:
    static bool on_receive(twai_node_handle_t node, const twai_rx_done_event_data_t *event, void *context);
    static bool on_state_change(twai_node_handle_t node, const twai_state_change_event_data_t *event, void *context);

    twai_node_handle_t node_ = nullptr;
    mutable portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
    // Written by the interrupt, under the lock.
    Snapshot frames_{};
    uint32_t count_ = 0;
    BusState state_ = BusState::ok;
};

} // namespace can
