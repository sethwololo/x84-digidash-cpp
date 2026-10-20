#include "vehicle.h"

#include "esp_log.h"
#include "esp_timer.h"

namespace {

constexpr const char *TAG = "vehicle";

// RPM and speed change all the time and cost the most to repaint: they are refreshed at most this often (20 Hz).
constexpr uint32_t FAST_UPDATE_MS = 50;

// The trip is saved each time the distance passes a multiple of this or the fuel one of a turn of the car's fuel
// counter (20.48 mL), and when it is reset. The fuel step keeps what the engine burns standing still (idling at the
// end of a trip) from being lost when the car is switched off. Same keys as the Rust firmware.
constexpr uint32_t TRIP_SAVE_STEP_DECIMETERS = 1000;
constexpr const char *NVS_NAMESPACE = "digidash";
constexpr const char *NVS_TRIP_KEY = "trip_dm";
constexpr const char *NVS_TRIP_FUEL_KEY = "trip_fuel_mm3";

} // namespace

Vehicle::Vehicle(can::Bus &bus) : bus_(bus)
{
    uint32_t decimeters = 0;
    uint32_t fuel_mm3 = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_) == ESP_OK) {
        nvs_get_u32(nvs_, NVS_TRIP_KEY, &decimeters);
        nvs_get_u32(nvs_, NVS_TRIP_FUEL_KEY, &fuel_mm3);
    } else {
        ESP_LOGW(TAG, "NVS unavailable: the trip will not be saved");
    }
    ESP_LOGI(TAG, "trip %.1f km, %.2f L", decimeters / 10000.0, fuel_mm3 / 1000000.0);
    trip_ = x84::Trip(decimeters, fuel_mm3);
    saved_distance_step_ = decimeters / TRIP_SAVE_STEP_DECIMETERS;
    saved_fuel_step_ = fuel_mm3 / x84::Trip::FUEL_TURN_MM3;
}

ui::Screen Vehicle::update(uint32_t now_ms)
{
    decode_new_frames(now_ms);
    x84::View view = x84::view(fresh_frames(now_ms));
    view.gear = gear_debounce_.update(view.gear, view.clutch_pressed, now_ms);
    update_trip(view, now_ms);

    if (!fast_update_ms_ || now_ms - *fast_update_ms_ >= FAST_UPDATE_MS) {
        fast_update_ms_ = now_ms;
        shown_rpm_ = view.rpm;
        shown_speed_ = view.speed_kmh;
    }
    view.rpm = shown_rpm_;
    view.speed_kmh = shown_speed_;

    if (!report_ms_ || now_ms - *report_ms_ >= 1000) {
        report_ms_ = now_ms;
        const can::Bus::Stats stats = bus_.take_stats();
        can_rate_ = static_cast<int>(stats.frames);
        can_state_ = stats.state == can::BusState::bus_off ? ui::CanState::bus_off
                     : stats.state == can::BusState::errors ? ui::CanState::errors
                     : stats.frames == 0                    ? ui::CanState::silent
                                                            : ui::CanState::ok;
    }

    ui::Screen screen;
    screen.view = view;
    screen.trip_hectometers = static_cast<int>(trip_.decimeters() / 1000);
    screen.trip_km_per_liter_tenths = trip_.km_per_liter_tenths();
    screen.can_state = can_state_;
    screen.can_rate = can_rate_;
    screen.firmware_version = "0.1.0";
    return screen;
}

void Vehicle::decode_new_frames(uint32_t now_ms)
{
    const can::Snapshot snapshot = bus_.snapshot();
    const int64_t now_us = esp_timer_get_time();
    for (size_t i = 0; i < can::SLOTS; i++) {
        const can::Received &received = snapshot[i];
        if (received.sequence == sequences_[i]) continue;
        sequences_[i] = received.sequence;
        if (frames_.store(x84::id::ALL[i], std::span(received.data, received.length))) {
            // When it arrived, on LVGL's clock.
            received_at_ms_[i] = now_ms - (now_us - received.at_us) / 1000;
        }
    }
}

x84::Frames Vehicle::fresh_frames(uint32_t now_ms) const
{
    x84::Frames fresh = frames_;
    for (size_t i = 0; i < can::SLOTS; i++) {
        const uint16_t frame_id = x84::id::ALL[i];
        if (sequences_[i] == 0 || now_ms - received_at_ms_[i] >= x84::timeout_ms(frame_id)) fresh.clear(frame_id);
    }
    return fresh;
}

void Vehicle::update_trip(const x84::View &view, uint32_t now_ms)
{
    trip_.update_distance(view.distance_counter);
    trip_.update_fuel(view.fuel_counter);
    const bool reset = trip_.update_button(view.trip_button, now_ms);
    if (reset) ESP_LOGI(TAG, "trip reset");

    const uint32_t distance_step = trip_.decimeters() / TRIP_SAVE_STEP_DECIMETERS;
    const uint32_t fuel_step = trip_.fuel_mm3() / x84::Trip::FUEL_TURN_MM3;
    if (reset || distance_step != saved_distance_step_ || fuel_step != saved_fuel_step_) {
        saved_distance_step_ = distance_step;
        saved_fuel_step_ = fuel_step;
        save_trip();
    }
}

void Vehicle::save_trip()
{
    if (!nvs_) return;
    esp_err_t err = nvs_set_u32(nvs_, NVS_TRIP_KEY, trip_.decimeters());
    if (err == ESP_OK) err = nvs_set_u32(nvs_, NVS_TRIP_FUEL_KEY, trip_.fuel_mm3());
    if (err == ESP_OK) err = nvs_commit(nvs_);
    if (err != ESP_OK) ESP_LOGW(TAG, "trip not saved: %s", esp_err_to_name(err));
}
