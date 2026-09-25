#include "UBX/UBXProtocol.h"
#include "UBXConfiguration_p.h"
#include "UBXMessageCodec.h"

// Receivers before protocol 27 (u-blox M8 and older) configure through CFG-MSG/CFG-RATE/CFG-NAV5/CFG-TMODE3
// instead of CFG-VALSET.

namespace {
constexpr uint16_t RTCM_MSM7_OBSERVATION_MESSAGES[] = {UBX_MSG_RTCM3_1077, UBX_MSG_RTCM3_1087, UBX_MSG_RTCM3_1097,
                                                       UBX_MSG_RTCM3_1127};
constexpr uint16_t RTCM_MSM4_OBSERVATION_MESSAGES[] = {UBX_MSG_RTCM3_1074, UBX_MSG_RTCM3_1084, UBX_MSG_RTCM3_1094,
                                                       UBX_MSG_RTCM3_1124};
}  // namespace

bool UBXProtocol::configureDevicePreV27()
{
    ubx_payload_tx_cfg_nav5_t payload_tx_cfg_nav5{};
    ubx_payload_tx_cfg_rate_t payload_tx_cfg_rate{};

    /* Send a CFG-RATE message to define update rate */
    payload_tx_cfg_rate = {};
    payload_tx_cfg_rate.measRate = UBX_TX_CFG_RATE_MEASINTERVAL;
    payload_tx_cfg_rate.navRate = UBX_TX_CFG_RATE_NAVRATE;
    payload_tx_cfg_rate.timeRef = UBX_TX_CFG_RATE_TIMEREF;

    if (!sendMessage(UBX_MSG_CFG_RATE, UBX::encode(payload_tx_cfg_rate))) {
        return false;
    }

    if (!waitForAck(UBX_MSG_CFG_RATE).succeeded()) {
        return false;
    }

    /* send a NAV5 message to set the options for the internal filter */
    payload_tx_cfg_nav5 = {};
    payload_tx_cfg_nav5.mask = UBX_TX_CFG_NAV5_MASK;
    payload_tx_cfg_nav5.dynModel = UBX::STATIONARY_DYNAMIC_MODEL;
    payload_tx_cfg_nav5.fixMode = UBX_TX_CFG_NAV5_FIXMODE;

    if (!sendMessage(UBX_MSG_CFG_NAV5, UBX::encode(payload_tx_cfg_nav5))) {
        return false;
    }

    if (!waitForAck(UBX_MSG_CFG_NAV5).succeeded()) {
        return false;
    }

    /* configure message rates */
    /* the last argument is divisor for measurement rate (set by CFG RATE), i.e. 1 means 5Hz */

    /* try to set rate for NAV-PVT */
    /* (implemented for ubx7+ modules only, use NAV-SOL, NAV-POSLLH, NAV-VELNED and NAV-TIMEUTC for ubx6) */
    if (!configureMessageRate(UBX_MSG_NAV_PVT, 1)) {
        return false;
    }

    if (!waitForAck(UBX_MSG_CFG_MSG).succeeded()) {
        _decodeContext.useNavPvt = false;

    } else {
        _decodeContext.useNavPvt = true;
    }

    if (!_decodeContext.useNavPvt) {
        if (!configureMessageRateAndAck(UBX_MSG_NAV_TIMEUTC, 5, true)) {
            return false;
        }

        if (!configureMessageRateAndAck(UBX_MSG_NAV_POSLLH, 1, true)) {
            return false;
        }

        if (!configureMessageRateAndAck(UBX_MSG_NAV_SOL, 1, true)) {
            return false;
        }

        if (!configureMessageRateAndAck(UBX_MSG_NAV_VELNED, 1, true)) {
            return false;
        }
    }

    if (!configureMessageRateAndAck(UBX_MSG_NAV_STATUS, 1, true)) {
        return false;
    }

    if (!configureMessageRateAndAck(UBX_MSG_NAV_DOP, 1, true)) {
        return false;
    }

    if (!configureMessageRateAndAck(UBX_MSG_NAV_SVINFO, (_satellites != nullptr) ? 5 : 0, true)) {
        return false;
    }

    if (!configureMessageRateAndAck(UBX_MSG_MON_HW, 1, true)) {
        return false;
    }

    return true;
}

bool UBXProtocol::restartSurveyInPreV27()
{
    ubx_payload_tx_cfg_tmode3_t payload_tx_cfg_tmode3{};

    // Disable RTCM output, including observations from a previous session in the other MSM format.
    configureMessageRate(UBX_MSG_RTCM3_1005, 0);
    configureMessageRate(UBX_MSG_RTCM3_1230, 0);
    for (const uint16_t message : RTCM_MSM7_OBSERVATION_MESSAGES) {
        configureMessageRate(message, 0);
    }
    for (const uint16_t message : RTCM_MSM4_OBSERVATION_MESSAGES) {
        configureMessageRate(message, 0);
    }

    if (!disableTimeMode() ||
        (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) && !waitForSurveyStop())) {
        return false;
    }

    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        payload_tx_cfg_tmode3 = {};
        payload_tx_cfg_tmode3.flags = 1; /* start survey-in */
        payload_tx_cfg_tmode3.svinMinDur = std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).durationSecs;
        payload_tx_cfg_tmode3.svinAccLimit =
            UBX::surveyAccuracyWireUnits(std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).accuracyMeters);

        if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(payload_tx_cfg_tmode3))) {
            return false;
        }

        if (!waitForAck(UBX_MSG_CFG_TMODE3).succeeded()) {
            return false;
        }

        /* enable status output of survey-in */
        if (!configureMessageRateAndAck(UBX_MSG_NAV_SVIN, 5, true)) {
            return false;
        }

    } else {
        const auto& settings = std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode);

        payload_tx_cfg_tmode3 = {};
        payload_tx_cfg_tmode3.flags = 2 /* fixed mode */ | (1 << 8) /* lat/lon mode */;
        const auto position = UBX::fixedPositionWire(settings.position);
        payload_tx_cfg_tmode3.ecefXOrLat = position.latitude;
        payload_tx_cfg_tmode3.ecefXOrLatHP = position.latitudeHp;
        payload_tx_cfg_tmode3.ecefYOrLon = position.longitude;
        payload_tx_cfg_tmode3.ecefYOrLonHP = position.longitudeHp;
        payload_tx_cfg_tmode3.ecefZOrAlt = position.height;
        payload_tx_cfg_tmode3.ecefZOrAltHP = position.heightHp;
        payload_tx_cfg_tmode3.fixedPosAcc = UBX::fixedAccuracyWireUnits(settings.accuracyMeters);

        if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(payload_tx_cfg_tmode3))) {
            return false;
        }

        if (!waitForAck(UBX_MSG_CFG_TMODE3).succeeded()) {
            return false;
        }

        // directly enable RTCM3 output
        return activateRTCMOutput();
    }

    return true;
}

bool UBXProtocol::activateRTCMOutputPreV27()
{
    ubx_payload_tx_cfg_rate_t payload_tx_cfg_rate{};
    payload_tx_cfg_rate.measRate = 1000;
    payload_tx_cfg_rate.navRate = UBX_TX_CFG_RATE_NAVRATE;
    payload_tx_cfg_rate.timeRef = UBX_TX_CFG_RATE_TIMEREF;

    if (!sendMessage(UBX_MSG_CFG_RATE, UBX::encode(payload_tx_cfg_rate))) {
        return false;
    }

    configureMessageRate(UBX_MSG_NAV_SVIN, 0);
    // The rover-rate satellite divisors would space satellite reports beyond their 5 s freshness at 1 Hz.
    configureMessageRate(UBX_MSG_NAV_SVINFO, _satellites != nullptr ? UBX::BASE_SATELLITE_INFO_RATE : 0);

    // stationary RTK reference station ARP (can be sent at lower rate)
    if (!configureMessageRate(UBX_MSG_RTCM3_1005, 5)) {
        return false;
    }

    const auto& observations =
        _baseConfig.compactObservations ? RTCM_MSM4_OBSERVATION_MESSAGES : RTCM_MSM7_OBSERVATION_MESSAGES;
    // GPS and GLONASS observations, then GLONASS code-phase biases, Galileo and BeiDou observations.
    return configureMessageRate(observations[0], 1) && configureMessageRate(observations[1], 1) &&
           configureMessageRate(UBX_MSG_RTCM3_1230, 1) && configureMessageRate(observations[2], 1) &&
           configureMessageRate(observations[3], 1);
}

bool UBXProtocol::configureMessageRate(const uint16_t msg, const uint8_t rate, bool required)
{
    if (_identity.protocol27) {
        // configureMessageRate() should not be called if _identity.protocol27 is true.
        // If you see this message the calling code needs to be fixed.
        log(GPSProtocolLogLevel::Warning, "FIXME: use of deprecated msg CFG_MSG (%i %i)", msg, rate);
    }

    ubx_payload_tx_cfg_msg_t cfg_msg{};

    cfg_msg.msg = msg;
    cfg_msg.rate = rate;

    return sendMessage(UBX_MSG_CFG_MSG, UBX::encode(cfg_msg),
                       {{}, std::chrono::milliseconds(UBX_CONFIG_TIMEOUT), {}, required});
}

bool UBXProtocol::configureMessageRateAndAck(uint16_t msg, uint8_t rate, bool report_ack_error)
{
    if (!configureMessageRate(msg, rate, report_ack_error)) {
        return false;
    }

    return waitForAck(UBX_MSG_CFG_MSG).succeeded();
}
