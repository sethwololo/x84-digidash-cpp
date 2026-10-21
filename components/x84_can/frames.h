#pragma once

// Decoders for the CAN frames of the Renault Mégane II (X84) that the dashboard reads: one struct per frame, holding
// only the signals the dashboard uses. Signal positions and value tables come from can/signals-x84.json (a test
// checks every position against it), quoted in the comments. Only what the descriptions state is decoded:
// undocumented or reserved values map to an `invalid` or `reserved` value, never to a guessed meaning. Values are
// kept in integer units close to the raw ones. A decoder returns nothing for a frame too short for its signals.

#include <cstdint>
#include <optional>
#include <span>

#include "signal.h"

namespace x84 {

// Identifiers of the frames decoded here.
namespace id {
constexpr uint16_t ECM_TORQUE_CONTROL = 0x181;
constexpr uint16_t ECM_TO_AT = 0x161;
constexpr uint16_t UPC_FAST_FRAME = 0x215;
constexpr uint16_t BRAKE_GENERAL_DATA = 0x354;
constexpr uint16_t UCH_SYSTEM_CONTROL = 0x35D;
constexpr uint16_t CLIM_CONTROL_FRAME = 0x374;
constexpr uint16_t ECM_GENERAL_STATUS = 0x551;
constexpr uint16_t CLUSTER_GENERAL_STATUS = 0x5C5;
constexpr uint16_t EPS_GENERAL_STATUS = 0x5E4;
constexpr uint16_t UCH_GENERAL_STATUS = 0x60D;
constexpr uint16_t UPC_GENERAL_STATUS = 0x625;
constexpr uint16_t CLUSTER_GENERAL_STATUS_2 = 0x645;
constexpr uint16_t AIRBAG_GENERAL_STATUS = 0x651;
constexpr uint16_t CLUSTER_BACKUP = 0x715;

// Every identifier decoded here.
constexpr uint16_t ALL[] = {
    ECM_TORQUE_CONTROL, ECM_TO_AT,          UPC_FAST_FRAME,     BRAKE_GENERAL_DATA,      UCH_SYSTEM_CONTROL,
    ECM_GENERAL_STATUS, CLUSTER_GENERAL_STATUS, EPS_GENERAL_STATUS, UCH_GENERAL_STATUS, UPC_GENERAL_STATUS,
    CLUSTER_GENERAL_STATUS_2, AIRBAG_GENERAL_STATUS, CLUSTER_BACKUP, CLIM_CONTROL_FRAME,
};
} // namespace id

// Signal positions, named after the frame (tests/frames_test.cpp checks them against the JSON).
namespace signal {
// 0x181 ECM_TorqueControl
constexpr Signal ENGINE_SPEED{7, 16};
constexpr Signal CLUTCH{44, 2};
// 0x161 ECM_to_AT
constexpr Signal CRUISE_STATE{31, 2};
// 0x215 UPC_Fast_Frame
constexpr Signal NEUTRAL{13, 2};
// 0x354 Brake_GeneralData
constexpr Signal ABS_LAMP{36, 2};
constexpr Signal DISTANCE{23, 16};
constexpr Signal WHEEL_SPEED{7, 16};
// 0x35D UCH_SystemControl
constexpr Signal BRAKE_PAD_WEAR{47, 2}; // only in the complementary signal database
constexpr Signal CHILD_LOCK_FAULT{41, 1};
// 0x374 CLIM_ControlFrame
constexpr Signal EVAPORATOR_TEMP{23, 8};
// 0x551 ECM_GeneralStatus
constexpr Signal COOLANT_TEMP{7, 8};
constexpr Signal COOLANT_TEMP_ALERT{28, 1};
constexpr Signal MIL{30, 2};
constexpr Signal SET_SPEED{39, 8};
constexpr Signal SET_SPEED_BLINK{47, 1};
constexpr Signal CRUISE_DISPLAY{46, 3};
constexpr Signal FUEL_CONSUMPTION{15, 8};
constexpr Signal INJECTION_FAULT{26, 1};
constexpr Signal SEVERE_INJECTION_FAULT{25, 1};
// 0x5C5 CLUSTER_GeneralStatus
constexpr Signal FUEL_LEVEL{62, 7};
constexpr Signal FUEL_GAUGE{1, 2};
constexpr Signal FUEL_LOW{5, 1};
constexpr Signal PARKING_BRAKE{2, 1};
constexpr Signal BRAKE_CIRCUIT{3, 1};
constexpr Signal AIRBAG_LAMP{36, 1};
constexpr Signal LAMP_SELF_TEST{54, 1};
constexpr Signal ODOMETER{15, 24};
// 0x5E4 EPS_GeneralStatus
constexpr Signal STEERING_FAULT{2, 2};
// 0x60D UCH_GeneralStatus
constexpr Signal OPENINGS{7, 5};
constexpr Signal TURN_SIGNALS{15, 3};
constexpr Signal REAR_FOG{18, 1};
constexpr Signal WIPER_FAULT{16, 1};
constexpr Signal COLUMN_LOCK_FAULT{23, 3};
constexpr Signal OUTSIDE_TEMP{39, 8};
constexpr Signal REVERSE{53, 2};
constexpr Signal SCROLL{57, 2};
constexpr Signal CARD_BATTERY_LOW{58, 1};
constexpr Signal KEYLESS_DISPLAY{63, 5};
// 0x625 UPC_GeneralStatus
constexpr Signal BATTERY_VOLTAGE{23, 8};
constexpr Signal OIL_PRESSURE_SWITCH{31, 1};
constexpr Signal BATTERY_LAMP{29, 1};
constexpr Signal BATTERY_LOW{6, 1};
constexpr Signal HIGH_BEAM{12, 1};
constexpr Signal FRONT_FOG{11, 1};
constexpr Signal LOW_BEAM{13, 1};
constexpr Signal APC_DISTRIBUTED{7, 1};
constexpr Signal POSITION_LAMPS{14, 1};
// 0x645 CLUSTER_GeneralStatus_2
constexpr Signal DISPLAYED_SPEED{31, 16};
constexpr Signal NIGHT_DIMMER{15, 8};
constexpr Signal CLUSTER_POSITION_LAMPS{5, 1}; // only in the complementary signal database
constexpr Signal SEAT_BELT_LAMP{7, 2};
// 0x651 AIRBAG_GeneralStatus
constexpr Signal DRIVER_BELT{9, 2};
constexpr Signal PASSENGER_ON_SEAT{13, 2};
constexpr Signal PASSENGER_BELT{15, 2};
// 0x715 CLUSTER_BackUp
constexpr Signal OIL_LEVEL{29, 4};
constexpr Signal CLUSTER_MIL{31, 2};
constexpr Signal OIL_SERVICE_RANGE{47, 8};
} // namespace signal

using Data = std::span<const uint8_t>;

// Cruise control and speed limiter states: 00 = cruise control and speed limiter inactive (OFF, standby or
// suspended), 01 = pedal resume, 10 = regulation or limitation active, 11 = reserved.
enum class CruiseState { inactive, pedal_override, active, reserved };
// Neutral: 00 = neutral position not reached, 01 = reached, 10 = vehicle not equipped, 11 = invalid.
enum class NeutralSwitch { not_in_neutral, in_neutral, not_equipped, invalid };
// ABS lamp state: 00 = ABS operational or vehicle not equipped (lamp off), 01 = ABS fault, 10 = ABS operational
// (not programmed), 11 = ABS calibrating (diagnostic mode).
enum class AbsLamp { off, fault, not_programmed, calibration };
// Emissions fault lamp (0x551) and MIL lamp state (0x715): 00 = MIL lamp off, 01 = MIL lamp steady on,
// 10 = MIL lamp blinking, 11 = lamp self-test.
enum class MilLamp { off, on, blinking, self_test };
// Cruise/limiter state display: 000 = cruise control and speed limiter OFF, 001 = limiter active, 010 = (limiter
// ON and inactive) or suspended, 011 = limiter requested and faulty, 100 to 110 the same for the cruise control,
// 111 = cruise control/limiter functions not present on the vehicle.
enum class CruiseDisplay {
    off,
    limiter_active,
    limiter_suspended,
    limiter_fault,
    cruise_active,
    cruise_suspended,
    cruise_fault,
    not_equipped,
};
// Fuel gauge diagnostic: 00 = gauge connection OK, 01 = gauge open circuit, 10 = gauge short circuit, 11 = reserved.
enum class FuelGauge { ok, open_circuit, short_circuit, reserved };
// DAE fault: 00 = normal, 01 = DAE minor fault -> Service, 10 = DAE major fault -> stop, 11 = diagnostic mode.
enum class SteeringFault { none, service, stop, diagnostic };
// Turn signal states: 000 = no turn signal active, 001 = left ON and right OFF, 010 = left OFF and right ON,
// 011 = left ON and right ON; 111 = invalid, and the undocumented 100 to 110.
enum class TurnSignals { off, left, right, both, invalid };
// Column lock or UCH fault: 000 = no lock fault, 001 = UCH/STOP electronic failure, 010 = locking not done (after
// engine stop), 100 = unlocking not done (before start), 110 = UCH/SERV electronic failure; 011 and 111 = reserved,
// and the undocumented 101.
enum class ColumnLockFault { none, stop, not_locked, not_unlocked, service, reserved };
// Trip computer scroll request (its button): 00 = not pressed, 01 = scroll + direction, 10 = scroll - direction,
// 11 = invalid.
enum class ScrollRequest { none, forward, backward, invalid };
// Seat belt lamp state: 00 = lamp off (seat belt fastened), 01 = lamp on (seat belt not fastened), 10 = lamp
// blinking (seat belt not fastened and V > 15 km/h and timer < 10 s), 11 = reserved.
enum class SeatBeltLamp { off, on, blinking, reserved };

// 0x181 ECM_TorqueControl (10 ms).
struct EcmTorqueControl {
    int engine_speed_rpm;              // Engine speed, 0.125 rpm per unit, truncated to whole rpm
    std::optional<bool> clutch_pressed; // Clutch switch: 00 = foot not pressed, 01 = pressed
    static std::optional<EcmTorqueControl> decode(Data data);
};

// 0x161 ECM_to_AT (10 ms).
struct EcmToAt {
    CruiseState cruise_state;
    static std::optional<EcmToAt> decode(Data data);
};

// 0x215 UPC_Fast_Frame (20 ms).
struct UpcFastFrame {
    NeutralSwitch neutral;
    static std::optional<UpcFastFrame> decode(Data data);
};

// 0x354 Brake_GeneralData (40 ms).
struct BrakeGeneralData {
    AbsLamp abs_lamp;
    uint16_t distance_decimeters;  // Distance travelled: rolling counter, 0.1 m per unit
    uint16_t wheel_speed_centi_kmh; // Vehicle speed (front wheel average), 0.01 km/h per unit
    static std::optional<BrakeGeneralData> decode(Data data);
};

// 0x35D UCH_SystemControl (100 ms). Always decodes: each signal is empty when the frame is too short for it.
struct UchSystemControl {
    // BrakePadWear (complementary database), raw: 0 = no fault, 1 = short circuit 12 V, 2 = pad to be changed. For
    // testing.
    std::optional<int> brake_pad_wear;
    std::optional<bool> child_lock_fault; // Child lock fault alert: 1 = alert
    static UchSystemControl decode(Data data);
};

// 0x374 CLIM_ControlFrame (50 ms), the air conditioning.
struct ClimControlFrame {
    // Evaporator temperature, 0.5 °C per unit from -40 °C, in tenths of a degree; empty for 0xFF, which the car
    // sends while it switches off (seen in the car).
    std::optional<int> evaporator_temp_tenths;
    static std::optional<ClimControlFrame> decode(Data data);
};

// 0x551 ECM_GeneralStatus (100 ms).
struct EcmGeneralStatus {
    int coolant_temp_c;       // Engine coolant temperature
    bool coolant_temp_alert;  // Coolant temperature alert
    MilLamp mil;              // Emissions fault lamp
    int set_speed_kmh;        // Set speed (cruise control or speed limiter)
    bool set_speed_blink;     // Cruise/limiter set speed blinking: 1 = blinking display
    CruiseDisplay cruise_display;
    // Fuel consumption: rolling counter of the fuel injected, 80 mm³ per unit, wrapping after 255 (every
    // 20.48 mL). Checked in the car: it only goes up, and wraps.
    uint8_t fuel_counter;
    bool injection_fault;        // Injection fault severity 1
    bool severe_injection_fault; // Injection fault severity 2, linked to the engine stop
    static std::optional<EcmGeneralStatus> decode(Data data);
};

// 0x5C5 CLUSTER_GeneralStatus (100 ms), sent by the original instrument cluster.
struct ClusterGeneralStatus {
    int fuel_liters; // Fuel level
    FuelGauge fuel_gauge;
    bool fuel_low;            // Low fuel alert: 1 = minimum reached
    bool parking_brake;       // Parking brake switch: 1 = parking brake applied
    bool brake_circuit_fault; // Brake circuit fault
    bool airbag_lamp;         // AIRBAG fault lamp state: 1 = lamp steady on
    bool lamp_self_test;      // Lamp self-test driven by the cluster: 1 = in progress
    std::optional<uint32_t> odometer_km; // Odometer; empty if the frame is too short
    static std::optional<ClusterGeneralStatus> decode(Data data);
};

// 0x5E4 EPS_GeneralStatus (100 ms), electric power steering (DAE).
struct EpsGeneralStatus {
    SteeringFault steering_fault;
    static std::optional<EpsGeneralStatus> decode(Data data);
};

// Openings state: one bit per opening, true = open (xxxx1 = front left door, xxx1x = front right, xx1xx = rear left,
// x1xxx = rear right, 1xxxx = trunk, tailgate or opening rear window).
struct Openings {
    bool front_left = false;
    bool front_right = false;
    bool rear_left = false;
    bool rear_right = false;
    bool trunk = false;
    bool operator==(const Openings &) const = default;
};

// 0x60D UCH_GeneralStatus (100 ms), body control unit.
struct UchGeneralStatus {
    Openings openings;
    TurnSignals turn_signals; // they follow the lamps: the UCH toggles them at the blinking rate
    bool rear_fog;            // Rear fog light state: 1 = fog light ON
    bool wiper_fault;         // Wiping fault
    ColumnLockFault column_lock_fault;
    int outside_temp_c;           // Outside temperature
    std::optional<bool> reverse; // Reverse gear engaged: 00 = not engaged, 01 = engaged
    ScrollRequest scroll_request;
    std::optional<bool> card_battery_low; // Card battery to replace; empty if the frame is too short, as the next one
    // Keyless information display, raw: the message on the original cluster's display, mostly about the keyless
    // card. The table is in view.cpp.
    std::optional<int> keyless_display;
    static std::optional<UchGeneralStatus> decode(Data data);
};

// 0x625 UPC_GeneralStatus (100 ms), engine compartment fuse box.
struct UpcGeneralStatus {
    int battery_millivolts;            // Battery voltage by UPC, 0.0625 V per unit
    bool oil_pressure_switch_closed;   // Oil pressure switch: 0 = switch open, 1 = switch closed
    bool battery_lamp;                 // Battery lamp: 1 = battery lamp on
    bool battery_low;                  // Low battery alert: 1 = battery voltage below the threshold
    bool high_beam;                    // High beam state: 1 = high beam on
    bool front_fog;                    // Front fog lights state
    bool low_beam;                     // Low beam state
    bool position_lamps;               // Position lights state
    bool apc_distributed;              // APC distribution state: 1 = APC lines distributed (ignition on)
    static std::optional<UpcGeneralStatus> decode(Data data);
};

// 0x645 CLUSTER_GeneralStatus_2 (100 ms), sent by the original instrument cluster.
struct ClusterGeneralStatus2 {
    // Displayed speed, 0.01 km/h per unit: the speed the original cluster shows. In the last two bytes, which this
    // car does not send (the frame has 3 of the database's 5 bytes), hence optional.
    std::optional<int> displayed_speed_centi_kmh;
    int night_dimmer_permille; // Night dimmer maximum lighting percentage, 0.4 % per unit, in tenths of a percent
    bool position_lamps;       // PositionLightsStatus (complementary database): 1 = on, as the cluster sees them
    SeatBeltLamp seat_belt_lamp;
    static std::optional<ClusterGeneralStatus2> decode(Data data);
};

// 0x651 AIRBAG_GeneralStatus (100 ms): the belt switches, read by the airbag ECU. The original cluster lights its
// belt lamp (0x645) from them.
struct AirbagGeneralStatus {
    std::optional<bool> driver_unbuckled;    // Driver seat belt switch: 00 = fastened, 01 = not fastened
    std::optional<bool> passenger_on_seat;   // Passenger on seat: 00 = detected, 01 = not detected
    std::optional<bool> passenger_unbuckled; // Passenger seat belt switch: 00 = fastened, 01 = not fastened
    static std::optional<AirbagGeneralStatus> decode(Data data);
};

// 0x715 CLUSTER_BackUp (3000 ms), sent by the original instrument cluster.
struct ClusterBackup {
    // Displayed oil level: 0000 = alert, 0001 to 1000 = 1 to 8 bar graph segments, 1111 = unavailable (and
    // the undocumented 1001 to 1110). `oil_bars` is empty for both alert and unavailable.
    bool oil_alert;
    std::optional<int> oil_bars;
    std::optional<MilLamp> mil;              // MIL lamp state, as the cluster shows it; empty if too short
    std::optional<int> oil_service_range_km; // Oil service range, 250 km per unit. For testing in the car
    static std::optional<ClusterBackup> decode(Data data);
};

// The latest valid frame of each identifier; empty when it is missing or timed out.
struct Frames {
    std::optional<EcmTorqueControl> ecm_torque_control;
    std::optional<EcmToAt> ecm_to_at;
    std::optional<UpcFastFrame> upc_fast_frame;
    std::optional<BrakeGeneralData> brake_general_data;
    std::optional<UchSystemControl> uch_system_control;
    std::optional<ClimControlFrame> clim_control_frame;
    std::optional<EcmGeneralStatus> ecm_general_status;
    std::optional<ClusterGeneralStatus> cluster_general_status;
    std::optional<EpsGeneralStatus> eps_general_status;
    std::optional<UchGeneralStatus> uch_general_status;
    std::optional<UpcGeneralStatus> upc_general_status;
    std::optional<ClusterGeneralStatus2> cluster_general_status_2;
    std::optional<AirbagGeneralStatus> airbag_general_status;
    std::optional<ClusterBackup> cluster_backup;

    // Decodes a frame of `id` into its slot. False for an unknown identifier or a frame too short, which leaves the
    // previous frame of that identifier in place.
    bool store(uint16_t id, Data data);
    // Empties the slot of `id`.
    void clear(uint16_t id);
};

} // namespace x84
