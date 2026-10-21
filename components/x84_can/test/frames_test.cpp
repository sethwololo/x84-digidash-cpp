#include <doctest/doctest.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "frame.h"
#include "frames.h"

using namespace x84;
using namespace x84::signal;

// Every signal position used, with its frame and its name in the JSON.
struct Position {
    uint16_t id;
    const char *name;
    Signal signal;
};

const Position POSITIONS[] = {
    {0x181, "Engine speed", ENGINE_SPEED},
    {0x181, "Clutch switch", CLUTCH},
    {0x161, "Cruise control and speed limiter states", CRUISE_STATE},
    {0x215, "Neutral", NEUTRAL},
    {0x374, "Evaporator temperature", EVAPORATOR_TEMP},
    {0x354, "ABS lamp state", ABS_LAMP},
    {0x354, "Distance travelled", DISTANCE},
    {0x354, "Vehicle speed (front wheel average)", WHEEL_SPEED},
    {0x35D, "BrakePadWear", BRAKE_PAD_WEAR},
    {0x35D, "Child lock fault alert", CHILD_LOCK_FAULT},
    {0x551, "Engine coolant temperature", COOLANT_TEMP},
    {0x551, "Coolant temperature alert", COOLANT_TEMP_ALERT},
    {0x551, "Emissions fault lamp", MIL},
    {0x551, "Set speed", SET_SPEED},
    {0x551, "Cruise/limiter set speed blinking", SET_SPEED_BLINK},
    {0x551, "Cruise/limiter state display", CRUISE_DISPLAY},
    {0x551, "Fuel consumption", FUEL_CONSUMPTION},
    {0x551, "Injection fault severity 1", INJECTION_FAULT},
    {0x551, "Injection fault severity 2", SEVERE_INJECTION_FAULT},
    {0x5C5, "Fuel level", FUEL_LEVEL},
    {0x5C5, "Fuel gauge diagnostic", FUEL_GAUGE},
    {0x5C5, "Low fuel alert", FUEL_LOW},
    {0x5C5, "Parking brake switch", PARKING_BRAKE},
    {0x5C5, "Brake circuit fault", BRAKE_CIRCUIT},
    {0x5C5, "AIRBAG fault lamp state", AIRBAG_LAMP},
    {0x5C5, "Lamp self-test driven by the cluster", LAMP_SELF_TEST},
    {0x5C5, "Odometer", ODOMETER},
    {0x5E4, "DAE fault", STEERING_FAULT},
    {0x60D, "Openings state", OPENINGS},
    {0x60D, "Turn signal states", TURN_SIGNALS},
    {0x60D, "Rear fog light state", REAR_FOG},
    {0x60D, "Wiping fault", WIPER_FAULT},
    {0x60D, "Column lock or UCH fault", COLUMN_LOCK_FAULT},
    {0x60D, "Outside temperature", OUTSIDE_TEMP},
    {0x60D, "Reverse gear engaged", REVERSE},
    {0x60D, "Trip computer scroll request", SCROLL},
    {0x60D, "Card battery to replace", CARD_BATTERY_LOW},
    {0x60D, "Keyless information display", KEYLESS_DISPLAY},
    {0x625, "Battery voltage by UPC", BATTERY_VOLTAGE},
    {0x625, "Oil pressure switch", OIL_PRESSURE_SWITCH},
    {0x625, "Battery lamp", BATTERY_LAMP},
    {0x625, "Low battery alert", BATTERY_LOW},
    {0x625, "High beam state", HIGH_BEAM},
    {0x625, "Front fog lights state", FRONT_FOG},
    {0x625, "Low beam state", LOW_BEAM},
    {0x625, "Position lights state", POSITION_LAMPS},
    {0x625, "APC distribution state", APC_DISTRIBUTED},
    {0x645, "Displayed speed", DISPLAYED_SPEED},
    {0x645, "Night dimmer maximum lighting percentage", NIGHT_DIMMER},
    {0x645, "PositionLightsStatus", CLUSTER_POSITION_LAMPS},
    {0x645, "Seat belt lamp state", SEAT_BELT_LAMP},
    {0x651, "Driver seat belt switch", DRIVER_BELT},
    {0x651, "Passenger on seat", PASSENGER_ON_SEAT},
    {0x651, "Passenger seat belt switch", PASSENGER_BELT},
    {0x715, "Displayed oil level", OIL_LEVEL},
    {0x715, "MIL lamp state", CLUSTER_MIL},
    {0x715, "Oil service range", OIL_SERVICE_RANGE},
};

TEST_CASE("positions match the JSON")
{
    std::ifstream file(SIGNALS_JSON);
    REQUIRE(file.good());
    const auto json = nlohmann::json::parse(file);
    for (const auto &position : POSITIONS) {
        CAPTURE(std::string(position.name));
        const auto &messages = json["messages"];
        const auto message =
            std::find_if(messages.begin(), messages.end(), [&](const auto &m) { return m["id"] == position.id; });
        REQUIRE(message != messages.end());
        const auto &signals = (*message)["signals"];
        const auto spec = std::find_if(signals.begin(), signals.end(),
                                       [&](const auto &s) { return s["name"] == position.name; });
        REQUIRE(spec != signals.end());
        CHECK((*spec)["byte_order"] == "big_endian");
        CHECK((*spec)["start_bit"] == position.signal.start);
        CHECK((*spec)["length"] == position.signal.length);
        // The signal must fit in the frame's DLC.
        const std::vector<uint8_t> zeros((*message)["dlc"].get<size_t>());
        CHECK(position.signal.raw(zeros).has_value());
    }
}

TEST_CASE("every identifier decodes a full frame")
{
    const std::vector<uint8_t> zeros(8);
    for (uint16_t frame_id : id::ALL) {
        Frames frames;
        CHECK(frames.store(frame_id, zeros));
    }
    Frames frames;
    CHECK_FALSE(frames.store(0x023, zeros)); // not decoded here
    CHECK_FALSE(frames.store(id::ECM_TORQUE_CONTROL, std::vector<uint8_t>{0x5D})); // too short
}

TEST_CASE("a frame too short keeps the previous one")
{
    Frames frames;
    frames.store(id::ECM_TORQUE_CONTROL, frame(8, {{ENGINE_SPEED, 6000 * 8}}));
    frames.store(id::ECM_TORQUE_CONTROL, std::vector<uint8_t>{0x01});
    REQUIRE(frames.ecm_torque_control);
    CHECK(frames.ecm_torque_control->engine_speed_rpm == 6000);
}

TEST_CASE("engine speed and clutch")
{
    // 3000 rpm = 24000 eighths = 0x5DC0, in bytes 0 and 1.
    const std::vector<uint8_t> data = {0x5D, 0xC0, 0, 0, 0, 0, 0, 0};
    auto engine = EcmTorqueControl::decode(data);
    REQUIRE(engine);
    CHECK(engine->engine_speed_rpm == 3000);
    CHECK(engine->clutch_pressed == false);
    engine = EcmTorqueControl::decode(frame(8, {{ENGINE_SPEED, 6007}, {CLUTCH, 0b01}}));
    CHECK(engine->engine_speed_rpm == 750);
    CHECK(engine->clutch_pressed == true);
    CHECK_FALSE(EcmTorqueControl::decode(frame(8, {{CLUTCH, 0b11}}))->clutch_pressed.has_value());
}

TEST_CASE("brake data")
{
    const auto brake = BrakeGeneralData::decode(frame(8, {{ABS_LAMP, 0b01}, {DISTANCE, 65535}, {WHEEL_SPEED, 13250}}));
    REQUIRE(brake);
    CHECK(brake->abs_lamp == AbsLamp::fault);
    CHECK(brake->distance_decimeters == 65535);
    CHECK(brake->wheel_speed_centi_kmh == 13250);
}

TEST_CASE("engine status")
{
    auto status = EcmGeneralStatus::decode(frame(8, {
        {COOLANT_TEMP, 129}, {COOLANT_TEMP_ALERT, 1}, {MIL, 0b10}, {SET_SPEED, 110}, {SET_SPEED_BLINK, 1},
        {CRUISE_DISPLAY, 0b001}, {SEVERE_INJECTION_FAULT, 1},
    }));
    REQUIRE(status);
    CHECK(status->coolant_temp_c == 89);
    CHECK(status->coolant_temp_alert);
    CHECK(status->mil == MilLamp::blinking);
    CHECK(status->set_speed_kmh == 110);
    CHECK(status->set_speed_blink);
    CHECK(status->cruise_display == CruiseDisplay::limiter_active);
    CHECK(status->fuel_counter == 0);
    CHECK_FALSE(status->injection_fault);
    CHECK(status->severe_injection_fault);
    // The car sends fewer than the 8 bytes of the database; every signal read is in the first 6.
    const auto data = frame(8, {{COOLANT_TEMP, 129}});
    CHECK(EcmGeneralStatus::decode(std::span(data).first(6))->coolant_temp_c == 89);
    status = EcmGeneralStatus::decode(frame(8, {{COOLANT_TEMP, 0}, {CRUISE_DISPLAY, 0b100}}));
    CHECK(status->coolant_temp_c == -40);
    CHECK(status->cruise_display == CruiseDisplay::cruise_active);
}

TEST_CASE("cluster status")
{
    const auto status = ClusterGeneralStatus::decode(frame(8, {
        {FUEL_LEVEL, 31}, {FUEL_GAUGE, 0b01}, {FUEL_LOW, 1}, {PARKING_BRAKE, 1}, {LAMP_SELF_TEST, 1}, {ODOMETER, 123456},
    }));
    REQUIRE(status);
    CHECK(status->fuel_liters == 31);
    CHECK(status->fuel_gauge == FuelGauge::open_circuit);
    CHECK(status->fuel_low);
    CHECK(status->parking_brake);
    CHECK_FALSE(status->brake_circuit_fault);
    CHECK_FALSE(status->airbag_lamp);
    CHECK(status->lamp_self_test);
    CHECK(status->odometer_km == 123456u);
}

TEST_CASE("steering fault")
{
    const std::pair<uint32_t, SteeringFault> cases[] = {
        {0b00, SteeringFault::none}, {0b01, SteeringFault::service}, {0b10, SteeringFault::stop}, {0b11, SteeringFault::diagnostic}};
    for (const auto &[value, fault] : cases) {
        CHECK(EpsGeneralStatus::decode(frame(3, {{STEERING_FAULT, value}}))->steering_fault == fault);
    }
}

TEST_CASE("body status")
{
    const auto status = UchGeneralStatus::decode(frame(8, {
        {OPENINGS, 0b10100}, {TURN_SIGNALS, 0b010}, {REAR_FOG, 1}, {WIPER_FAULT, 1}, {COLUMN_LOCK_FAULT, 0b110},
        {OUTSIDE_TEMP, 58}, {REVERSE, 0b01}, {SCROLL, 0b10}, {CARD_BATTERY_LOW, 1}, {KEYLESS_DISPLAY, 0b10001},
    }));
    REQUIRE(status);
    CHECK(status->openings == Openings{.rear_left = true, .trunk = true});
    CHECK(status->turn_signals == TurnSignals::right);
    CHECK(status->rear_fog);
    CHECK(status->wiper_fault);
    CHECK(status->column_lock_fault == ColumnLockFault::service);
    CHECK(status->outside_temp_c == 18);
    CHECK(status->reverse == true);
    CHECK(status->scroll_request == ScrollRequest::backward);
    CHECK(status->card_battery_low == true);
    CHECK(status->keyless_display == 0b10001);
}

TEST_CASE("body status: undocumented values")
{
    for (uint32_t value : {0b100u, 0b101u, 0b110u, 0b111u}) {
        CHECK(UchGeneralStatus::decode(frame(8, {{TURN_SIGNALS, value}}))->turn_signals == TurnSignals::invalid);
    }
    const std::pair<uint32_t, ColumnLockFault> cases[] = {
        {0b001, ColumnLockFault::stop},         {0b010, ColumnLockFault::not_locked}, {0b011, ColumnLockFault::reserved},
        {0b100, ColumnLockFault::not_unlocked}, {0b101, ColumnLockFault::reserved},   {0b111, ColumnLockFault::reserved},
    };
    for (const auto &[value, fault] : cases) {
        CHECK(UchGeneralStatus::decode(frame(8, {{COLUMN_LOCK_FAULT, value}}))->column_lock_fault == fault);
    }
}

TEST_CASE("power status")
{
    // 12.625 V = 202 sixteenths of a volt.
    const auto status = UpcGeneralStatus::decode(frame(6, {{BATTERY_VOLTAGE, 202}, {OIL_PRESSURE_SWITCH, 1}, {HIGH_BEAM, 1}}));
    REQUIRE(status);
    CHECK(status->battery_millivolts == 12625);
    CHECK(status->oil_pressure_switch_closed);
    CHECK(status->high_beam);
    CHECK_FALSE(status->low_beam);
    CHECK_FALSE(status->apc_distributed);
}

TEST_CASE("cluster status 2")
{
    const auto data = frame(5, {{DISPLAYED_SPEED, 13600}, {NIGHT_DIMMER, 125}, {SEAT_BELT_LAMP, 0b10}, {CLUSTER_POSITION_LAMPS, 1}});
    const auto status = ClusterGeneralStatus2::decode(data);
    REQUIRE(status);
    CHECK(status->displayed_speed_centi_kmh == 13600);
    CHECK(status->night_dimmer_permille == 500);
    CHECK(status->position_lamps);
    CHECK(status->seat_belt_lamp == SeatBeltLamp::blinking);
    // This car sends only the first 3 bytes: everything but the displayed speed.
    const auto short_status = ClusterGeneralStatus2::decode(std::span(data).first(3));
    CHECK_FALSE(short_status->displayed_speed_centi_kmh.has_value());
    CHECK(short_status->night_dimmer_permille == 500);
    CHECK(short_status->position_lamps);
}

TEST_CASE("oil level")
{
    const auto oil = [](uint32_t value) { return ClusterBackup::decode(frame(6, {{OIL_LEVEL, value}})); };
    CHECK(oil(0)->oil_alert);
    CHECK_FALSE(oil(0)->oil_bars.has_value());
    CHECK(oil(1)->oil_bars == 1);
    CHECK(oil(8)->oil_bars == 8);
    CHECK_FALSE(oil(9)->oil_bars.has_value());
    CHECK_FALSE(oil(15)->oil_bars.has_value());
    CHECK_FALSE(oil(15)->oil_alert);
}

TEST_CASE("cluster backup")
{
    const auto data = frame(8, {{OIL_LEVEL, 5}, {CLUSTER_MIL, 0b01}, {OIL_SERVICE_RANGE, 60}});
    const auto backup = ClusterBackup::decode(data);
    CHECK(backup->oil_bars == 5);
    CHECK(backup->mil == MilLamp::on);
    CHECK(backup->oil_service_range_km == 15000);
    // The 6 bytes an older signal database gives this frame hold everything read here; with fewer, the MIL and the range are missing but the oil
    // level is still read.
    CHECK(ClusterBackup::decode(std::span(data).first(6))->oil_service_range_km == 15000);
    const auto short_backup = ClusterBackup::decode(std::span(data).first(4));
    CHECK(short_backup->oil_bars == 5);
    CHECK(short_backup->mil == MilLamp::on);
    CHECK_FALSE(short_backup->oil_service_range_km.has_value());
}

TEST_CASE("small frames")
{
    CHECK(EcmToAt::decode(frame(5, {{CRUISE_STATE, 0b10}}))->cruise_state == CruiseState::active);
    CHECK(UpcFastFrame::decode(frame(6, {{NEUTRAL, 0b10}}))->neutral == NeutralSwitch::not_equipped);
    // 0.5 °C from -40 °C; 0xFF while the car switches off.
    CHECK(ClimControlFrame::decode(frame(3, {{EVAPORATOR_TEMP, 153}}))->evaporator_temp_tenths == 365);
    CHECK(ClimControlFrame::decode(frame(3, {{EVAPORATOR_TEMP, 70}}))->evaporator_temp_tenths == -50);
    CHECK_FALSE(ClimControlFrame::decode(frame(3, {{EVAPORATOR_TEMP, 0xFF}}))->evaporator_temp_tenths.has_value());
    CHECK_FALSE(ClimControlFrame::decode(frame(2, {})).has_value());
    CHECK(UchSystemControl::decode(frame(8, {{BRAKE_PAD_WEAR, 0b10}})).brake_pad_wear == 0b10);
    CHECK_FALSE(UchSystemControl::decode(std::vector<uint8_t>(5)).brake_pad_wear.has_value());
    CHECK(UchSystemControl::decode(frame(8, {{CHILD_LOCK_FAULT, 1}})).child_lock_fault == true);
}

TEST_CASE("airbag status")
{
    const auto status = AirbagGeneralStatus::decode(frame(2, {{DRIVER_BELT, 0b01}, {PASSENGER_ON_SEAT, 0b00}, {PASSENGER_BELT, 0b11}}));
    REQUIRE(status);
    CHECK(status->driver_unbuckled == true);
    CHECK(status->passenger_on_seat == true);
    CHECK_FALSE(status->passenger_unbuckled.has_value());
}
