#include "view.h"

#include <algorithm>
#include <array>
#include <utility>

namespace x84 {

namespace {

// Which state of the oil pressure switch lights the oil lamp. The descriptions only say "switch open" / "switch
// closed": check against the original cluster (ignition on, engine off, the lamp must be lit).
constexpr bool OIL_LAMP_WHEN_SWITCH_CLOSED = true;
// Step of the RPM shown. Rounding keeps the digits from flickering; 50 looked too coarse in the car.
constexpr int RPM_STEP = 25;
// JH3 gearbox of the K4M 1.6 16v, 1st to 5th. 1st to 4th confirmed in the car; 5th showed as 4th with the original
// 0.756 (a JB3-954 catalog value), so it was raised to 0.821 (the JB3-953 value, which agrees with the confirmed
// 4th of 1.029): still check the boundary between 4th and 5th in the car.
constexpr std::array<float, 5> GEAR_RATIOS = {3.727f, 2.048f, 1.393f, 1.029f, 0.821f};
constexpr float FINAL_DRIVE = 4.214f;
constexpr float TIRE_CIRCUMFERENCE_M = 1.985f; // 205/55 R16: 631.9 mm diameter
// Below this wheel speed the gear is not shown: RPM over speed means nothing when nearly stopped.
constexpr int MIN_GEAR_SPEED_CENTI_KMH = 500;
// How long a lost gear (coasting, a mismatch while the clutch engages, or neutral) must persist, counted from the
// clutch being released, before the dashboard shows it.
constexpr uint32_t GEAR_NONE_DEBOUNCE_MS = 400;
// With the clutch pressed, a lost gear or neutral is a shift in progress: the last gear stays on screen this long.
// 400 ms was shorter than a calm shift in the car, which flashed "-" and "N".
constexpr uint32_t GEAR_SHIFT_HOLD_MS = 1500;
// Backlight with the low or high beam on. The cluster's night dimmer (0x645) did not follow the rheostat in the
// car, so the headlights decide.
constexpr int NIGHT_BACKLIGHT_PERCENT = 70;
// How long after the ignition comes on before a missing frame counts: the computers start at their own pace. Not
// in the manual: check in the car that no warning flashes at the start.
constexpr uint32_t IGNITION_SETTLE_MS = 3000;

// Keyless information display. Empty for 00000 = no display, the reserved and undocumented values, 01111 = gearbox
// in P or N (automatic gearbox only) and 11100 = Full HMI - cluster extinction.
std::optional<Notice> keyless_notice(int raw)
{
    switch (raw) {
    case 1: return Notice::welcome;                       // 00001 = welcome
    case 2: return Notice::insert_card;                   // 00010 = Insert card
    case 3:                                               // 00011 = card not detected
    case 6: return Notice::card_not_detected;             // 00110 = Card not detected + buzzer
    case 4:                                               // 00100 = hands-free - failure
    case 5: return Notice::hands_free_fault;              // 00101 = ... - see the manual
    case 7: return Notice::press_stop_twice;              // 00111 = press stop twice
    case 8:                                               // 01000 = card holder failed
    case 22: return Notice::card_reader_fault;            // 10110 = card holder failure
    case 9: return Notice::brake_and_start;               // 01001 = press Brake + start
    case 10:                                              // 01010 = Remove card + buzzer
    case 17: return Notice::remove_card;                  // 10001 = Remove card
    case 11: return Notice::restricted_card_mode;         // 01011 = restricted card mode
    case 14: return Notice::brake_and_start_or_clutch;    // 01110 = press brake + start or declutch
    case 16: return Notice::neutral_or_clutch;            // 10000 = Shift to neutral or declutch
    case 18: return Notice::trunk_open_insert_card;       // 10010 = start with the trunk open -> insert card
    case 19: return Notice::confirm_engine_stop;          // 10011 = confirm engine stop
    case 20: return Notice::starting;                     // 10100 = starting
    case 21: return Notice::pedal_not_held;               // 10101 = pedal not held
    default: return std::nullopt;
    }
}

Lamp lamp(bool on)
{
    return on ? Lamp::on : Lamp::off;
}

// Road speed per 1000 rpm in a gear, in km/h.
float kmh_per_1000_rpm(float ratio)
{
    return 1000.0f / (ratio * FINAL_DRIVE) * TIRE_CIRCUMFERENCE_M * 60.0f / 1000.0f;
}

// The gear whose speed per 1000 rpm is closest to the measured one, on a log scale: the boundary between two gears
// is the geometric mean of their values, so compare squares instead of taking roots. Empty more than half a step
// outside 1st to 5th (clutch slipping, coasting in neutral).
std::optional<int> forward_gear(int rpm, int speed_centi_kmh)
{
    if (rpm == 0) return std::nullopt;
    const float measured = speed_centi_kmh / 100.0f / (rpm / 1000.0f);
    const float squared = measured * measured;
    std::array<float, GEAR_RATIOS.size()> gears;
    std::transform(GEAR_RATIOS.begin(), GEAR_RATIOS.end(), gears.begin(), kmh_per_1000_rpm);
    const float first = gears.front();
    const float last = gears.back();
    if (squared < first * first * first / gears[1] || squared > last * last * last / gears[gears.size() - 2]) {
        return std::nullopt;
    }
    for (size_t i = 0; i + 1 < gears.size(); i++) {
        if (squared < gears[i] * gears[i + 1]) return static_cast<int>(i + 1);
    }
    return static_cast<int>(gears.size());
}

} // namespace

Severity severity(Notice notice)
{
    switch (notice) {
    // As the original cluster's message table (workshop manual 83A-19, 83A-20): Stop, Service, or no lamp.
    case Notice::severe_injection_fault:
    case Notice::steering_fault:
        return Severity::fault;
    case Notice::card_not_detected:
    case Notice::hands_free_fault:
    case Notice::card_reader_fault:
    case Notice::column_not_locked:
    case Notice::column_not_unlocked:
    case Notice::child_lock_fault:
    case Notice::wiper_fault:
    case Notice::injection_fault:
    case Notice::check_steering:
    case Notice::electronic_fault:
        return Severity::service;
    default:
        return Severity::info;
    }
}

uint32_t timeout_ms(uint16_t frame_id)
{
    return frame_id == id::CLUSTER_BACKUP ? 15000 // 3000 ms cycle
                                          : 500;  // 10 to 100 ms cycles
}

std::vector<uint16_t> missing_frames(const Frames &f)
{
    const std::pair<bool, uint16_t> expected[] = {
        {f.ecm_torque_control.has_value(), id::ECM_TORQUE_CONTROL},
        {f.brake_general_data.has_value(), id::BRAKE_GENERAL_DATA},
        {f.ecm_general_status.has_value(), id::ECM_GENERAL_STATUS},
        {f.eps_general_status.has_value(), id::EPS_GENERAL_STATUS},
        {f.uch_general_status.has_value(), id::UCH_GENERAL_STATUS},
        {f.upc_general_status.has_value(), id::UPC_GENERAL_STATUS},
        {f.airbag_general_status.has_value(), id::AIRBAG_GENERAL_STATUS},
    };
    std::vector<uint16_t> missing;
    for (const auto &[present, frame_id] : expected) {
        if (!present) missing.push_back(frame_id);
    }
    return missing;
}

bool IgnitionSettle::update(const Frames &f, uint32_t now_ms)
{
    const bool on = f.upc_general_status ? f.upc_general_status->apc_distributed
                                         : f.ecm_torque_control || f.ecm_general_status || f.brake_general_data ||
                                               f.airbag_general_status || f.eps_general_status;
    if (!on) {
        on_since_ms_.reset();
        return false;
    }
    if (!on_since_ms_) on_since_ms_ = now_ms;
    return now_ms - *on_since_ms_ >= IGNITION_SETTLE_MS;
}

View view(const Frames &f, bool ignition_settled)
{
    View v;
    v.online = f.ecm_torque_control || f.ecm_to_at || f.upc_fast_frame || f.brake_general_data || f.uch_system_control ||
               f.ecm_general_status || f.cluster_general_status || f.eps_general_status || f.uch_general_status ||
               f.upc_general_status || f.cluster_general_status_2 || f.airbag_general_status || f.cluster_backup;

    const auto &engine = f.ecm_torque_control;
    if (engine) v.rpm = (engine->engine_speed_rpm + RPM_STEP / 2) / RPM_STEP * RPM_STEP;
    v.clutch_pressed = engine && engine->clutch_pressed == true;

    // Speed: what the original cluster shows (0x645) when the car sends it; otherwise the wheel speed (0x354).
    std::optional<int> wheel_speed;
    if (const auto &brake = f.brake_general_data) {
        wheel_speed = brake->wheel_speed_centi_kmh;
        v.lamps.abs = lamp(brake->abs_lamp == AbsLamp::fault);
        v.distance_counter = brake->distance_decimeters;
    }
    std::optional<int> displayed_speed;
    // The belt lamp: as the original cluster shows it (0x645), else from the belt switches (0x651).
    SeatBeltLamp cluster_belt = SeatBeltLamp::off;
    if (const auto &cluster = f.cluster_general_status_2) {
        displayed_speed = cluster->displayed_speed_centi_kmh;
        cluster_belt = cluster->seat_belt_lamp;
    }
    if (displayed_speed) v.speed_kmh = *displayed_speed / 100;
    else if (wheel_speed) v.speed_kmh = *wheel_speed / 100;

    // The MOTOR lamp: as the original cluster shows it (0x715, every 3 s), else from the ECU (0x551).
    std::optional<MilLamp> mil;
    bool injection_fault = false;
    bool severe_injection_fault = false;
    if (const auto &ecu = f.ecm_general_status) {
        v.coolant_temp_c = ecu->coolant_temp_c;
        v.coolant_temp_alert = ecu->coolant_temp_alert;
        v.fuel_counter = ecu->fuel_counter;
        mil = ecu->mil;
        switch (ecu->cruise_display) {
        case CruiseDisplay::limiter_active: v.cruise = {CruiseMode::limiter, {}, true, false, false}; break;
        case CruiseDisplay::limiter_suspended: v.cruise = {CruiseMode::limiter, {}, false, false, false}; break;
        case CruiseDisplay::limiter_fault: v.cruise = {CruiseMode::limiter, {}, false, true, false}; break;
        case CruiseDisplay::cruise_active: v.cruise = {CruiseMode::regulator, {}, true, false, false}; break;
        case CruiseDisplay::cruise_suspended: v.cruise = {CruiseMode::regulator, {}, false, false, false}; break;
        case CruiseDisplay::cruise_fault: v.cruise = {CruiseMode::regulator, {}, false, true, false}; break;
        case CruiseDisplay::off:
        case CruiseDisplay::not_equipped: break;
        }
        if (v.cruise.mode != CruiseMode::none) {
            // The car remembers the set speed while suspended; a reset (mode back to none) is what clears it, like
            // the original cluster.
            v.cruise.set_speed_kmh = ecu->set_speed_kmh;
            v.cruise.blink = ecu->set_speed_blink;
        }
        injection_fault = ecu->injection_fault;
        severe_injection_fault = ecu->severe_injection_fault;
    }
    if (const auto &backup = f.cluster_backup; backup && backup->mil) mil = backup->mil;
    v.lamps.engine = !mil || *mil == MilLamp::off ? Lamp::off
                     : *mil == MilLamp::blinking  ? Lamp::blinking
                                                  : Lamp::on; // "autotest voyant": lit while it is tested

    std::optional<Notice> keyless;
    std::optional<Notice> column_lock;
    bool card_battery_low = false;
    bool wiper_fault = false;
    std::optional<bool> reverse;
    if (const auto &body = f.uch_general_status) {
        v.outside_temp_c = body->outside_temp_c;
        v.doors = {body->openings.front_left, body->openings.front_right, body->openings.rear_left,
                   body->openings.rear_right, body->openings.trunk};
        v.turn_left = body->turn_signals == TurnSignals::left || body->turn_signals == TurnSignals::both;
        v.turn_right = body->turn_signals == TurnSignals::right || body->turn_signals == TurnSignals::both;
        v.lamps.rear_fog = lamp(body->rear_fog);
        reverse = body->reverse;
        v.trip_button = body->scroll_request == ScrollRequest::forward || body->scroll_request == ScrollRequest::backward;
        switch (body->column_lock_fault) {
        case ColumnLockFault::stop: v.lamps.stop = Lamp::on; break;
        case ColumnLockFault::service: v.lamps.service = Lamp::on; break;
        case ColumnLockFault::not_locked: column_lock = Notice::column_not_locked; break;
        case ColumnLockFault::not_unlocked: column_lock = Notice::column_not_unlocked; break;
        case ColumnLockFault::none:
        case ColumnLockFault::reserved: break;
        }
        if (body->keyless_display) keyless = keyless_notice(*body->keyless_display);
        card_battery_low = body->card_battery_low == true;
        wiper_fault = body->wiper_fault;
    }

    // With the original display's messages for it (83A-19: Faulty steering, Check steering).
    std::optional<Notice> steering;
    if (const auto &eps = f.eps_general_status) {
        if (eps->steering_fault == SteeringFault::stop) {
            v.lamps.stop = Lamp::on;
            steering = Notice::steering_fault;
        } else if (eps->steering_fault == SteeringFault::service) {
            v.lamps.service = Lamp::on;
            steering = Notice::check_steering;
        }
    }

    if (const auto &power = f.upc_general_status) {
        v.battery_decivolts = power->battery_millivolts / 100;
        v.battery_low = power->battery_low;
        v.lamps.battery = lamp(power->battery_lamp);
        // The CAN stays up with the ignition off, and the switch still reads "no pressure": like the original
        // cluster, the lamp only lights with the ignition on.
        v.lamps.oil_pressure =
            lamp(power->apc_distributed && power->oil_pressure_switch_closed == OIL_LAMP_WHEN_SWITCH_CLOSED);
        v.lamps.headlights = power->high_beam        ? Headlights::high_beam
                             : power->low_beam       ? Headlights::low_beam
                             : power->position_lamps ? Headlights::position
                                                     : Headlights::off;
        v.lamps.front_fog = lamp(power->front_fog);
    }

    bool self_test = false;
    bool fuel_gauge_fault = false;
    if (const auto &cluster = f.cluster_general_status) {
        if (cluster->fuel_gauge == FuelGauge::ok) v.fuel_liters = cluster->fuel_liters;
        fuel_gauge_fault =
            cluster->fuel_gauge == FuelGauge::open_circuit || cluster->fuel_gauge == FuelGauge::short_circuit;
        v.fuel_low = cluster->fuel_low;
        v.lamps.brake = lamp(cluster->parking_brake || cluster->brake_circuit_fault);
        v.lamps.airbag = lamp(cluster->airbag_lamp);
        self_test = cluster->lamp_self_test;
        v.odometer_km = cluster->odometer_km;
    }

    if (const auto &backup = f.cluster_backup) {
        if (backup->oil_alert) {
            v.oil_level = 0;
            v.oil_alert = true;
        } else if (backup->oil_bars) {
            v.oil_level = std::min(*backup->oil_bars, OIL_SEGMENTS);
        }
    }

    // The cluster of this car never lit its belt lamp in the test drive (0x645 stays off): the switches read by the
    // airbag ECU are the fallback, and lit the lamp in the car. The passenger only counts when the seat sensor
    // detects someone, like the original cluster.
    bool unbuckled = false;
    if (const auto &airbag = f.airbag_general_status) {
        const bool passenger = airbag->passenger_on_seat == true && airbag->passenger_unbuckled == true;
        unbuckled = airbag->driver_unbuckled == true || passenger;
    }
    v.lamps.seat_belt = cluster_belt == SeatBeltLamp::blinking    ? Lamp::blinking
                        : cluster_belt == SeatBeltLamp::on || unbuckled ? Lamp::on
                                                                        : Lamp::off;

    // STOP and SERVICE along with a system's own lamp or message, as the original cluster lights them (workshop
    // manual 83A-18 to 83A-20, 17B, 38C, 87G; docs/decisoes.md). The brake circuit incident is the brake fluid
    // level, which the original cluster reads by wire.
    const bool child_lock_fault = f.uch_system_control && f.uch_system_control->child_lock_fault == true;
    const bool abs_fault = f.brake_general_data && f.brake_general_data->abs_lamp == AbsLamp::fault;
    const bool brake_fluid_low = f.cluster_general_status && f.cluster_general_status->brake_circuit_fault;
    if (severe_injection_fault || brake_fluid_low || v.lamps.oil_pressure == Lamp::on || v.lamps.battery == Lamp::on) {
        v.lamps.stop = Lamp::on;
    }
    if (injection_fault || abs_fault || v.lamps.airbag == Lamp::on || v.oil_alert || fuel_gauge_fault || column_lock ||
        child_lock_fault || wiper_fault || (keyless && severity(*keyless) == Severity::service)) {
        v.lamps.service = Lamp::on;
    }

    // A missing frame, with the ignition settled, lights what the original cluster lights for it (83A-22, 83A-23),
    // and only that: the lamps of a missing injection frame do not bring STOP along. Its values stay empty ("--")
    // instead of the original's needles at 0, and the made-up states (doors open, turn signals on with the UCH
    // missing) are left out.
    bool electronic_fault = false;
    if (ignition_settled) {
        if (!f.airbag_general_status) {
            v.lamps.airbag = Lamp::on;
            v.lamps.service = Lamp::on;
        }
        if (!f.brake_general_data) {
            v.lamps.brake = Lamp::on;
            v.lamps.abs = Lamp::on;
        }
        if (!f.ecm_torque_control) {
            electronic_fault = true;
            v.lamps.oil_pressure = Lamp::on;
            v.lamps.battery = Lamp::on;
            v.coolant_temp_alert = true;
        }
        if (!f.ecm_general_status || !f.upc_general_status) electronic_fault = true;
        if (!f.eps_general_status) {
            v.lamps.service = Lamp::on;
            steering = Notice::steering_fault;
        }
        if (!f.uch_general_status) v.lamps.service = Lamp::on;
        if (electronic_fault) v.lamps.service = Lamp::on;
    }

    // The original display's message (0x60D VSC) is what the driver needs right now (start, remove the card): only a
    // fault that stops the car comes before it. The lasting service faults come after.
    if (severe_injection_fault) v.notice = Notice::severe_injection_fault;
    else if (steering == Notice::steering_fault) v.notice = steering;
    else if (keyless) v.notice = keyless;
    else if (injection_fault) v.notice = Notice::injection_fault;
    else if (steering) v.notice = steering;
    else if (electronic_fault) v.notice = Notice::electronic_fault;
    else if (column_lock) v.notice = column_lock;
    else if (child_lock_fault) v.notice = Notice::child_lock_fault;
    else if (wiper_fault) v.notice = Notice::wiper_fault;
    else if (card_battery_low) v.notice = Notice::card_battery_low;

    if (reverse == true) {
        v.gear = REVERSE;
    } else if (f.upc_fast_frame && f.upc_fast_frame->neutral == NeutralSwitch::in_neutral) {
        v.gear = NEUTRAL;
    } else if (engine && wheel_speed && engine->clutch_pressed == false && *wheel_speed >= MIN_GEAR_SPEED_CENTI_KMH) {
        v.gear = forward_gear(engine->engine_speed_rpm, *wheel_speed);
    }

    // From the lamps the car switched on, before the self-test below lights them all.
    v.night = v.lamps.headlights == Headlights::low_beam || v.lamps.headlights == Headlights::high_beam;
    v.backlight_percent = v.night ? NIGHT_BACKLIGHT_PERCENT : 100;

    // The original cluster tests its lamps: light them all, in step with it.
    if (self_test) {
        v.lamps = {Headlights::high_beam, Lamp::on, Lamp::on, Lamp::on, Lamp::on, Lamp::on,
                   Lamp::on,              Lamp::on, Lamp::on, Lamp::on, Lamp::on, Lamp::on};
    }
    return v;
}

std::optional<int> GearDebounce::update(std::optional<int> gear, bool clutch_pressed, uint32_t now_ms)
{
    if (clutch_pressed) clutch_pressed_at_ms_ = now_ms;
    if (gear && *gear != NEUTRAL) {
        unsettled_since_ms_.reset();
        displayed_ = gear;
        return displayed_;
    }
    if (!unsettled_since_ms_) unsettled_since_ms_ = now_ms;
    bool settled;
    if (clutch_pressed) {
        settled = now_ms - *unsettled_since_ms_ >= GEAR_SHIFT_HOLD_MS;
    } else {
        // Counted from the later of the start and the clutch release.
        const uint32_t from = std::max(*unsettled_since_ms_, clutch_pressed_at_ms_.value_or(0));
        settled = now_ms - from >= GEAR_NONE_DEBOUNCE_MS;
    }
    if (settled) displayed_ = gear;
    return displayed_;
}

} // namespace x84
