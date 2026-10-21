#include <doctest/doctest.h>

#include "trip.h"

using x84::Trip;

TEST_CASE("sums the distance counter across wrap-around")
{
    Trip trip(100, 0);
    trip.update_distance(65000);
    CHECK(trip.decimeters() == 100);
    trip.update_distance(65300);
    trip.update_distance(65300);
    trip.update_distance(64); // 65536 - 65300 + 64
    CHECK(trip.decimeters() == 100 + 300 + 300);
}

TEST_CASE("restarts after a gap or a jump")
{
    Trip trip(0, 0);
    trip.update_distance(1000);
    trip.update_distance(std::nullopt);
    trip.update_distance(1500); // first reading after the gap: baseline only
    CHECK(trip.decimeters() == 0);
    trip.update_distance(1600);
    CHECK(trip.decimeters() == 100);
    trip.update_distance(40000); // counter restarted
    trip.update_distance(40010);
    CHECK(trip.decimeters() == 110);
}

TEST_CASE("resets after holding the button")
{
    Trip trip(5000, 800);
    CHECK_FALSE(trip.update_button(true, 10000));
    CHECK_FALSE(trip.update_button(true, 11000));
    CHECK(trip.update_button(true, 12000));
    CHECK(trip.decimeters() == 0);
    CHECK(trip.fuel_mm3() == 0);
    // Only once per press.
    trip.update_distance(0);
    trip.update_distance(10);
    CHECK_FALSE(trip.update_button(true, 15000));
    CHECK(trip.decimeters() == 10);
}

TEST_CASE("short presses do nothing")
{
    Trip trip(5000, 800);
    for (uint32_t t = 0; t < 10000; t += 1500) {
        CHECK_FALSE(trip.update_button(true, t));
        CHECK_FALSE(trip.update_button(false, t + 1000));
    }
    CHECK(trip.decimeters() == 5000);
}

TEST_CASE("sums the fuel counter across wrap-around")
{
    Trip trip(0, 0);
    trip.update_fuel(250);
    trip.update_fuel(252);
    trip.update_fuel(3); // 256 - 252 + 3
    CHECK(trip.fuel_mm3() == (2 + 7) * Trip::FUEL_UNIT_MM3);
    trip.update_fuel(std::nullopt);
    trip.update_fuel(100); // baseline after a gap
    trip.update_fuel(99);  // a step back: the counter restarted, not 255 units
    CHECK(trip.fuel_mm3() == 9 * Trip::FUEL_UNIT_MM3);
}

TEST_CASE("average consumption")
{
    // 100 km on 8 L: 12,5 km/L.
    CHECK(Trip(1000000, 8000000).km_per_liter_tenths() == 125);
    // Not before 400 m, nor without fuel.
    CHECK_FALSE(Trip(3999, 100000).km_per_liter_tenths().has_value());
    CHECK_FALSE(Trip(20000, 0).km_per_liter_tenths().has_value());
}
