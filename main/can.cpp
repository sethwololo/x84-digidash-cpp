#include "can.h"

#include <algorithm>
#include <cstring>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_twai_onchip.h"

namespace can {

namespace {

constexpr const char *TAG = "can";
constexpr gpio_num_t TX = GPIO_NUM_3;
constexpr gpio_num_t RX = GPIO_NUM_2;
// The driver derives the timing from the controller's real clock (the P4's TWAI runs from the 40 MHz crystal).
constexpr uint32_t BITRATE = 500000;

} // namespace

esp_err_t Bus::start()
{
    twai_onchip_node_config_t config = {};
    config.io_cfg.tx = TX;
    config.io_cfg.rx = RX;
    config.io_cfg.quanta_clk_out = GPIO_NUM_NC;
    config.io_cfg.bus_off_indicator = GPIO_NUM_NC;
    config.bit_timing.bitrate = BITRATE;
    config.tx_queue_depth = 1;
    config.flags.enable_listen_only = true;
    ESP_RETURN_ON_ERROR(twai_new_node_onchip(&config, &node_), TAG, "controller init");

    twai_event_callbacks_t callbacks = {};
    callbacks.on_rx_done = on_receive;
    callbacks.on_state_change = on_state_change;
    ESP_RETURN_ON_ERROR(twai_node_register_event_callbacks(node_, &callbacks, this), TAG, "callbacks");
    ESP_RETURN_ON_ERROR(twai_node_enable(node_), TAG, "controller start");
    ESP_LOGI(TAG, "listening at %lu kbit/s (TX GPIO%d, RX GPIO%d)", static_cast<unsigned long>(BITRATE / 1000), TX, RX);
    return ESP_OK;
}

// In the receive interrupt: only copies the frame into its slot.
bool Bus::on_receive(twai_node_handle_t node, const twai_rx_done_event_data_t *, void *context)
{
    Bus &bus = *static_cast<Bus *>(context);
    uint8_t data[8];
    twai_frame_t frame = {};
    frame.buffer = data;
    frame.buffer_len = sizeof(data);
    if (twai_node_receive_from_isr(node, &frame) != ESP_OK) return false;

    const int64_t now = esp_timer_get_time();
    const size_t length = std::min<size_t>(twaifd_dlc2len(frame.header.dlc), sizeof(data));
    const uint16_t *slot = std::find(std::begin(x84::id::ALL), std::end(x84::id::ALL), frame.header.id);
    const bool used = !frame.header.ide && !frame.header.rtr && slot != std::end(x84::id::ALL);

    portENTER_CRITICAL_ISR(&bus.lock_);
    bus.count_++;
    if (used) {
        Received &received = bus.frames_[slot - std::begin(x84::id::ALL)];
        received.sequence++;
        received.at_us = now;
        received.length = static_cast<uint8_t>(length);
        std::memcpy(received.data, data, length);
    }
    portEXIT_CRITICAL_ISR(&bus.lock_);
    return false;
}

bool Bus::on_state_change(twai_node_handle_t, const twai_state_change_event_data_t *event, void *context)
{
    Bus &bus = *static_cast<Bus *>(context);
    const BusState state = event->new_sta == TWAI_ERROR_BUS_OFF  ? BusState::bus_off
                           : event->new_sta == TWAI_ERROR_ACTIVE ? BusState::ok
                                                                 : BusState::errors;
    portENTER_CRITICAL_ISR(&bus.lock_);
    bus.state_ = state;
    portEXIT_CRITICAL_ISR(&bus.lock_);
    return false;
}

Snapshot Bus::snapshot() const
{
    portENTER_CRITICAL(&lock_);
    const Snapshot copy = frames_;
    portEXIT_CRITICAL(&lock_);
    return copy;
}

Bus::Stats Bus::take_stats()
{
    portENTER_CRITICAL(&lock_);
    const Stats stats{count_, state_};
    count_ = 0;
    portEXIT_CRITICAL(&lock_);
    return stats;
}

} // namespace can
