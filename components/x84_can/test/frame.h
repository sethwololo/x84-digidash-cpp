#pragma once

// Builds test frames by writing raw values at signal positions (the inverse of Signal::raw).

#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

#include "signal.h"

inline std::vector<uint8_t> frame(size_t length, std::initializer_list<std::pair<x84::Signal, uint32_t>> fields)
{
    std::vector<uint8_t> data(length);
    for (const auto &[signal, value] : fields) {
        size_t byte = signal.start / 8;
        int bit = signal.start % 8;
        for (int i = signal.length - 1; i >= 0; i--) {
            if (value & (1u << i)) data[byte] |= static_cast<uint8_t>(1 << bit);
            if (bit == 0) {
                byte++;
                bit = 7;
            } else {
                bit--;
            }
        }
    }
    return data;
}
