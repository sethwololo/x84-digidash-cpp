#include <doctest/doctest.h>

#include <vector>

#include "view.h"

using namespace x84;

namespace {

const std::vector<uint8_t> ZEROS(8);

UchGeneralStatus body()
{
    return *UchGeneralStatus::decode(ZEROS);
}

EcmGeneralStatus engine()
{
    return *EcmGeneralStatus::decode(ZEROS);
}

ClusterGeneralStatus cluster()
{
    return *ClusterGeneralStatus::decode(ZEROS);
}

UpcGeneralStatus power()
{
    return *UpcGeneralStatus::decode(ZEROS);
}

ClusterGeneralStatus2 cluster2(SeatBeltLamp belt, bool position_lamps = false, int dimmer = 0,
                               std::optional<int> displayed_speed = std::nullopt)
{
    return ClusterGeneralStatus2{displayed_speed, dimmer, position_lamps, belt};
}

Frames driving(int rpm, int speed_centi_kmh, std::optional<bool> clutch_pressed)
{
    Frames frames;
    frames.ecm_torque_control = EcmTorqueControl{rpm, clutch_pressed};
    frames.brake_general_data = BrakeGeneralData{AbsLamp::off, 0, static_cast<uint16_t>(speed_centi_kmh)};
    return frames;
}

} // namespace

TEST_CASE("nothing received")
{
    const View v = view(Frames{});
    CHECK_FALSE(v.online);
    CHECK_FALSE(v.rpm.has_value());
    CHECK_FALSE(v.speed_kmh.has_value());
    CHECK(v.lamps.stop == Lamp::off);
    CHECK(v.lamps.headlights == Headlights::off);
}

TEST_CASE("RPM is rounded to the step")
{
    const auto rpm = [](int raw) {
        Frames frames;
        frames.ecm_torque_control = EcmTorqueControl{raw, std::nullopt};
        return view(frames).rpm;
    };
    CHECK(rpm(0) == 0);
    CHECK(rpm(812) == 800);
    CHECK(rpm(813) == 825);
    CHECK(rpm(837) == 825);
    CHECK(rpm(838) == 850);
    CHECK(rpm(6990) == 7000);
}

TEST_CASE("speed from the cluster or the wheels")
{
    Frames frames = driving(0, 9850, std::nullopt);
    frames.cluster_general_status_2 = cluster2(SeatBeltLamp::off, false, 0, 10250);
    CHECK(view(frames).speed_kmh == 102);
    frames.cluster_general_status_2 = cluster2(SeatBeltLamp::off);
    CHECK(view(frames).speed_kmh == 98);
    CHECK(view(driving(0, 9850, std::nullopt)).speed_kmh == 98);
    Frames only_cluster;
    only_cluster.cluster_general_status_2 = cluster2(SeatBeltLamp::off);
    CHECK_FALSE(view(only_cluster).speed_kmh.has_value());
}

TEST_CASE("STOP and SERVICE only from documented signals")
{
    const auto steering = [](SteeringFault fault) {
        Frames frames;
        frames.eps_general_status = EpsGeneralStatus{fault};
        return view(frames).lamps;
    };
    CHECK(steering(SteeringFault::stop).stop == Lamp::on);
    CHECK(steering(SteeringFault::service).service == Lamp::on);
    CHECK(steering(SteeringFault::diagnostic).stop == Lamp::off);
    CHECK(steering(SteeringFault::diagnostic).service == Lamp::off);

    const auto lock = [](ColumnLockFault fault) {
        Frames frames;
        frames.uch_general_status = body();
        frames.uch_general_status->column_lock_fault = fault;
        return view(frames).lamps;
    };
    CHECK(lock(ColumnLockFault::stop).stop == Lamp::on);
    CHECK(lock(ColumnLockFault::service).service == Lamp::on);
    for (auto fault : {ColumnLockFault::not_locked, ColumnLockFault::not_unlocked, ColumnLockFault::reserved}) {
        CHECK(lock(fault).stop == Lamp::off);
        CHECK(lock(fault).service == Lamp::off);
    }

    // Faults with their own lamp only do not light STOP or SERVICE.
    Frames frames;
    frames.ecm_general_status = engine();
    frames.ecm_general_status->coolant_temp_alert = true;
    frames.ecm_general_status->mil = MilLamp::on;
    frames.cluster_general_status = cluster();
    frames.cluster_general_status->brake_circuit_fault = true;
    CHECK(view(frames).lamps.stop == Lamp::off);
    CHECK(view(frames).lamps.service == Lamp::off);
}

TEST_CASE("STOP and SERVICE along with a system's own lamp")
{
    const auto injection = [](bool fault, bool severe) {
        Frames frames;
        frames.ecm_general_status = engine();
        frames.ecm_general_status->injection_fault = fault;
        frames.ecm_general_status->severe_injection_fault = severe;
        return view(frames).lamps;
    };
    CHECK(injection(true, false).service == Lamp::on);
    CHECK(injection(true, false).stop == Lamp::off);
    CHECK(injection(false, true).stop == Lamp::on);

    const auto brakes = [](AbsLamp abs, bool brake_fault) {
        Frames frames = driving(800, 0, false);
        frames.brake_general_data->abs_lamp = abs;
        frames.cluster_general_status = cluster();
        frames.cluster_general_status->brake_circuit_fault = brake_fault;
        return view(frames).lamps;
    };
    CHECK(brakes(AbsLamp::fault, false).service == Lamp::on);
    CHECK(brakes(AbsLamp::fault, false).stop == Lamp::off);
    CHECK(brakes(AbsLamp::fault, true).service == Lamp::on);
    CHECK(brakes(AbsLamp::fault, true).stop == Lamp::on);
    for (auto abs : {AbsLamp::not_programmed, AbsLamp::calibration}) {
        CHECK(brakes(abs, true).stop == Lamp::off);
        CHECK(brakes(abs, true).service == Lamp::off);
    }

    // The battery lamp is lit before the engine starts: only a running engine makes it a charge fault.
    const auto battery = [](int rpm) {
        Frames frames = driving(rpm, 0, false);
        frames.upc_general_status = power();
        frames.upc_general_status->battery_lamp = true;
        return view(frames).lamps;
    };
    CHECK(battery(0).stop == Lamp::off);
    CHECK(battery(300).stop == Lamp::off);
    CHECK(battery(800).stop == Lamp::on);

    Frames airbag;
    airbag.cluster_general_status = cluster();
    airbag.cluster_general_status->airbag_lamp = true;
    CHECK(view(airbag).lamps.service == Lamp::on);
    CHECK(view(airbag).lamps.stop == Lamp::off);
}

TEST_CASE("lamp self-test lights everything")
{
    Frames frames;
    frames.cluster_general_status = cluster();
    frames.cluster_general_status->lamp_self_test = true;
    const Lamps lamps = view(frames).lamps;
    CHECK(lamps.headlights == Headlights::high_beam);
    for (Lamp lamp : {lamps.seat_belt, lamps.front_fog, lamps.rear_fog, lamps.brake, lamps.oil_pressure, lamps.engine,
                      lamps.battery, lamps.abs, lamps.airbag, lamps.stop, lamps.service}) {
        CHECK(lamp == Lamp::on);
    }
}

TEST_CASE("blinking lamps")
{
    Frames frames;
    frames.ecm_general_status = engine();
    frames.ecm_general_status->mil = MilLamp::blinking;
    CHECK(view(frames).lamps.engine == Lamp::blinking);
    Frames belt;
    belt.cluster_general_status_2 = cluster2(SeatBeltLamp::blinking);
    CHECK(view(belt).lamps.seat_belt == Lamp::blinking);
}

TEST_CASE("keyless display messages")
{
    const auto notice = [](std::optional<int> raw) {
        Frames frames;
        frames.uch_general_status = body();
        frames.uch_general_status->keyless_display = raw;
        return view(frames).notice;
    };
    CHECK_FALSE(notice(0).has_value());
    CHECK(notice(2) == Notice::insert_card);
    CHECK(notice(9) == Notice::brake_and_start);
    CHECK(notice(10) == Notice::remove_card);
    CHECK(notice(17) == Notice::remove_card);
    // Reserved, automatic gearbox only, and "extinction totale".
    for (int raw : {12, 13, 15, 28, 31}) CHECK_FALSE(notice(raw).has_value());
    // A frame too short for the signal shows nothing.
    CHECK_FALSE(notice(std::nullopt).has_value());
}

TEST_CASE("notice priority")
{
    const auto all_faults = [](std::optional<int> keyless) {
        Frames frames;
        frames.uch_general_status = body();
        frames.uch_general_status->keyless_display = keyless;
        frames.uch_general_status->card_battery_low = true;
        frames.uch_general_status->wiper_fault = true;
        frames.uch_general_status->column_lock_fault = ColumnLockFault::not_locked;
        frames.ecm_general_status = engine();
        frames.ecm_general_status->injection_fault = true;
        return frames;
    };
    // The original display's message comes before the lasting faults.
    CHECK(view(all_faults(17)).notice == Notice::remove_card);
    CHECK(view(all_faults(std::nullopt)).notice == Notice::injection_fault);
    Frames remove_card;
    remove_card.uch_general_status = body();
    remove_card.uch_general_status->keyless_display = 17;
    CHECK(view(remove_card).notice == Notice::remove_card);
    Frames card_battery;
    card_battery.uch_general_status = body();
    card_battery.uch_general_status->card_battery_low = true;
    CHECK(view(card_battery).notice == Notice::card_battery_low);
    // Only the fault that stops the car comes before it.
    remove_card.ecm_general_status = engine();
    remove_card.ecm_general_status->severe_injection_fault = true;
    CHECK(view(remove_card).notice == Notice::severe_injection_fault);
    CHECK(severity(Notice::severe_injection_fault) == Severity::fault);
    CHECK(severity(Notice::card_battery_low) == Severity::service);
    CHECK(severity(Notice::remove_card) == Severity::info);
}

TEST_CASE("seat belt from the airbag switch")
{
    const auto belt = [](std::optional<SeatBeltLamp> cluster_lamp, std::optional<bool> driver,
                         std::optional<bool> on_seat = std::nullopt, std::optional<bool> passenger = std::nullopt) {
        Frames frames;
        if (cluster_lamp) frames.cluster_general_status_2 = cluster2(*cluster_lamp);
        frames.airbag_general_status = AirbagGeneralStatus{driver, on_seat, passenger};
        return view(frames).lamps.seat_belt;
    };
    CHECK(belt(SeatBeltLamp::off, true) == Lamp::on);
    CHECK(belt(std::nullopt, true) == Lamp::on);
    CHECK(belt(std::nullopt, false) == Lamp::off);
    CHECK(belt(std::nullopt, std::nullopt) == Lamp::off);
    // The passenger, only when the seat sensor detects someone.
    CHECK(belt(std::nullopt, false, true, true) == Lamp::on);
    CHECK(belt(std::nullopt, false, false, true) == Lamp::off);
    CHECK(belt(std::nullopt, false, std::nullopt, true) == Lamp::off);
    CHECK(belt(std::nullopt, false, true, false) == Lamp::off);
    // The cluster's blinking wins over the plain switch.
    CHECK(belt(SeatBeltLamp::blinking, true) == Lamp::blinking);
}

TEST_CASE("engine lamp from the cluster, else the ECU")
{
    const auto lamp = [](std::optional<MilLamp> ecu, bool backup, std::optional<MilLamp> cluster_mil) {
        Frames frames;
        if (ecu) {
            frames.ecm_general_status = engine();
            frames.ecm_general_status->mil = *ecu;
        }
        if (backup) frames.cluster_backup = ClusterBackup{false, std::nullopt, cluster_mil, std::nullopt};
        return view(frames).lamps.engine;
    };
    CHECK(lamp(MilLamp::on, false, std::nullopt) == Lamp::on);
    CHECK(lamp(MilLamp::on, true, MilLamp::off) == Lamp::off);
    CHECK(lamp(MilLamp::off, true, MilLamp::blinking) == Lamp::blinking);
    CHECK(lamp(std::nullopt, true, MilLamp::self_test) == Lamp::on);
    // A 0x715 too short for the lamp leaves it to the ECU.
    CHECK(lamp(MilLamp::on, true, std::nullopt) == Lamp::on);
}

TEST_CASE("fuel needs a working gauge")
{
    Frames frames;
    frames.cluster_general_status = cluster();
    frames.cluster_general_status->fuel_liters = 31;
    CHECK(view(frames).fuel_liters == 31);
    frames.cluster_general_status->fuel_gauge = FuelGauge::open_circuit;
    CHECK_FALSE(view(frames).fuel_liters.has_value());
}

TEST_CASE("oil level")
{
    const auto oil = [](bool alert, std::optional<int> bars) {
        Frames frames;
        frames.cluster_backup = ClusterBackup{alert, bars, std::nullopt, std::nullopt};
        return view(frames);
    };
    CHECK(oil(false, 5).oil_level == 5);
    CHECK(oil(false, 8).oil_level == OIL_SEGMENTS);
    CHECK(oil(true, std::nullopt).oil_level == 0);
    CHECK(oil(true, std::nullopt).oil_alert);
    CHECK_FALSE(oil(false, std::nullopt).oil_level.has_value());
}

TEST_CASE("cruise set speed remembered while suspended")
{
    const auto cruise = [](CruiseDisplay display) {
        Frames frames;
        frames.ecm_general_status = engine();
        frames.ecm_general_status->cruise_display = display;
        frames.ecm_general_status->set_speed_kmh = 110;
        return view(frames).cruise;
    };
    Cruise active = cruise(CruiseDisplay::limiter_active);
    CHECK(active.mode == CruiseMode::limiter);
    CHECK(active.set_speed_kmh == 110);
    CHECK(active.active);
    // Suspended (clutch, brake, pedal): the car keeps showing the last set speed, just not applying it.
    Cruise suspended = cruise(CruiseDisplay::limiter_suspended);
    CHECK(suspended.set_speed_kmh == 110);
    CHECK_FALSE(suspended.active);
    // Off (reset button): the memory is gone.
    CHECK_FALSE(cruise(CruiseDisplay::off).set_speed_kmh.has_value());
    CHECK_FALSE(view(Frames{}).cruise.set_speed_kmh.has_value());
    Cruise fault = cruise(CruiseDisplay::cruise_fault);
    CHECK(fault.mode == CruiseMode::regulator);
    CHECK_FALSE(fault.active);
    CHECK(fault.fault);
}

TEST_CASE("cruise set speed blinks as asked")
{
    const auto blink = [](CruiseDisplay display, bool blinking) {
        Frames frames;
        frames.ecm_general_status = engine();
        frames.ecm_general_status->cruise_display = display;
        frames.ecm_general_status->set_speed_kmh = 110;
        frames.ecm_general_status->set_speed_blink = blinking;
        return view(frames).cruise.blink;
    };
    CHECK(blink(CruiseDisplay::limiter_active, true));
    CHECK_FALSE(blink(CruiseDisplay::limiter_active, false));
    // Nothing to blink once the set speed is cleared.
    CHECK_FALSE(blink(CruiseDisplay::off, true));
}

TEST_CASE("gear from RPM and speed")
{
    const auto gear = [](int rpm, int kmh) { return view(driving(rpm, kmh * 100, false)).gear; };
    // About 7.6, 13.8, 20.3, 27.5 and 34.4 km/h per 1000 rpm.
    CHECK(gear(3000, 23) == 1);
    CHECK(gear(3000, 41) == 2);
    CHECK(gear(2500, 51) == 3);
    CHECK(gear(2000, 55) == 4);
    CHECK(gear(3200, 120) == 5);
    // Coasting in neutral at idle, and clutch slipping at take-off.
    CHECK_FALSE(gear(800, 60).has_value());
    CHECK_FALSE(gear(3000, 10).has_value());
}

TEST_CASE("no gear with the clutch pressed or nearly stopped")
{
    CHECK_FALSE(view(driving(3000, 4100, true)).gear.has_value());
    CHECK_FALSE(view(driving(3000, 4100, std::nullopt)).gear.has_value());
    CHECK_FALSE(view(driving(900, 400, false)).gear.has_value());
}

TEST_CASE("reverse and neutral come from their switches")
{
    Frames frames = driving(900, 0, false);
    frames.uch_general_status = body();
    frames.uch_general_status->reverse = true;
    CHECK(view(frames).gear == REVERSE);
    frames = driving(900, 0, false);
    frames.upc_fast_frame = UpcFastFrame{NeutralSwitch::in_neutral};
    CHECK(view(frames).gear == NEUTRAL);
    frames.upc_fast_frame = UpcFastFrame{NeutralSwitch::not_equipped};
    CHECK_FALSE(view(frames).gear.has_value());
}

TEST_CASE("gear debounce holds through a brief none")
{
    GearDebounce debounce;
    CHECK(debounce.update(3, false, 0) == 3);
    // A dip shorter than the debounce (clutch blip, mid-shift mismatch) keeps showing the old gear.
    CHECK(debounce.update(std::nullopt, false, 100) == 3);
    CHECK(debounce.update(std::nullopt, false, 300) == 3);
    // The new gear takes over immediately, without waiting out the debounce.
    CHECK(debounce.update(4, false, 350) == 4);
}

TEST_CASE("gear debounce clears after a sustained none")
{
    GearDebounce debounce;
    debounce.update(2, false, 0);
    CHECK(debounce.update(std::nullopt, false, 100) == 2);
    CHECK_FALSE(debounce.update(std::nullopt, false, 500).has_value());
    CHECK_FALSE(debounce.update(std::nullopt, false, 600).has_value());
}

TEST_CASE("gear debounce ignores a neutral blip mid-shift")
{
    GearDebounce debounce;
    debounce.update(2, false, 0);
    // The selector crosses the neutral gate for an instant while changing rails (e.g. 2nd to 3rd).
    CHECK(debounce.update(NEUTRAL, false, 50) == 2);
    CHECK(debounce.update(3, false, 120) == 3);
}

TEST_CASE("gear debounce shows a sustained neutral")
{
    GearDebounce debounce;
    debounce.update(2, false, 0);
    CHECK(debounce.update(NEUTRAL, false, 100) == 2);
    CHECK(debounce.update(NEUTRAL, false, 500) == NEUTRAL);
}

TEST_CASE("gear debounce shows reverse at once")
{
    GearDebounce debounce;
    debounce.update(1, false, 0);
    CHECK(debounce.update(REVERSE, false, 10) == REVERSE);
}

TEST_CASE("gear debounce holds through a slow shift")
{
    GearDebounce debounce;
    debounce.update(2, false, 0);
    // Clutch in, selector through the neutral gate, for longer than the short debounce.
    CHECK(debounce.update(std::nullopt, true, 100) == 2);
    CHECK(debounce.update(NEUTRAL, true, 600) == 2);
    CHECK(debounce.update(std::nullopt, true, 1000) == 2);
    // Clutch released, still slipping: the short debounce counts from the release.
    CHECK(debounce.update(std::nullopt, false, 1200) == 2);
    CHECK(debounce.update(3, false, 1300) == 3);
}

TEST_CASE("gear debounce shows a held clutch, and neutral after the release")
{
    GearDebounce debounce;
    debounce.update(3, false, 0);
    CHECK(debounce.update(std::nullopt, true, 100) == 3);
    CHECK_FALSE(debounce.update(std::nullopt, true, 1700).has_value());
    // Released in neutral: shown the short debounce after the release.
    CHECK_FALSE(debounce.update(NEUTRAL, false, 1800).has_value());
    CHECK(debounce.update(NEUTRAL, false, 2300) == NEUTRAL);
}

TEST_CASE("night and backlight follow the headlights")
{
    const auto lights = [](bool position, bool low, bool high, bool self_test = false) {
        Frames frames;
        frames.upc_general_status = power();
        frames.upc_general_status->position_lamps = position;
        frames.upc_general_status->low_beam = low;
        frames.upc_general_status->high_beam = high;
        if (self_test) {
            frames.cluster_general_status = cluster();
            frames.cluster_general_status->lamp_self_test = true;
        }
        const View v = view(frames);
        return std::pair(v.night, v.backlight_percent);
    };
    // Off or position lamps: day.
    CHECK(lights(false, false, false) == std::pair(false, 100));
    CHECK(lights(true, false, false) == std::pair(false, 100));
    // Low or high beam: night.
    CHECK(lights(true, true, false) == std::pair(true, 70));
    CHECK(lights(false, false, true) == std::pair(true, 70));
    // The lamp self-test lights the high beam on screen, but it is still day.
    CHECK(lights(false, false, false, true) == std::pair(false, 100));
    CHECK(view(Frames{}).backlight_percent == 100);
}

TEST_CASE("trip inputs")
{
    Frames frames;
    frames.uch_general_status = body();
    frames.uch_general_status->scroll_request = ScrollRequest::backward;
    CHECK(view(frames).trip_button);
    CHECK_FALSE(view(frames).distance_counter.has_value());
}

TEST_CASE("oil lamp only with the ignition on")
{
    const auto oil = [](bool apc, bool closed) {
        Frames frames;
        frames.upc_general_status = power();
        frames.upc_general_status->apc_distributed = apc;
        frames.upc_general_status->oil_pressure_switch_closed = closed;
        return view(frames).lamps.oil_pressure;
    };
    CHECK(oil(true, true) == Lamp::on);
    CHECK(oil(true, false) == Lamp::off);
    CHECK(oil(false, true) == Lamp::off);
}

TEST_CASE("headlights show the brightest")
{
    const auto lights = [](bool position, bool low, bool high) {
        Frames frames;
        frames.upc_general_status = power();
        frames.upc_general_status->position_lamps = position;
        frames.upc_general_status->low_beam = low;
        frames.upc_general_status->high_beam = high;
        return view(frames).lamps.headlights;
    };
    CHECK(lights(false, false, false) == Headlights::off);
    CHECK(lights(true, false, false) == Headlights::position);
    CHECK(lights(true, true, false) == Headlights::low_beam);
    CHECK(lights(true, true, true) == Headlights::high_beam);
    CHECK(lights(false, false, true) == Headlights::high_beam);
}

TEST_CASE("turn signals")
{
    const auto signals = [](TurnSignals state) {
        Frames frames;
        frames.uch_general_status = body();
        frames.uch_general_status->turn_signals = state;
        const View v = view(frames);
        return std::pair(v.turn_left, v.turn_right);
    };
    CHECK(signals(TurnSignals::left) == std::pair(true, false));
    CHECK(signals(TurnSignals::both) == std::pair(true, true));
    CHECK(signals(TurnSignals::invalid) == std::pair(false, false));
}
