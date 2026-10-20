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
    CLUSTER_GENERAL_STATUS_2, AIRBAG_GENERAL_STATUS, CLUSTER_BACKUP,
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

// Etats régulateur et limiteur de vitesse: 00 = RV et LV inactifs (OFF, en veille ou suspendu), 01 = reprise pédale,
// 10 = régulation ou limitation active, 11 = réservé.
enum class CruiseState { inactive, pedal_override, active, reserved };
// Point Mort: 00 = non atteinte, 01 = atteinte, 10 = véhicule non équipé, 11 = invalide.
enum class NeutralSwitch { not_in_neutral, in_neutral, not_equipped, invalid };
// Voyant état ABS: 00 = opérationnel ou non équipé (éteint), 01 = en défaut, 10 = absence de programmation,
// 11 = en calibration (mode diagnostic).
enum class AbsLamp { off, fault, not_programmed, calibration };
// Voyant défaut dépollution (0x551) and Etat témoin MIL (0x715): 00 = éteint, 01 = allumé permanent,
// 10 = clignotant, 11 = autotest voyant.
enum class MilLamp { off, on, blinking, self_test };
// Affichage état RV/LV: 000 = régulateur et limiteur OFF, 001 = limiteur actif, 010 = (limiteur ON et inactif) ou
// suspendu, 011 = limiteur demandé et en défaut, 100 to 110 the same for the régulateur, 111 = non présentes.
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
// Diagnostic jauge carburant: 00 = connexion OK, 01 = circuit ouvert, 10 = court-circuit, 11 = réservé.
enum class FuelGauge { ok, open_circuit, short_circuit, reserved };
// Défaut DAE: 00 = normal, 01 = défaut mineur -> Service, 10 = défaut majeur -> stop, 11 = mode diagnostic.
enum class SteeringFault { none, service, stop, diagnostic };
// Etats Clignotants: 000 = aucun, 001 = gauche, 010 = droit, 011 = les deux; 111 = invalide, and the undocumented
// 100 to 110.
enum class TurnSignals { off, left, right, both, invalid };
// Défaillance verrouillage colonne ou UCH: 000 = pas de défaut, 001 = défaillance électronique UCH/STOP,
// 010 = verrouillage non effectué (après coupure moteur), 100 = déverrouillage non effectué (avant démarrage),
// 110 = défaillance électronique UCH/SERV; 011 and 111 = réservé, and the undocumented 101.
enum class ColumnLockFault { none, stop, not_locked, not_unlocked, service, reserved };
// Demande de défilement ADAC (trip computer button): 00 = pas d'appui, 01 = sens +, 10 = sens -, 11 = invalide.
enum class ScrollRequest { none, forward, backward, invalid };
// Etat voyant ceinture: 00 = éteint (ceinture mise), 01 = allumé, 10 = clignotant (V > 15 km/h et tempo < 10 s),
// 11 = réservé.
enum class SeatBeltLamp { off, on, blinking, reserved };

// 0x181 ECM_TorqueControl (10 ms).
struct EcmTorqueControl {
    int engine_speed_rpm;              // Régime moteur, 0.125 rpm per unit, truncated to whole rpm
    std::optional<bool> clutch_pressed; // Contact embrayage: 00 = pied non appuyé, 01 = appuyé
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
    uint16_t distance_decimeters;  // Distance parcourue: rolling counter, 0.1 m per unit
    uint16_t wheel_speed_centi_kmh; // Vitesse véhicule (moyenne roues avant), 0.01 km/h per unit
    static std::optional<BrakeGeneralData> decode(Data data);
};

// 0x35D UCH_SystemControl (100 ms). Always decodes: each signal is empty when the frame is too short for it.
struct UchSystemControl {
    // BrakePadWear (complementary database), raw: 0 = no fault, 1 = short circuit 12 V, 2 = pad to be changed. For testing.
    std::optional<int> brake_pad_wear;
    std::optional<bool> child_lock_fault; // Alerte défaillance sécurité enfant: 1 = alerte
    static UchSystemControl decode(Data data);
};

// 0x551 ECM_GeneralStatus (100 ms).
struct EcmGeneralStatus {
    int coolant_temp_c;       // Température eau moteur
    bool coolant_temp_alert;  // Alerte température eau
    MilLamp mil;              // Voyant défaut dépollution
    int set_speed_kmh;        // Vitesse de consigne (cruise control or speed limiter)
    bool set_speed_blink;     // Clignotement vitesse de consigne RV/LV: 1 = affichage clignotant
    CruiseDisplay cruise_display;
    // Consommation carburant: rolling counter of the fuel injected, 80 mm³ per unit, wrapping after 255 (every
    // 20.48 mL). Checked in the car: it only goes up, and wraps.
    uint8_t fuel_counter;
    bool injection_fault;        // Défaut injection gravité 1
    bool severe_injection_fault; // Défaut injection gravité 2 lié à l'arrêt moteur
    static std::optional<EcmGeneralStatus> decode(Data data);
};

// 0x5C5 CLUSTER_GeneralStatus (100 ms), sent by the original instrument cluster.
struct ClusterGeneralStatus {
    int fuel_liters; // Niveau de carburant
    FuelGauge fuel_gauge;
    bool fuel_low;            // Alerte mini carburant: 1 = mini atteint
    bool parking_brake;       // Contact frein de parking: 1 = actionné
    bool brake_circuit_fault; // Incident circuit frein
    bool airbag_lamp;         // Etat témoin Défaut AIRBAG: 1 = allumé fixe
    bool lamp_self_test;      // Autotest voyants piloté par le TdB: 1 = en cours
    std::optional<uint32_t> odometer_km; // Totalisation générale; empty if the frame is too short
    static std::optional<ClusterGeneralStatus> decode(Data data);
};

// 0x5E4 EPS_GeneralStatus (100 ms), electric power steering (DAE).
struct EpsGeneralStatus {
    SteeringFault steering_fault;
    static std::optional<EpsGeneralStatus> decode(Data data);
};

// Etat ouvrants: one bit per door, true = open (xxxx1 = porte Av.G., xxx1x = Av.D., xx1xx = Ar.G., x1xxx = Ar.D.,
// 1xxxx = coffre ou abattant ou lunette arrière ouvrante).
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
    bool rear_fog;            // Etat feu de brouillard arrière: 1 = ON
    bool wiper_fault;         // Défaut essuyage
    ColumnLockFault column_lock_fault;
    int outside_temp_c;           // Température Extérieure
    std::optional<bool> reverse; // Marche arrière engagée: 00 = non engagée, 01 = engagée
    ScrollRequest scroll_request;
    std::optional<bool> card_battery_low; // Pile carte à changer; empty if the frame is too short, as the next one
    // Affichage informations VSC, raw: the message on the original cluster's display, mostly about the keyless
    // card. The table is in view.cpp.
    std::optional<int> keyless_display;
    static std::optional<UchGeneralStatus> decode(Data data);
};

// 0x625 UPC_GeneralStatus (100 ms), engine compartment fuse box.
struct UpcGeneralStatus {
    int battery_millivolts;            // Tension batterie by UPC, 0.0625 V per unit
    bool oil_pressure_switch_closed;   // Manocontact pression huile: 0 = ouvert, 1 = fermé
    bool battery_lamp;                 // Témoin Batterie: 1 = allumé
    bool battery_low;                  // Alerte batterie faible: 1 = tension inférieure au seuil
    bool high_beam;                    // Etat feux de route: 1 = allumés
    bool front_fog;                    // Etat feux de brouillard avant
    bool low_beam;                     // Etat feux de croisement
    bool position_lamps;               // Etat feux de position
    bool apc_distributed;              // Etat distribution APC: 1 = lignes APC distribuées (ignition on)
    static std::optional<UpcGeneralStatus> decode(Data data);
};

// 0x645 CLUSTER_GeneralStatus_2 (100 ms), sent by the original instrument cluster.
struct ClusterGeneralStatus2 {
    // Vitesse Affichée, 0.01 km/h per unit: the speed the original cluster shows. In the last two bytes, which this
    // car does not send (the frame has 3 of the database's 5 bytes), hence optional.
    std::optional<int> displayed_speed_centi_kmh;
    int night_dimmer_permille; // Pourcentage éclairage max rhéostaté de nuit, 0.4 % per unit, in tenths of a percent
    bool position_lamps;       // PositionLightsStatus (complementary database): 1 = on, as the cluster sees them
    SeatBeltLamp seat_belt_lamp;
    static std::optional<ClusterGeneralStatus2> decode(Data data);
};

// 0x651 AIRBAG_GeneralStatus (100 ms): the belt switches, read by the airbag ECU. The original cluster lights its
// belt lamp (0x645) from them.
struct AirbagGeneralStatus {
    std::optional<bool> driver_unbuckled;    // Contact ceinture conducteur: 00 = bouclée, 01 = non bouclée
    std::optional<bool> passenger_on_seat;   // Présence passager sur siège: 00 = détecté, 01 = non détecté
    std::optional<bool> passenger_unbuckled; // Contact ceinture passager: 00 = bouclée, 01 = non bouclée
    static std::optional<AirbagGeneralStatus> decode(Data data);
};

// 0x715 CLUSTER_BackUp (3000 ms), sent by the original instrument cluster.
struct ClusterBackup {
    // Niveau d'huile affiché: 0000 = alerte, 0001 to 1000 = 1 to 8 pavés de barre graphe, 1111 = indisponible (and
    // the undocumented 1001 to 1110). `oil_bars` is empty for both alert and unavailable.
    bool oil_alert;
    std::optional<int> oil_bars;
    std::optional<MilLamp> mil;              // Etat témoin MIL, as the cluster shows it; empty if too short
    std::optional<int> oil_service_range_km; // Autonomie vidange, 250 km per unit. For testing in the car
    static std::optional<ClusterBackup> decode(Data data);
};

// The latest valid frame of each identifier; empty when it is missing or timed out.
struct Frames {
    std::optional<EcmTorqueControl> ecm_torque_control;
    std::optional<EcmToAt> ecm_to_at;
    std::optional<UpcFastFrame> upc_fast_frame;
    std::optional<BrakeGeneralData> brake_general_data;
    std::optional<UchSystemControl> uch_system_control;
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
