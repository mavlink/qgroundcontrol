#include "UBX/UBXPlan.h"

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

}  // namespace

FixedPositionWire fixedPositionWire(const GPSEllipsoidPosition& position)
{
    const auto latitude = static_cast<int64_t>(position.latitudeDegrees * 1e9);
    const auto longitude = static_cast<int64_t>(position.longitudeDegrees * 1e9);
    const auto height = static_cast<int64_t>(static_cast<double>(position.altitudeMeters) * 1e4);
    return {static_cast<int32_t>(latitude / 100),  static_cast<int8_t>(latitude % 100),
            static_cast<int32_t>(longitude / 100), static_cast<int8_t>(longitude % 100),
            static_cast<int32_t>(height / 100),    static_cast<int8_t>(height % 100)};
}

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
    return {.policy = NakPolicy::Report, .items = {{Cfg::UART1_BAUDRATE, baudrate}}};
}

std::vector<ValsetBatch> portsAndNavigation(const Target& target)
{
    const auto& board = target.profile;
    return {
        // UBX on USB; neither USB nor RTCM on M10.
        {.included = board.usb,
         .items = {{Cfg::UART1INPROT_RTCM3X, 0},
                   {Cfg::USBINPROT_UBX, 1},
                   {Cfg::USBINPROT_RTCM3X, 0},
                   {Cfg::USBINPROT_NMEA, 0},
                   {Cfg::USBOUTPROT_UBX, 1},
                   {Cfg::USBOUTPROT_NMEA, 0},
                   {Cfg::UART1OUTPROT_RTCM3X, 1, board.rtcmOutput},
                   {Cfg::USBOUTPROT_RTCM3X, 1, board.rtcmOutput}}},
        // SPARTN input (PointPerfect) is off; receivers without SPARTN reject it.
        {.included = board.usb,
         .policy = NakPolicy::Tolerate,
         .items = {{Cfg::UART1INPROT_SPARTN, 0}, {Cfg::USBINPROT_SPARTN, 0}}},
        {.items = {{Cfg::NAVSPG_FIXMODE, 3},      // auto 2D/3D
                   {Cfg::NAVSPG_UTCSTANDARD, 3},  // USNO, derived from GPS time
                   {Cfg::NAVSPG_DYNMODEL, STATIONARY_DYNAMIC_MODEL},
                   {Cfg::RATE_MEAS, board.measurementPeriodMs},
                   {Cfg::RATE_NAV, 1},
                   {Cfg::RATE_TIMEREF, 0}}},  // UTC
        // Odometer off. The F20 platform (ZED-X20P HPG 2.10+) has no CFG-ODO.
        {.policy = NakPolicy::Tolerate,
         .items = {{Cfg::ODO_USE_ODO, 0},
                   {Cfg::ODO_USE_COG, 0, board.fullOdometer},
                   {Cfg::ODO_OUTLPVEL, 0, board.fullOdometer},
                   {Cfg::ODO_OUTLPCOG, 0, board.fullOdometer}}},
        // RTK fixed mode, on RTK receivers only.
        {.policy = NakPolicy::IgnoreReply, .items = {{Cfg::NAVHPG_DGNSSMODE, 3}}},
    };
}

ValsetBatch jammingDetection()
{
    return {.policy = NakPolicy::Report, .items = {{Cfg::SEC_JAMDET_SENSITIVITY_HI, 0}}};
}

ValsetBatch interferenceMonitor()
{
    return {.policy = NakPolicy::Warn,
            .warning = "Jamming monitor not supported by this receiver",
            .items = {{Cfg::ITFM_ENABLE, 1}}};
}

std::vector<ValsetBatch> messageOutput(const Target& target)
{
    const auto& board = target.profile;
    return {
        // The key is only in app note UBX-21038688, so it has a batch of its own.
        {.included = board.l5HealthOverride,
         .policy = NakPolicy::Warn,
         .warning = "GPS L5 health override not supported by this receiver",
         .items = {{Cfg::SIGNAL_HEALTH_L5, 1}}},
        {.items = {{Cfg::MSGOUT_UBX_NAV_PVT, 1},
                   {Cfg::MSGOUT_UBX_NAV_HPPOSLLH, 1, board.highPrecision},
                   {Cfg::MSGOUT_UBX_NAV_RELPOSNED, NO_OUTPUT, board.highPrecision},
                   {Cfg::MSGOUT_UBX_NAV_DOP, 1},
                   {Cfg::MSGOUT_UBX_NAV_SAT, target.satelliteInfo ? ROVER_SATELLITE_RATE : NO_OUTPUT},
                   {Cfg::MSGOUT_UBX_NAV_STATUS, 1},
                   {Cfg::MSGOUT_UBX_MON_RF, 1}}},
        // Older firmware has no NAV-EOE; epochs then end at their bounded deadline.
        {.policy = NakPolicy::Tolerate, .items = {{Cfg::MSGOUT_UBX_NAV_EOE, 1}}},
        // RXM-COR reports every correction protocol (RTCM3, SPARTN, HAS) and is the only form on X20.
        {.policy = NakPolicy::Tolerate, .items = {{Cfg::MSGOUT_UBX_RXM_COR, 1}}},
        // u-blox 9 firmware without RXM-COR reports RTCM3 input in RXM-RTCM.
        {.included = board.rxmRtcmFallback,
         .policy = NakPolicy::Tolerate,
         .fallback = true,
         .items = {{Cfg::MSGOUT_UBX_RXM_RTCM, 1}}},
        // SEC-SIG jammingState (F9 HPG 1.51, X20); MON-RF jammingState stays 0 on that firmware.
        {.policy = NakPolicy::Tolerate, .items = {{Cfg::MSGOUT_UBX_SEC_SIG, 1}}},
        // Unused output that u-center or other firmware can leave enabled in non-volatile layers.
        {.policy = NakPolicy::Warn,
         .warning = "Could not disable unused messages",
         .items = {{Cfg::MSGOUT_UBX_NAV_TIMEGPS, NO_OUTPUT},
                   {Cfg::MSGOUT_UBX_RXM_SFRBX, NO_OUTPUT},
                   {Cfg::MSGOUT_UBX_RXM_RAWX, NO_OUTPUT, board.usb}}},
        // Dual-antenna heading. The receiver offset is zeroed so GPS_YAW_OFFSET is the only one applied; a
        // position-only X20P rejects the batch.
        {.included = board.dualAntennaHeading,
         .policy = NakPolicy::Tolerate,
         .items = {{Cfg::MSGOUT_UBX_NAV_DAHEADING, 1}, {Cfg::NAVSPG_DAHEADING_OFFSET, 0}}},
        // Galileo HAS (HPG 2.10+) only works without host corrections, so host input is restored: a HOST=0 left by
        // u-center would discard RTCM.
        {.included = board.dualAntennaHeading,
         .policy = NakPolicy::Tolerate,
         .items = {{Cfg::NAVCOR_ENABLE_HOST, 1}, {Cfg::NAVCOR_ENABLE_GAL_HAS, 0}}},
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
                      {Cfg::TMODE_SVIN_ACC_LIMIT, surveyAccuracyWireUnits(survey.accuracyMeters)},
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
                      {Cfg::TMODE_FIXED_POS_ACC, fixedAccuracyWireUnits(fixed.accuracyMeters)}}};
}

ValsetBatch rtcmOutput(const Target& target, bool compactObservations)
{
    const auto& on = compactObservations ? MSM4 : MSM7;
    const auto& off = compactObservations ? MSM7 : MSM4;
    return {.policy = NakPolicy::Report,
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
                      {Cfg::MSGOUT_UBX_NAV_SAT, target.satelliteInfo ? BASE_SATELLITE_INFO_RATE : NO_OUTPUT}}};
}

ValsetBatch disableMessage(MsgOutKey key)
{
    return {.policy = NakPolicy::WriteOnly, .items = {{key, NO_OUTPUT}}};
}

std::array<CfgPrt, 2> legacyPorts(uint32_t baudrate)
{
    const auto port = [baudrate](uint16_t id) {
        return CfgPrt{.portID = static_cast<uint8_t>(id),
                      .mode = LEGACY_PORT_MODE_8N1,
                      .baudRate = baudrate,
                      .inProtoMask = LEGACY_PROTOCOL_UBX,
                      .outProtoMask = LEGACY_PROTOCOL_UBX | LEGACY_PROTOCOL_RTCM3};
    };
    return {port(LEGACY_PORT_UART1), port(LEGACY_PORT_USB)};
}

std::array<MessageRate, 4> legacyStatus(const Target& target)
{
    return {{{Msg::NAV_STATUS, 1},
             {Msg::NAV_DOP, 1},
             {Msg::NAV_SVINFO, target.satelliteInfo ? uint8_t{5} : NO_OUTPUT},
             {Msg::MON_HW, 1}}};
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

std::array<MessageRate, 2> legacyBaseStatus(const Target& target)
{
    return {
        {{Msg::NAV_SVIN, NO_OUTPUT}, {Msg::NAV_SVINFO, target.satelliteInfo ? BASE_SATELLITE_INFO_RATE : NO_OUTPUT}}};
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
            .svinAccLimit = surveyAccuracyWireUnits(survey.accuracyMeters)};
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
            .fixedPosAcc = fixedAccuracyWireUnits(fixed.accuracyMeters)};
}

}  // namespace UBX::Plan
