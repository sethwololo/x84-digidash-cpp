#pragma once

// Feeds the dashboard from the CAN bus: decodes the new frames, keeps the ones that have not timed out, turns them
// into what the screen shows, and keeps the trip (distance and fuel, for the average consumption) in NVS.

#include <array>
#include <cstdint>

#include "can.h"
#include "nvs.h"
#include "screen.h"
#include "trip.h"
#include "view.h"

class Vehicle {
public:
    explicit Vehicle(can::Bus &bus);

    // What the dashboard shows now; `now_ms` is LVGL's tick.
    ui::Screen update(uint32_t now_ms);

private:
    void decode_new_frames(uint32_t now_ms);
    x84::Frames fresh_frames(uint32_t now_ms) const;
    void update_trip(const x84::View &view, uint32_t now_ms);
    void save_trip();

    can::Bus &bus_;
    nvs_handle_t nvs_ = 0;
    x84::Trip trip_{0, 0};
    uint32_t saved_distance_step_ = 0;
    uint32_t saved_fuel_step_ = 0;
    x84::GearDebounce gear_debounce_;

    // The latest decoded frame of each identifier, and when it arrived; a frame too short to decode leaves the
    // previous one.
    x84::Frames frames_;
    std::array<uint32_t, can::SLOTS> sequences_{};
    std::array<int64_t, can::SLOTS> received_at_ms_{};

    // RPM and speed on screen, refreshed every FAST_UPDATE_MS.
    std::optional<uint32_t> fast_update_ms_;
    std::optional<int> shown_rpm_;
    std::optional<int> shown_speed_;

    // The CAN bus in the footer, refreshed every second.
    std::optional<uint32_t> report_ms_;
    ui::CanState can_state_ = ui::CanState::silent;
    int can_rate_ = 0;
};
