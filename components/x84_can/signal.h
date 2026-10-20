#pragma once

// A big-endian (Motorola) signal, in the DBC convention used by can/signals-x84.json: `start` is the bit number of
// the most significant bit (byte * 8 + bit, bit 7 being the MSB of the byte), and the signal continues towards the
// LSB, wrapping into bit 7 of the next byte.

#include <cstdint>
#include <optional>
#include <span>

namespace x84 {

struct Signal {
    uint8_t start;
    uint8_t length;

    // The raw (unscaled) value, or nothing if the frame is too short to hold it.
    constexpr std::optional<uint32_t> raw(std::span<const uint8_t> data) const
    {
        size_t byte = start / 8;
        int bit = start % 8;
        uint32_t value = 0;
        for (int i = 0; i < length; i++) {
            if (byte >= data.size()) return std::nullopt;
            value = (value << 1) | ((data[byte] >> bit) & 1);
            if (bit == 0) {
                byte++;
                bit = 7;
            } else {
                bit--;
            }
        }
        return value;
    }
};

} // namespace x84
