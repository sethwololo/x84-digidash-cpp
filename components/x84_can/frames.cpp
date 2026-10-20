#include "frames.h"

namespace x84 {

using namespace signal;

namespace {

// Two-bit "no / yes / reserved / invalid" signals: 00 = no, 01 = yes, anything else = nothing.
std::optional<bool> yes_no(uint32_t value)
{
    if (value == 0) return false;
    if (value == 1) return true;
    return std::nullopt;
}

// Every temperature signal used here: raw 0 is -40 °C, 1 °C per unit.
int celsius(uint32_t value)
{
    return static_cast<int>(value) - 40;
}

std::optional<bool> flag(std::optional<uint32_t> value)
{
    if (!value) return std::nullopt;
    return *value != 0;
}

MilLamp mil_lamp(uint32_t value)
{
    constexpr MilLamp LAMPS[] = {MilLamp::off, MilLamp::on, MilLamp::blinking, MilLamp::self_test};
    return LAMPS[value & 3];
}

} // namespace

std::optional<EcmTorqueControl> EcmTorqueControl::decode(Data data)
{
    const auto speed = ENGINE_SPEED.raw(data);
    const auto clutch = CLUTCH.raw(data);
    if (!speed || !clutch) return std::nullopt;
    return EcmTorqueControl{static_cast<int>(*speed / 8), yes_no(*clutch)};
}

std::optional<EcmToAt> EcmToAt::decode(Data data)
{
    constexpr CruiseState STATES[] = {CruiseState::inactive, CruiseState::pedal_override, CruiseState::active,
                                      CruiseState::reserved};
    const auto state = CRUISE_STATE.raw(data);
    if (!state) return std::nullopt;
    return EcmToAt{STATES[*state]};
}

std::optional<UpcFastFrame> UpcFastFrame::decode(Data data)
{
    constexpr NeutralSwitch SWITCHES[] = {NeutralSwitch::not_in_neutral, NeutralSwitch::in_neutral,
                                          NeutralSwitch::not_equipped, NeutralSwitch::invalid};
    const auto neutral = NEUTRAL.raw(data);
    if (!neutral) return std::nullopt;
    return UpcFastFrame{SWITCHES[*neutral]};
}

std::optional<BrakeGeneralData> BrakeGeneralData::decode(Data data)
{
    constexpr AbsLamp LAMPS[] = {AbsLamp::off, AbsLamp::fault, AbsLamp::not_programmed, AbsLamp::calibration};
    const auto abs = ABS_LAMP.raw(data);
    const auto distance = DISTANCE.raw(data);
    const auto speed = WHEEL_SPEED.raw(data);
    if (!abs || !distance || !speed) return std::nullopt;
    return BrakeGeneralData{LAMPS[*abs], static_cast<uint16_t>(*distance), static_cast<uint16_t>(*speed)};
}

UchSystemControl UchSystemControl::decode(Data data)
{
    const auto wear = BRAKE_PAD_WEAR.raw(data);
    return UchSystemControl{wear ? std::optional<int>(static_cast<int>(*wear)) : std::nullopt,
                            flag(CHILD_LOCK_FAULT.raw(data))};
}

std::optional<EcmGeneralStatus> EcmGeneralStatus::decode(Data data)
{
    constexpr CruiseDisplay DISPLAYS[] = {
        CruiseDisplay::off,           CruiseDisplay::limiter_active,   CruiseDisplay::limiter_suspended,
        CruiseDisplay::limiter_fault, CruiseDisplay::cruise_active,    CruiseDisplay::cruise_suspended,
        CruiseDisplay::cruise_fault,  CruiseDisplay::not_equipped,
    };
    const auto coolant = COOLANT_TEMP.raw(data);
    const auto coolant_alert = COOLANT_TEMP_ALERT.raw(data);
    const auto mil = MIL.raw(data);
    const auto set_speed = SET_SPEED.raw(data);
    const auto blink = SET_SPEED_BLINK.raw(data);
    const auto cruise = CRUISE_DISPLAY.raw(data);
    const auto fuel = FUEL_CONSUMPTION.raw(data);
    const auto injection = INJECTION_FAULT.raw(data);
    const auto severe = SEVERE_INJECTION_FAULT.raw(data);
    if (!coolant || !coolant_alert || !mil || !set_speed || !blink || !cruise || !fuel || !injection || !severe) {
        return std::nullopt;
    }
    return EcmGeneralStatus{
        .coolant_temp_c = celsius(*coolant),
        .coolant_temp_alert = *coolant_alert != 0,
        .mil = mil_lamp(*mil),
        .set_speed_kmh = static_cast<int>(*set_speed),
        .set_speed_blink = *blink != 0,
        .cruise_display = DISPLAYS[*cruise],
        .fuel_counter = static_cast<uint8_t>(*fuel),
        .injection_fault = *injection != 0,
        .severe_injection_fault = *severe != 0,
    };
}

std::optional<ClusterGeneralStatus> ClusterGeneralStatus::decode(Data data)
{
    constexpr FuelGauge GAUGES[] = {FuelGauge::ok, FuelGauge::open_circuit, FuelGauge::short_circuit, FuelGauge::reserved};
    const auto fuel = FUEL_LEVEL.raw(data);
    const auto gauge = FUEL_GAUGE.raw(data);
    const auto low = FUEL_LOW.raw(data);
    const auto parking = PARKING_BRAKE.raw(data);
    const auto circuit = BRAKE_CIRCUIT.raw(data);
    const auto airbag = AIRBAG_LAMP.raw(data);
    const auto self_test = LAMP_SELF_TEST.raw(data);
    if (!fuel || !gauge || !low || !parking || !circuit || !airbag || !self_test) return std::nullopt;
    return ClusterGeneralStatus{
        .fuel_liters = static_cast<int>(*fuel),
        .fuel_gauge = GAUGES[*gauge],
        .fuel_low = *low != 0,
        .parking_brake = *parking != 0,
        .brake_circuit_fault = *circuit != 0,
        .airbag_lamp = *airbag != 0,
        .lamp_self_test = *self_test != 0,
        .odometer_km = ODOMETER.raw(data),
    };
}

std::optional<EpsGeneralStatus> EpsGeneralStatus::decode(Data data)
{
    constexpr SteeringFault FAULTS[] = {SteeringFault::none, SteeringFault::service, SteeringFault::stop,
                                        SteeringFault::diagnostic};
    const auto fault = STEERING_FAULT.raw(data);
    if (!fault) return std::nullopt;
    return EpsGeneralStatus{FAULTS[*fault]};
}

std::optional<UchGeneralStatus> UchGeneralStatus::decode(Data data)
{
    constexpr TurnSignals TURNS[] = {TurnSignals::off,     TurnSignals::left,    TurnSignals::right,   TurnSignals::both,
                                     TurnSignals::invalid, TurnSignals::invalid, TurnSignals::invalid, TurnSignals::invalid};
    constexpr ColumnLockFault LOCKS[] = {
        ColumnLockFault::none,         ColumnLockFault::stop,     ColumnLockFault::not_locked, ColumnLockFault::reserved,
        ColumnLockFault::not_unlocked, ColumnLockFault::reserved, ColumnLockFault::service,    ColumnLockFault::reserved,
    };
    constexpr ScrollRequest SCROLLS[] = {ScrollRequest::none, ScrollRequest::forward, ScrollRequest::backward,
                                         ScrollRequest::invalid};
    const auto openings = OPENINGS.raw(data);
    const auto turns = TURN_SIGNALS.raw(data);
    const auto rear_fog = REAR_FOG.raw(data);
    const auto wiper = WIPER_FAULT.raw(data);
    const auto lock = COLUMN_LOCK_FAULT.raw(data);
    const auto outside = OUTSIDE_TEMP.raw(data);
    const auto reverse = REVERSE.raw(data);
    const auto scroll = SCROLL.raw(data);
    if (!openings || !turns || !rear_fog || !wiper || !lock || !outside || !reverse || !scroll) return std::nullopt;
    const auto keyless = KEYLESS_DISPLAY.raw(data);
    return UchGeneralStatus{
        .openings = {
            .front_left = (*openings & 1) != 0,
            .front_right = (*openings & 2) != 0,
            .rear_left = (*openings & 4) != 0,
            .rear_right = (*openings & 8) != 0,
            .trunk = (*openings & 16) != 0,
        },
        .turn_signals = TURNS[*turns],
        .rear_fog = *rear_fog != 0,
        .wiper_fault = *wiper != 0,
        .column_lock_fault = LOCKS[*lock],
        .outside_temp_c = celsius(*outside),
        .reverse = yes_no(*reverse),
        .scroll_request = SCROLLS[*scroll],
        .card_battery_low = flag(CARD_BATTERY_LOW.raw(data)),
        .keyless_display = keyless ? std::optional<int>(static_cast<int>(*keyless)) : std::nullopt,
    };
}

std::optional<UpcGeneralStatus> UpcGeneralStatus::decode(Data data)
{
    const auto battery = BATTERY_VOLTAGE.raw(data);
    const auto oil = OIL_PRESSURE_SWITCH.raw(data);
    const auto lamp = BATTERY_LAMP.raw(data);
    const auto low = BATTERY_LOW.raw(data);
    const auto high = HIGH_BEAM.raw(data);
    const auto fog = FRONT_FOG.raw(data);
    const auto low_beam = LOW_BEAM.raw(data);
    const auto position = POSITION_LAMPS.raw(data);
    const auto apc = APC_DISTRIBUTED.raw(data);
    if (!battery || !oil || !lamp || !low || !high || !fog || !low_beam || !position || !apc) return std::nullopt;
    return UpcGeneralStatus{
        .battery_millivolts = static_cast<int>(*battery * 125 / 2),
        .oil_pressure_switch_closed = *oil != 0,
        .battery_lamp = *lamp != 0,
        .battery_low = *low != 0,
        .high_beam = *high != 0,
        .front_fog = *fog != 0,
        .low_beam = *low_beam != 0,
        .position_lamps = *position != 0,
        .apc_distributed = *apc != 0,
    };
}

std::optional<ClusterGeneralStatus2> ClusterGeneralStatus2::decode(Data data)
{
    constexpr SeatBeltLamp BELTS[] = {SeatBeltLamp::off, SeatBeltLamp::on, SeatBeltLamp::blinking, SeatBeltLamp::reserved};
    const auto dimmer = NIGHT_DIMMER.raw(data);
    const auto position = CLUSTER_POSITION_LAMPS.raw(data);
    const auto belt = SEAT_BELT_LAMP.raw(data);
    if (!dimmer || !position || !belt) return std::nullopt;
    const auto speed = DISPLAYED_SPEED.raw(data);
    return ClusterGeneralStatus2{
        .displayed_speed_centi_kmh = speed ? std::optional<int>(static_cast<int>(*speed)) : std::nullopt,
        .night_dimmer_permille = static_cast<int>(*dimmer * 4),
        .position_lamps = *position != 0,
        .seat_belt_lamp = BELTS[*belt],
    };
}

std::optional<AirbagGeneralStatus> AirbagGeneralStatus::decode(Data data)
{
    const auto driver = DRIVER_BELT.raw(data);
    const auto seat = PASSENGER_ON_SEAT.raw(data);
    const auto passenger = PASSENGER_BELT.raw(data);
    if (!driver || !seat || !passenger) return std::nullopt;
    const auto absent = yes_no(*seat);
    return AirbagGeneralStatus{
        .driver_unbuckled = yes_no(*driver),
        .passenger_on_seat = absent ? std::optional<bool>(!*absent) : std::nullopt,
        .passenger_unbuckled = yes_no(*passenger),
    };
}

std::optional<ClusterBackup> ClusterBackup::decode(Data data)
{
    const auto oil = OIL_LEVEL.raw(data);
    if (!oil) return std::nullopt;
    const auto mil = CLUSTER_MIL.raw(data);
    const auto range = OIL_SERVICE_RANGE.raw(data);
    return ClusterBackup{
        .oil_alert = *oil == 0,
        .oil_bars = *oil >= 1 && *oil <= 8 ? std::optional<int>(static_cast<int>(*oil)) : std::nullopt,
        .mil = mil ? std::optional<MilLamp>(mil_lamp(*mil)) : std::nullopt,
        .oil_service_range_km = range ? std::optional<int>(static_cast<int>(*range * 250)) : std::nullopt,
    };
}

namespace {

// Stores a decoded frame in its slot, if it decoded.
template <typename Frame>
bool keep(std::optional<Frame> &slot, std::optional<Frame> decoded)
{
    if (!decoded) return false;
    slot = decoded;
    return true;
}

} // namespace

bool Frames::store(uint16_t frame_id, Data data)
{
    switch (frame_id) {
    case id::ECM_TORQUE_CONTROL: return keep(ecm_torque_control, EcmTorqueControl::decode(data));
    case id::ECM_TO_AT: return keep(ecm_to_at, EcmToAt::decode(data));
    case id::UPC_FAST_FRAME: return keep(upc_fast_frame, UpcFastFrame::decode(data));
    case id::BRAKE_GENERAL_DATA: return keep(brake_general_data, BrakeGeneralData::decode(data));
    case id::UCH_SYSTEM_CONTROL: return keep(uch_system_control, std::optional(UchSystemControl::decode(data)));
    case id::ECM_GENERAL_STATUS: return keep(ecm_general_status, EcmGeneralStatus::decode(data));
    case id::CLUSTER_GENERAL_STATUS: return keep(cluster_general_status, ClusterGeneralStatus::decode(data));
    case id::EPS_GENERAL_STATUS: return keep(eps_general_status, EpsGeneralStatus::decode(data));
    case id::UCH_GENERAL_STATUS: return keep(uch_general_status, UchGeneralStatus::decode(data));
    case id::UPC_GENERAL_STATUS: return keep(upc_general_status, UpcGeneralStatus::decode(data));
    case id::CLUSTER_GENERAL_STATUS_2: return keep(cluster_general_status_2, ClusterGeneralStatus2::decode(data));
    case id::AIRBAG_GENERAL_STATUS: return keep(airbag_general_status, AirbagGeneralStatus::decode(data));
    case id::CLUSTER_BACKUP: return keep(cluster_backup, ClusterBackup::decode(data));
    default: return false;
    }
}

void Frames::clear(uint16_t frame_id)
{
    switch (frame_id) {
    case id::ECM_TORQUE_CONTROL: ecm_torque_control.reset(); break;
    case id::ECM_TO_AT: ecm_to_at.reset(); break;
    case id::UPC_FAST_FRAME: upc_fast_frame.reset(); break;
    case id::BRAKE_GENERAL_DATA: brake_general_data.reset(); break;
    case id::UCH_SYSTEM_CONTROL: uch_system_control.reset(); break;
    case id::ECM_GENERAL_STATUS: ecm_general_status.reset(); break;
    case id::CLUSTER_GENERAL_STATUS: cluster_general_status.reset(); break;
    case id::EPS_GENERAL_STATUS: eps_general_status.reset(); break;
    case id::UCH_GENERAL_STATUS: uch_general_status.reset(); break;
    case id::UPC_GENERAL_STATUS: upc_general_status.reset(); break;
    case id::CLUSTER_GENERAL_STATUS_2: cluster_general_status_2.reset(); break;
    case id::AIRBAG_GENERAL_STATUS: airbag_general_status.reset(); break;
    case id::CLUSTER_BACKUP: cluster_backup.reset(); break;
    default: break;
    }
}

} // namespace x84
