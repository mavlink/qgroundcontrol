#include "UBX/UBXPlan.h"

#include <algorithm>

#include "GPSProtocolMath.h"

namespace UBX::Plan {
namespace {

/// RTCM3 observations of GPS, GLONASS, Galileo and BeiDou, as CFG-MSGOUT keys and as legacy CFG-MSG messages.
struct Observations
{
    std::array<MsgOutKey, 4> keys;
    std::array<MessageId, 4> messages;
};

constexpr Observations MSM7{{Cfg::MSGOUT_RTCM_3X_TYPE1077, Cfg::MSGOUT_RTCM_3X_TYPE1087, Cfg::MSGOUT_RTCM_3X_TYPE1097,
                             Cfg::MSGOUT_RTCM_3X_TYPE1127},
                            {Msg::RTCM3_1077, Msg::RTCM3_1087, Msg::RTCM3_1097, Msg::RTCM3_1127}};
/// About a third less bandwidth than MSM7, without Doppler.
constexpr Observations MSM4{{Cfg::MSGOUT_RTCM_3X_TYPE1074, Cfg::MSGOUT_RTCM_3X_TYPE1084, Cfg::MSGOUT_RTCM_3X_TYPE1094,
                             Cfg::MSGOUT_RTCM_3X_TYPE1124},
                            {Msg::RTCM3_1074, Msg::RTCM3_1084, Msg::RTCM3_1094, Msg::RTCM3_1124}};

constexpr uint32_t LEGACY_PORT_MODE_8N1 = 0x08d0;
constexpr uint16_t LEGACY_PROTOCOL_UBX = 1 << 0;
constexpr uint16_t LEGACY_PROTOCOL_NMEA = 1 << 1;
constexpr uint16_t LEGACY_PROTOCOL_RTCM3 = 1 << 5;

/// An accuracy in 0.1 mm; gpsValidateBaseStationConfig() rejects one that does not fit.
uint32_t accuracyWireUnits(double accuracyMeters)
{
    return GPSProtocolMath::tenthMillimeters(accuracyMeters).value_or(0);
}

/// Fixed position: latitude and longitude in 1e-7 deg and height in cm, each with a high-precision remainder
/// (1e-9 deg, 0.1 mm) in [-99, 99].
struct FixedPositionWire
{
    int32_t latitude;
    int8_t latitudeHp;
    int32_t longitude;
    int8_t longitudeHp;
    int32_t height;
    int8_t heightHp;
};

FixedPositionWire fixedPositionWire(const GPSEllipsoidPosition& position)
{
    const auto latitude = static_cast<int64_t>(position.latitudeDegrees * 1e9);
    const auto longitude = static_cast<int64_t>(position.longitudeDegrees * 1e9);
    const auto height = static_cast<int64_t>(position.altitudeMeters * 1e4);
    return {static_cast<int32_t>(latitude / 100),  static_cast<int8_t>(latitude % 100),
            static_cast<int32_t>(longitude / 100), static_cast<int8_t>(longitude % 100),
            static_cast<int32_t>(height / 100),    static_cast<int8_t>(height % 100)};
}

}  // namespace

ValsetBatch uart1Protocols()
{
    return {.timeout = PORT_TIMEOUT,
            .items = {{Cfg::UART1_STOPBITS, 1},
                      {Cfg::UART1_DATABITS, 0},  // 8 bits
                      {Cfg::UART1_PARITY, 0},
                      {Cfg::UART1INPROT_UBX, 1},
                      {Cfg::UART1INPROT_NMEA, 0},
                      {Cfg::UART1OUTPROT_UBX, 1},
                      {Cfg::UART1OUTPROT_NMEA, 0}}};
}

ValsetBatch uart1Baudrate(uint32_t baudrate)
{
    return {.policy = NakPolicy::Tolerate, .items = {{Cfg::UART1_BAUDRATE, baudrate}}};
}

std::vector<ValsetBatch> portsAndNavigation(const ReceiverProfile& profile)
{
    return {
        // UBX on USB, and RTCM3 output on both ports. USB keeps NMEA output, which the base ignores, so NMEA consumers
        // work after the session; UART1 gets none, as it typically feeds a correction radio.
        {.items = {{Cfg::UART1INPROT_RTCM3X, 0},
                   {Cfg::USBINPROT_UBX, 1},
                   {Cfg::USBINPROT_RTCM3X, 0},
                   {Cfg::USBINPROT_NMEA, 0},
                   {Cfg::USBOUTPROT_UBX, 1},
                   {Cfg::USBOUTPROT_NMEA, 1},
                   {Cfg::UART1OUTPROT_RTCM3X, 1},
                   {Cfg::USBOUTPROT_RTCM3X, 1}}},
        // SPARTN input (PointPerfect) is off; receivers without SPARTN reject it.
        {.policy = NakPolicy::Tolerate, .items = {{Cfg::UART1INPROT_SPARTN, 0}, {Cfg::USBINPROT_SPARTN, 0}}},
        {.items = {{Cfg::NAVSPG_FIXMODE, 3},      // auto 2D/3D
                   {Cfg::NAVSPG_UTCSTANDARD, 3},  // USNO, derived from GPS time
                   {Cfg::NAVSPG_DYNMODEL, STATIONARY_DYNAMIC_MODEL},
                   {Cfg::RATE_MEAS, profile.measurementPeriodMs},
                   {Cfg::RATE_NAV, 1},
                   {Cfg::RATE_TIMEREF, 0}}},  // UTC
        // Odometer off. The F20 platform (ZED-X20P HPG 2.10+) has no CFG-ODO.
        {.policy = NakPolicy::Tolerate,
         .items = {{Cfg::ODO_USE_ODO, 0}, {Cfg::ODO_USE_COG, 0}, {Cfg::ODO_OUTLPVEL, 0}, {Cfg::ODO_OUTLPCOG, 0}}},
        // RTK fixed mode, on RTK receivers only.
        {.policy = NakPolicy::Tolerate, .items = {{Cfg::NAVHPG_DGNSSMODE, 3}}},
    };
}

ValsetBatch jammingDetection()
{
    return {.policy = NakPolicy::Tolerate, .items = {{Cfg::SEC_JAMDET_SENSITIVITY_HI, 0}}};
}

ValsetBatch interferenceMonitor()
{
    return {.policy = NakPolicy::Warn,
            .warning = "Jamming monitor not supported by this receiver",
            .items = {{Cfg::ITFM_ENABLE, 1}}};
}

std::vector<ValsetBatch> messageOutput(const ReceiverProfile& profile)
{
    return {
        // The key is only in app note UBX-21038688, so it has a batch of its own.
        {.included = profile.l5HealthOverride,
         .policy = NakPolicy::Warn,
         .warning = "GPS L5 health override not supported by this receiver",
         .items = {{Cfg::SIGNAL_HEALTH_L5, 1}}},
        {.items = {{Cfg::MSGOUT_UBX_NAV_PVT, 1},
                   {Cfg::MSGOUT_UBX_NAV_HPPOSLLH, 1},
                   {Cfg::MSGOUT_UBX_NAV_RELPOSNED, NO_OUTPUT},
                   {Cfg::MSGOUT_UBX_NAV_DOP, 1},
                   {Cfg::MSGOUT_UBX_NAV_SAT, SURVEY_SATELLITE_RATE},
                   {Cfg::MSGOUT_UBX_NAV_STATUS, 1},
                   {Cfg::MSGOUT_UBX_MON_RF, 1}}},
        // Older firmware has no NAV-EOE; epochs then end at their bounded deadline.
        {.policy = NakPolicy::Tolerate, .items = {{Cfg::MSGOUT_UBX_NAV_EOE, 1}}},
        // SEC-SIG jammingState (F9 HPG 1.51, X20); MON-RF jammingState stays 0 on that firmware.
        {.policy = NakPolicy::Tolerate, .items = {{Cfg::MSGOUT_UBX_SEC_SIG, 1}}},
        // Unused output that u-center or other firmware can leave enabled in non-volatile layers.
        {.policy = NakPolicy::Warn,
         .warning = "Could not disable unused messages",
         .items = {{Cfg::MSGOUT_UBX_NAV_TIMEGPS, NO_OUTPUT},
                   {Cfg::MSGOUT_UBX_RXM_SFRBX, NO_OUTPUT},
                   {Cfg::MSGOUT_UBX_RXM_RAWX, NO_OUTPUT}}},
    };
}

ValsetBatch disableRTCMOutput()
{
    return {.policy = NakPolicy::Tolerate,
            .items = {{Cfg::MSGOUT_RTCM_3X_TYPE1005, NO_OUTPUT},
                      {MSM7.keys[0], NO_OUTPUT},
                      {MSM7.keys[1], NO_OUTPUT},
                      {Cfg::MSGOUT_RTCM_3X_TYPE1230, NO_OUTPUT},
                      {MSM7.keys[2], NO_OUTPUT},
                      {MSM7.keys[3], NO_OUTPUT},
                      {MSM4.keys[0], NO_OUTPUT},
                      {MSM4.keys[1], NO_OUTPUT},
                      {MSM4.keys[2], NO_OUTPUT},
                      {MSM4.keys[3], NO_OUTPUT}}};
}

ValsetBatch disableTimeMode()
{
    return {.items = {{Cfg::TMODE_MODE, 0}}};
}

ValsetBatch surveyIn(const GPSBaseStationConfig::SurveyIn& survey)
{
    return {.items = {{Cfg::TMODE_MODE, 1},
                      {Cfg::TMODE_SVIN_MIN_DUR, static_cast<uint32_t>(survey.duration.count())},
                      {Cfg::TMODE_SVIN_ACC_LIMIT, accuracyWireUnits(survey.accuracyMeters)},
                      {Cfg::MSGOUT_UBX_NAV_SVIN, SURVEY_STATUS_RATE}}};
}

ValsetBatch fixedBase(const GPSBaseStationConfig::Fixed& fixed)
{
    const auto position = fixedPositionWire(fixed.position);
    return {.items = {{Cfg::TMODE_MODE, 2},
                      {Cfg::TMODE_POS_TYPE, 1},  // latitude, longitude and height
                      {Cfg::TMODE_LAT, position.latitude},
                      {Cfg::TMODE_LAT_HP, position.latitudeHp},
                      {Cfg::TMODE_LON, position.longitude},
                      {Cfg::TMODE_LON_HP, position.longitudeHp},
                      {Cfg::TMODE_HEIGHT, position.height},
                      {Cfg::TMODE_HEIGHT_HP, position.heightHp},
                      {Cfg::TMODE_FIXED_POS_ACC, accuracyWireUnits(fixed.accuracyMeters)}}};
}

ValsetBatch rtcmOutput(bool compactObservations)
{
    const auto& on = compactObservations ? MSM4 : MSM7;
    const auto& off = compactObservations ? MSM7 : MSM4;
    return {.policy = NakPolicy::Tolerate,
            .items = {{Cfg::RATE_MEAS, BASE_MEASUREMENT_PERIOD_MS},
                      {Cfg::MSGOUT_RTCM_3X_TYPE1005, 1},  // stationary reference station position
                      {on.keys[0], 1},
                      {on.keys[1], 1},
                      {Cfg::MSGOUT_RTCM_3X_TYPE1230, 1},  // GLONASS code-phase biases
                      {on.keys[2], 1},
                      {on.keys[3], 1},
                      {off.keys[0], NO_OUTPUT},
                      {off.keys[1], NO_OUTPUT},
                      {off.keys[2], NO_OUTPUT},
                      {off.keys[3], NO_OUTPUT},
                      {Cfg::MSGOUT_UBX_NAV_SVIN, NO_OUTPUT},
                      {Cfg::MSGOUT_UBX_NAV_SAT, BASE_SATELLITE_INFO_RATE}}};
}

ValsetBatch disableMessage(MsgOutKey key)
{
    return {.policy = NakPolicy::WriteOnly, .items = {{key, NO_OUTPUT}}};
}

std::array<uint8_t, 2 * WIRE_SIZE<CfgPrt>> legacyPorts(uint32_t baudrate)
{
    std::array<uint8_t, 2 * WIRE_SIZE<CfgPrt>> payload{};
    auto next = payload.begin();
    for (const uint16_t id : {LEGACY_PORT_UART1, LEGACY_PORT_USB}) {
        const uint16_t nmea = id == LEGACY_PORT_USB ? LEGACY_PROTOCOL_NMEA : 0;
        const auto port = Wire::encode(
            CfgPrt{.portID = static_cast<uint8_t>(id),
                   .mode = LEGACY_PORT_MODE_8N1,
                   .baudRate = baudrate,
                   .inProtoMask = LEGACY_PROTOCOL_UBX,
                   .outProtoMask = static_cast<uint16_t>(LEGACY_PROTOCOL_UBX | nmea | LEGACY_PROTOCOL_RTCM3)});
        next = std::copy(port.begin(), port.end(), next);
    }
    return payload;
}

std::array<MessageRate, 5> legacyMessageOutput()
{
    return {{{Msg::NAV_PVT, 1}, {Msg::NAV_STATUS, 1}, {Msg::NAV_DOP, 1}, {Msg::NAV_SVINFO, 5}, {Msg::MON_HW, 1}}};
}

std::array<MessageRate, 10> legacyDisableRTCMOutput()
{
    return {{{Msg::RTCM3_1005, NO_OUTPUT},
             {Msg::RTCM3_1230, NO_OUTPUT},
             {MSM7.messages[0], NO_OUTPUT},
             {MSM7.messages[1], NO_OUTPUT},
             {MSM7.messages[2], NO_OUTPUT},
             {MSM7.messages[3], NO_OUTPUT},
             {MSM4.messages[0], NO_OUTPUT},
             {MSM4.messages[1], NO_OUTPUT},
             {MSM4.messages[2], NO_OUTPUT},
             {MSM4.messages[3], NO_OUTPUT}}};
}

std::array<MessageRate, 2> legacyBaseStatus()
{
    return {{{Msg::NAV_SVIN, NO_OUTPUT}, {Msg::NAV_SVINFO, BASE_SATELLITE_INFO_RATE}}};
}

std::array<RTCMOutput, 6> legacyRTCMOutput(bool compactObservations)
{
    const auto& observations = compactObservations ? MSM4 : MSM7;
    return {{{{Msg::RTCM3_1005, 5}, RTCMContent::StationPosition},  // the station position can be sent at a lower rate
             {{observations.messages[0], 1}, RTCMContent::Observations},
             {{observations.messages[1], 1}, RTCMContent::Observations},
             {{Msg::RTCM3_1230, 1}, RTCMContent::Optional},
             {{observations.messages[2], 1}, RTCMContent::Observations},
             {{observations.messages[3], 1}, RTCMContent::Observations}}};
}

CfgTmode3 legacySurveyIn(const GPSBaseStationConfig::SurveyIn& survey)
{
    return {.flags = 1,
            .svinMinDur = static_cast<uint32_t>(survey.duration.count()),
            .svinAccLimit = accuracyWireUnits(survey.accuracyMeters)};
}

CfgTmode3 legacyFixedBase(const GPSBaseStationConfig::Fixed& fixed)
{
    const auto position = fixedPositionWire(fixed.position);
    return {.flags = 2 | (1 << 8),  // fixed mode, latitude/longitude/height
            .ecefXOrLat = position.latitude,
            .ecefYOrLon = position.longitude,
            .ecefZOrAlt = position.height,
            .ecefXOrLatHP = position.latitudeHp,
            .ecefYOrLonHP = position.longitudeHp,
            .ecefZOrAltHP = position.heightHp,
            .fixedPosAcc = accuracyWireUnits(fixed.accuracyMeters)};
}

}  // namespace UBX::Plan
