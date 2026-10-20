#include <doctest/doctest.h>

#include <vector>

#include "signal.h"

using x84::Signal;

TEST_CASE("whole bytes")
{
    const std::vector<uint8_t> data = {0x5D, 0xC0, 0xAB};
    CHECK(Signal{7, 16}.raw(data) == 0x5DC0u);
    CHECK(Signal{15, 16}.raw(data) == 0xC0ABu);
    CHECK(Signal{15, 8}.raw(data) == 0xC0u);
}

TEST_CASE("inside a byte")
{
    const std::vector<uint8_t> data = {0b1011'0110};
    CHECK(Signal{7, 5}.raw(data) == 0b10110u);
    CHECK(Signal{2, 3}.raw(data) == 0b110u);
    CHECK(Signal{5, 2}.raw(data) == 0b11u);
    CHECK(Signal{0, 1}.raw(data) == 0u);
    CHECK(Signal{1, 1}.raw(data) == 1u);
}

TEST_CASE("crosses a byte boundary")
{
    // Starts at bit 2 of byte 0 and continues into bits 7..6 of byte 1.
    const std::vector<uint8_t> data = {0b0000'0101, 0b1000'0000};
    CHECK(Signal{2, 5}.raw(data) == 0b10110u);
}

TEST_CASE("short frame")
{
    const std::vector<uint8_t> data = {0xFF};
    CHECK_FALSE(Signal{15, 8}.raw(data).has_value());
    CHECK_FALSE(Signal{7, 16}.raw(data).has_value());
}
