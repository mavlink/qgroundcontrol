/****************************************************************************
 *
 *   Copyright (c) 2012-2023 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include <cmath>
#include <string.h>

#include <QtCore/QScopeGuard>

#include "LittleEndian.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"
#include "UBX/GPSDriverUBX.h"
#include "UBXMessageCodec.h"
#include "UBXMessageSchema.h"

namespace {
// Base stations always use the stationary navigation model.
constexpr uint8_t STATIONARY_DYNAMIC_MODEL = 2;

// RTCM3 message sets for a base: the station/bias messages plus GPS, GLONASS, Galileo and BeiDou
// observations as MSM4 or MSM7 (1074/1084/1094/1124 vs 1077/1087/1097/1127)
static constexpr uint32_t RTCM_BASE_MSGOUT_I2C[] = {
    UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1005_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1077_I2C,
    UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1087_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1230_I2C,
    UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1097_I2C, UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1127_I2C};

uint32_t fixedAccuracyWireUnits(float accuracyMeters)
{
    // Match shared validation: multiplying directly by 10000 can round an accepted value past UINT32_MAX.
    const float accuracyMillimeters = accuracyMeters * 1000.0f;
    return static_cast<uint32_t>(accuracyMillimeters * 10.0f);
}
}  // namespace

GPSNativeUBX::BaseStationCapability GPSNativeUBX::baseStationCapability() const
{
    if (_identity.board == Board::u_blox8) {
        return _identity.isM8p
                   ? BaseStationCapability::Supported
                   : (_identity.modelName[0] ? BaseStationCapability::Unsupported : BaseStationCapability::Unknown);
    }
    const auto profile = UBX::receiverProfile(_identity.board);
    return profile.rtcmOutput            ? BaseStationCapability::Supported
           : profile.baseCapabilityKnown ? BaseStationCapability::Unsupported
                                         : BaseStationCapability::Unknown;
}

int GPSNativeUBX::configure(unsigned& baudrate, const GPSConfig& config)
{
    _baseConfig = config.base;
    resetIOError();
    _identity.timeModeUnsupported = false;
    _valsetAckAmbiguous = false;
    _configured = false;
    _decodeContext = {};
    _navigationEpochs = {};
    _controller = {};
    _pendingDisableMessage = 0;
    _comms.pending = false;
    _rtcmActivationPending = false;
    _rtcm_parsing.reset();
    if (!validateConfiguration(config)) {
        return -1;
    }

    const bool auto_baudrate = baudrate == 0;
    const auto identify = [this] {
        // Slow factory NMEA output can delay a MON-VER response by over a second.
        const Operation operation(*this, 2000);
        _identity.board = Board::unknown;
        return sendMessage(UBX_MSG_MON_VER, nullptr, 0, {{}, std::chrono::milliseconds(2000)}) &&
               waitForAck(UBX_MSG_MON_VER).succeeded();
    };
    constexpr unsigned BAUD_RATES[] = {38400, 57600, 9600, 115200, 230400, 460800, 921600};
    unsigned detectedBaud = 0;
    for (const unsigned candidate : BAUD_RATES) {
        const unsigned selected = auto_baudrate ? candidate : baudrate;
        if (setBaudrate(selected) < 0) {
            return -1;
        }
        decodeInit();
        bool readError = false;
        receiveInternal(20, readError);
        decodeInit();
        if (readError) {
            return -1;
        }
        if (identify()) {
            detectedBaud = selected;
            break;
        }
        if (ioError() || !auto_baudrate) {
            return -1;
        }
    }
    // Discovery only polls identity: silence or an unsupported identity must not change receiver settings.
    if (!detectedBaud || _identity.board == Board::unknown) {
        return -1;
    }
    if (baseStationCapability() == BaseStationCapability::Unsupported) {
        return -1;
    }

    const unsigned desiredBaud = !auto_baudrate                                              ? baudrate
                                 : _identity.protocol27 || _identity.board == Board::u_blox8 ? UBX_BAUDRATE_M8_AND_NEWER
                                                                                             : UBX_TX_CFG_PRT_BAUDRATE;
    ubx_payload_tx_cfg_prt_t ports[2]{};
    if (_identity.protocol27) {
        static constexpr CfgValsetItem UART1_UBX[] = {
            {UBX_CFG_KEY_CFG_UART1_STOPBITS, 1},    {UBX_CFG_KEY_CFG_UART1_DATABITS, 0},
            {UBX_CFG_KEY_CFG_UART1_PARITY, 0},      {UBX_CFG_KEY_CFG_UART1INPROT_UBX, 1},
            {UBX_CFG_KEY_CFG_UART1INPROT_NMEA, 0},  {UBX_CFG_KEY_CFG_UART1OUTPROT_UBX, 1},
            {UBX_CFG_KEY_CFG_UART1OUTPROT_NMEA, 0},
        };
        initCfgValset();
        cfgValset(UART1_UBX);
        if (!sendCfgValset(true, 2000) || !waitForAck(UBX_MSG_CFG_VALSET).succeeded()) {
            return -1;
        }
    } else {
        for (auto& port : ports) {
            port.mode = UBX_TX_CFG_PRT_MODE;
            port.baudRate = detectedBaud;
            port.inProtoMask = UBX_TX_CFG_PRT_PROTO_UBX;
            port.outProtoMask = UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_RTCM;
        }
        ports[0].portID = UBX_TX_CFG_PRT_PORTID;
        ports[1].portID = UBX_TX_CFG_PRT_PORTID_USB;
        if (!sendMessage(UBX_MSG_CFG_PRT, UBX::encode(ports)) || !waitForAck(UBX_MSG_CFG_PRT).succeeded()) {
            return -1;
        }
    }
    if (desiredBaud != detectedBaud) {
        const bool modern = _identity.protocol27;
        const Board identifiedBoard = _identity.board;
        if (modern) {
            initCfgValset();
            cfgValset<uint32_t>(UBX_CFG_KEY_CFG_UART1_BAUDRATE, desiredBaud);
            if (!sendCfgValset(false)) {
                return -1;
            }
        } else {
            for (auto& port : ports) {
                port.baudRate = desiredBaud;
            }
            if (!sendMessage(UBX_MSG_CFG_PRT, UBX::encode(ports),
                             {{}, std::chrono::milliseconds(UBX_CONFIG_TIMEOUT), {}, false})) {
                return -1;
            }
        }
        const uint16_t command = modern ? UBX_MSG_CFG_VALSET : UBX_MSG_CFG_PRT;
        const auto result = waitForAck(command);
        const bool acknowledged = result.succeeded();
        if (ioError() || result.evidence.outcome == GPSCommandOutcome::Rejected || setBaudrate(desiredBaud) < 0) {
            return -1;
        }
        if (modern && !acknowledged) {
            // Do not clear ACK ambiguity after UART handoff: every later VALSET needs matching readback.
            _controller.requireConfigurationReadback(command);
        }
        decodeInit();
        if (!identify() || _identity.board != identifiedBoard || _identity.protocol27 != modern ||
            _controller.lateRejection()) {
            return -1;
        }
        if (modern && !acknowledged &&
            !verifyCfgValset(
                 {"UBX-CFG-VALSET readback", std::chrono::milliseconds(UBX_CONFIG_TIMEOUT), _valset.settings})
                 .succeeded()) {
            return -1;
        }
    }
    baudrate = desiredBaud;

    if (!_rtcm_parsing) {
        _rtcm_parsing.emplace();
    }
    _rtcm_parsing->reset();

    int ret;

    /* Configure the device, use config commands depending on protocol version */
    if (_identity.protocol27) {
        ret = configureDevice();

    } else {
        ret = configureDevicePreV27();
    }

    if (ret != 0) {
        return ret;
    }

    if (restartSurveyIn() < 0) {
        return -1;
    }

    _configured = true;
    _decodeContext.navigation = true;
    _decodeContext.assembleEpochs = true;
    return 0;
}

int GPSNativeUBX::configureDevicePreV27()
{
    ubx_payload_tx_cfg_nav5_t payload_tx_cfg_nav5{};
    ubx_payload_tx_cfg_rate_t payload_tx_cfg_rate{};

    /* Send a CFG-RATE message to define update rate */
    payload_tx_cfg_rate = {};
    payload_tx_cfg_rate.measRate = UBX_TX_CFG_RATE_MEASINTERVAL;
    payload_tx_cfg_rate.navRate = UBX_TX_CFG_RATE_NAVRATE;
    payload_tx_cfg_rate.timeRef = UBX_TX_CFG_RATE_TIMEREF;

    if (!sendMessage(UBX_MSG_CFG_RATE, UBX::encode(payload_tx_cfg_rate))) {
        return -1;
    }

    if (!waitForAck(UBX_MSG_CFG_RATE).succeeded()) {
        return -1;
    }

    /* send a NAV5 message to set the options for the internal filter */
    payload_tx_cfg_nav5 = {};
    payload_tx_cfg_nav5.mask = UBX_TX_CFG_NAV5_MASK;
    payload_tx_cfg_nav5.dynModel = STATIONARY_DYNAMIC_MODEL;
    payload_tx_cfg_nav5.fixMode = UBX_TX_CFG_NAV5_FIXMODE;

    if (!sendMessage(UBX_MSG_CFG_NAV5, UBX::encode(payload_tx_cfg_nav5))) {
        return -1;
    }

    if (!waitForAck(UBX_MSG_CFG_NAV5).succeeded()) {
        return -1;
    }

    /* configure message rates */
    /* the last argument is divisor for measurement rate (set by CFG RATE), i.e. 1 means 5Hz */

    /* try to set rate for NAV-PVT */
    /* (implemented for ubx7+ modules only, use NAV-SOL, NAV-POSLLH, NAV-VELNED and NAV-TIMEUTC for ubx6) */
    if (!configureMessageRate(UBX_MSG_NAV_PVT, 1)) {
        return -1;
    }

    if (!waitForAck(UBX_MSG_CFG_MSG).succeeded()) {
        _decodeContext.useNavPvt = false;

    } else {
        _decodeContext.useNavPvt = true;
    }

    if (!_decodeContext.useNavPvt) {
        if (!configureMessageRateAndAck(UBX_MSG_NAV_TIMEUTC, 5, true)) {
            return -1;
        }

        if (!configureMessageRateAndAck(UBX_MSG_NAV_POSLLH, 1, true)) {
            return -1;
        }

        if (!configureMessageRateAndAck(UBX_MSG_NAV_SOL, 1, true)) {
            return -1;
        }

        if (!configureMessageRateAndAck(UBX_MSG_NAV_VELNED, 1, true)) {
            return -1;
        }
    }

    if (!configureMessageRateAndAck(UBX_MSG_NAV_STATUS, 1, true)) {
        return -1;
    }

    if (!configureMessageRateAndAck(UBX_MSG_NAV_DOP, 1, true)) {
        return -1;
    }

    if (!configureMessageRateAndAck(UBX_MSG_NAV_SVINFO, (_satellites != nullptr) ? 5 : 0, true)) {
        return -1;
    }

    if (!configureMessageRateAndAck(UBX_MSG_MON_HW, 1, true)) {
        return -1;
    }

    return 0;
}

int GPSNativeUBX::configureDevice()
{
    // There is no RTCM or USB interface on M10
    if (UBX::receiverProfile(_identity.board).usb) {
        initCfgValset();

        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1INPROT_RTCM3X, 0);

        // USB
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_UBX, 1);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_RTCM3X, 0);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_NMEA, 0);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_UBX, 1);

        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA, 0);

        // Only RTCM-output-capable receivers expose these keys. M9 SPG rejects
        // the entire VALSET if they are included, even with a value of zero.
        if (UBX::receiverProfile(_identity.board).rtcmOutput) {
            cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X, 1);
            cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X, 1);
        }

        if (!sendCfgValsetAcked().succeeded()) {
            return -1;
        }

        // Optional SPARTN input (PointPerfect). Sent separately so modules without
        // SPARTN support can NACK without failing the rest of configuration.
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1INPROT_SPARTN, 0);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_SPARTN, 0);

        sendCfgValsetAcked(false);
    }

    /* set configuration parameters */
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_FIXMODE, 3 /* Auto 2d/3d */);
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_UTCSTANDARD, 3 /* USNO (U.S. Naval Observatory derived from GPS) */);
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_DYNMODEL, STATIONARY_DYNAMIC_MODEL);

    // measurement rate
    // M9N max rate is 8Hz for all satellites, above 8Hz the number of used satellites is restricted to 16.
    // F9P L1L2 in firmware <1.50 the max update rate with 4 constellations is 9Hz without RTK and 7Hz with RTK
    // F9P L1L2 in firmware >=1.50 the max update rate with 4 constellations is 7Hz without RTK and 5Hz with RTK
    // F9P L1L5 the max update rate with 4 constellations is 8Hz without RTK and 7Hz with RTK
    // DAN-F10N the max update rate is 10Hz with GPS+GAL+BDS(Default)
    // X20 max update rate is 25Hz, but 25Hz at 115200 baud causes high dropouts, especially with RTK. So default 10Hz
    // is selected. Receivers such as M9N and DAN-F10N can go higher than 10Hz, but the number of used satellites will
    // be restricted to 16. (Not mentioned in datasheet)
    int rate_meas = 100;  // 10Hz

    switch (_identity.board) {
        case Board::u_blox9:
            rate_meas = 125;  // 8Hz
            break;

        case Board::u_blox9_F9P_L1L2:
        case Board::u_blox9_F9P_L1L5:
            rate_meas = 200;  // 5Hz
            break;

        default:
            break;
    }

    cfgValset<uint16_t>(UBX_CFG_KEY_RATE_MEAS, rate_meas);
    cfgValset<uint16_t>(UBX_CFG_KEY_RATE_NAV, 1);
    cfgValset<uint8_t>(UBX_CFG_KEY_RATE_TIMEREF, 0);

    if (!sendCfgValsetAcked().succeeded()) {
        return -1;
    }

    // Disable odometer. Separate, non-fatal VALSET: CFG-ODO-* was removed on the
    // F20 platform (ZED-X20P HPG 2.10+), so a NAK here must not abort config.
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_ODO_USE_ODO, 0);

    // M9 (SPG) only has USE_ODO and PROFILE in CFG-ODO
    if (_identity.board != Board::u_blox9) {
        static constexpr uint32_t odo_keys[] = {UBX_CFG_KEY_ODO_USE_COG, UBX_CFG_KEY_ODO_OUTLPVEL,
                                                UBX_CFG_KEY_ODO_OUTLPCOG};
        cfgValset(odo_keys, 0);
    }

    sendCfgValsetAcked(false);

    // RTK (optional, as only RTK devices like F9P support it)
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVHPG_DGNSSMODE, 3 /* RTK Fixed */);

    if (!sendCfgValset(false)) {
        return -1;
    }

    waitForAck(UBX_MSG_CFG_VALSET);

    // Jamming detection. Firmware with CFG-SEC-JAMDET (F9 HPG 1.50+, F9 L1L5, F20/X20) has
    // detection always on and no CFG-ITFM; everything older has CFG-ITFM and no JAMDET key.
    // A NAK on the sensitivity key is therefore the signal that the monitor still needs enabling.
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_SEC_JAMDET_SENSITIVITY_HI, 0);

    if (!sendCfgValsetAcked(false).succeeded()) {
        if (_valsetAckAmbiguous) {
            if (!ioError()) {
                log(GPSProtocolLogLevel::Warning, "CFG-SEC-JAMDET_SENSITIVITY_HI not supported");
            }
            return -1;
        }
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_ITFM_ENABLE, 1);

        if (!sendCfgValsetAcked(false).succeeded() && !ioError()) {
            log(GPSProtocolLogLevel::Warning, "Jamming monitor not supported by this receiver");
        }
    }

    // GPS L5 is broadcast unhealthy while it is pre-operational, so tell the receiver to take
    // the L1 health flag instead. The key is not in any interface description, only in app note
    // UBX-21038688, so it gets a message of its own rather than putting the constellation
    // config at the mercy of a firmware that has never heard of it.
    if (_identity.board == Board::u_blox9_F9P_L1L5 || _identity.board == Board::u_blox10_L1L5 ||
        _identity.board == Board::u_blox_X20) {
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_L5_HEALTH_OVERRIDE, 1);

        if (!sendCfgValsetAcked(false).succeeded() && !ioError()) {
            log(GPSProtocolLogLevel::Warning, "GPS L5 health override not supported by this receiver");
        }
    }

    // Configure message rates
    // Send a new CFG-VALSET message to make sure it does not get too large
    initCfgValset();
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_PVT_I2C, 1);

    // There is no RTCM on M10 and M9* (except F9P)
    if (_identity.board != Board::u_blox10 && _identity.board != Board::u_blox9 &&
        _identity.board != Board::u_blox10_L1L5) {
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_RELPOSNED_I2C, 0);
    }

    _decodeContext.useNavPvt = true;
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_DOP_I2C, 1);
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SAT_I2C, (_satellites != nullptr) ? 10 : 0);
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_STATUS_I2C, 1);
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_MON_RF_I2C, 1);
    _got_sec_sig = false;

    if (!sendCfgValsetAcked().succeeded()) {
        return -1;
    }

    // Optional on older firmware. A rejected EOE key leaves the bounded epoch deadline in use.
    initCfgValset();
    cfgValsetPort(UBX::NAV_EOE_MSGOUT_I2C, 1);
    (void) sendCfgValsetAcked(false);
    if (ioError()) {
        return -1;
    }

    // Correction input status. RXM-COR reports every protocol (RTCM3, SPARTN, HAS) and is
    // the only form on the X20, which has no RXM-RTCM; receivers without it get RXM-RTCM.
    initCfgValset();
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_COR_I2C, 1);

    if (!sendCfgValsetAcked(false).succeeded() &&
        ((_identity.board == Board::u_blox9) || (_identity.board == Board::u_blox9_F9P_L1L2) ||
         (_identity.board == Board::u_blox9_F9P_L1L5))) {
        initCfgValset();
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_RTCM_I2C, 1);
        sendCfgValsetAcked(false);
    }

    // UBX-SEC-SIG carries jammingState. MON-RF jammingState is always 0 on
    // firmware that supports this message (F9 HPG 1.51, X20). The rate key is
    // absent on older protocol-27 receivers; a NAK must not abort config.
    initCfgValset();
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_SEC_SIG_I2C, 1);

    (void) sendCfgValsetAcked(false);

    // Explicitly disable the messages this driver never consumes. We do not enable them,
    // but they can be left on in the receiver's non-volatile config by other software
    // (u-center, or another firmware such as ArduPilot, which writes CFG-VALSET to the
    // BBR/FLASH layers). Clearing them here rather than waiting for payloadRxInit() to
    // notice them avoids wasting UART bandwidth on every boot.
    // Separate, non-fatal VALSET: these messages do not exist on every generation, and
    // bandwidth we failed to save is not worth losing the receiver over.
    initCfgValset();
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_TIMEGPS_I2C, 0);
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_SFRBX_I2C, 0);

    // M10 and F10 have no raw measurement output
    if (UBX::receiverProfile(_identity.board).usb) {
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_RAWX_I2C, 0);
    }

    if (!sendCfgValsetAcked(false).succeeded() && !ioError()) {
        log(GPSProtocolLogLevel::Warning, "Could not disable unused messages");
    }

    // Dual antenna heading, not used in a moving base setup where NAV-RELPOSNED provides it. The rate is
    // always written so a mode change is idempotent; a NAK from a position-only X20P must not abort config.
    // The receiver side heading offset is zeroed, leaving GPS_YAW_OFFSET as the only offset applied.
    if (_identity.board == Board::u_blox_X20) {
        initCfgValset();
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_DAHEADING_I2C, 1);
        cfgValset<int32_t>(UBX_CFG_KEY_NAVSPG_DAHEADING_OFFSET, 0);

        (void) sendCfgValsetAcked(false);

        // Galileo HAS (HPG 2.10+) is only processed while host corrections are off, so the pair is
        // written together and every other mode restores host input: a HOST=0 left behind by
        // u-center would silently discard RTCM. Absent before 2.10, so a NAK must not abort config.
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_NAVCOR_ENABLE_HOST, 1);
        cfgValset<uint8_t>(UBX_CFG_KEY_NAVCOR_ENABLE_GAL_HAS, 0);
        sendCfgValsetAcked(false);
    }

    return 0;
}

void GPSNativeUBX::initCfgValset()
{
    _valset = {};
}

bool GPSNativeUBX::sendCfgValset(bool required, unsigned timeout)
{
    const auto payload = _valset.payload();
    if (payload.empty() || (_valsetAckAmbiguous && !_controller.configurationReadbackRequired())) {
        beginCommandWrite(
            {std::to_string(UBX_MSG_CFG_VALSET), std::chrono::milliseconds(timeout), _valset.settings, required});
        failCommandWrite(GPSCommandOutcome::Rejected);
        return false;
    }
    return sendMessage(UBX_MSG_CFG_VALSET, payload,
                       {{}, std::chrono::milliseconds(timeout), _valset.settings, required});
}

GPSCommandResult GPSNativeUBX::sendCfgValsetAcked(bool required)
{
    if (!sendCfgValset(required)) {
        return completeCommand(ioError() == ReadCancelled ? GPSCommandOutcome::Cancelled
                                                          : GPSCommandOutcome::TransportError);
    }

    return waitForAck(UBX_MSG_CFG_VALSET);
}

bool GPSNativeUBX::cfgValsetRaw(uint32_t key_id, uint32_t value)
{
    if ((key_id == UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X || key_id == UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X) &&
        !UBX::receiverProfile(_identity.board).rtcmOutput) {
        return _valset.invalidate();
    }
    if (!_valset.append(key_id, value)) {
        return false;
    }
    if (key_id == UBX_CFG_KEY_RATE_MEAS || key_id == UBX_CFG_KEY_RATE_NAV) {
        _valset.settings.add(GPSReceiverSetting::OutputRateHz);
    }

    return true;
}

bool GPSNativeUBX::cfgValsetPort(uint32_t key_id, uint8_t value)
{
    for (const auto port : UBX::OUTPUT_PORTS) {
        if ((!port.requiresUsb || UBX::receiverProfile(_identity.board).usb) &&
            !cfgValset<uint8_t>(key_id + port.messageKeyOffset, value)) {
            return false;
        }
    }

    return true;
}

bool GPSNativeUBX::cfgValset(std::span<const CfgValsetItem> items)
{
    for (const auto& item : items) {
        if (!cfgValsetRaw(item.key, item.value)) {
            return false;
        }
    }

    return true;
}

bool GPSNativeUBX::cfgValset(std::span<const uint32_t> keys, uint8_t value)
{
    for (const auto key : keys) {
        if (!cfgValsetRaw(key, value)) {
            return false;
        }
    }

    return true;
}

bool GPSNativeUBX::cfgValsetPort(std::span<const uint32_t> keys, uint8_t value)
{
    for (const auto key : keys) {
        if (!cfgValsetPort(key, value)) {
            return false;
        }
    }

    return true;
}

int GPSNativeUBX::disableTimeMode()
{
    if (_identity.timeModeUnsupported) {
        return 0;
    }
    if (_identity.protocol27) {
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 0);
        if (!sendCfgValsetAcked().succeeded()) {
            return -1;
        }
        return verifyConfigValue(UBX_CFG_KEY_TMODE_MODE, 0);
    }

    const ubx_payload_tx_cfg_tmode3_t disabled{};
    if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(disabled)) || !waitForAck(UBX_MSG_CFG_TMODE3).succeeded()) {
        return -1;
    }
    _timeModeReadback = {.pending = true};
    const auto clearReadback = qScopeGuard([this] { _timeModeReadback.pending = false; });
    if (!sendMessage(UBX_MSG_CFG_TMODE3, nullptr, 0,
                     {"UBX-CFG-TMODE3 disabled readback", std::chrono::milliseconds(UBX_CONFIG_TIMEOUT)})) {
        return -1;
    }
    const auto result =
        awaitCommand([this] {
            if (_controller.lateRejection()) {
                return GPSCommandOutcome::Rejected;
            }
            return !_timeModeReadback.response.has_value() ? GPSCommandOutcome::Pending
                   : _timeModeReadback.response == 0       ? GPSCommandOutcome::ReadbackVerified
                                                           : GPSCommandOutcome::Rejected;
        });
    return result.evidence.outcome == GPSCommandOutcome::ReadbackVerified ? 0 : -1;
}

int GPSNativeUBX::verifyConfigValue(uint32_t key, uint8_t value)
{
    const std::array keys{key};
    _controller.beginReadback(keys);
    const auto clearReadback = qScopeGuard([this] { _controller.finishReadback(); });
    std::array<uint8_t, 8> request{};
    (void) LittleEndian::write(request, 4, key);
    if (!sendMessage(UBX_MSG_CFG_VALGET, request,
                     {"UBX-CFG-VALGET " + std::to_string(key), std::chrono::milliseconds(UBX_CONFIG_TIMEOUT)})) {
        return -1;
    }
    const auto result =
        awaitCommand([this, value] {
            if (_controller.lateRejection()) {
                return GPSCommandOutcome::Rejected;
            }
            return !_controller.readbackReady()                      ? GPSCommandOutcome::Pending
                   : _controller.readback().values[0].value == value ? GPSCommandOutcome::ReadbackVerified
                                                                     : GPSCommandOutcome::Rejected;
        });
    return result.evidence.outcome == GPSCommandOutcome::ReadbackVerified ? 0 : -1;
}

int GPSNativeUBX::waitForSurveyStop()
{
    _survey_in_stopped = false;
    const uint64_t stop_deadline = nowUs() + 3000000;

    while (!_survey_in_stopped && nowUs() < stop_deadline) {
        if (!sendMessage(UBX_MSG_NAV_SVIN, nullptr, 0,
                         {"UBX-NAV-SVIN stopped", std::chrono::milliseconds(UBX_CONFIG_TIMEOUT)})) {
            return -1;
        }

        const uint64_t poll_deadline = nowUs() + 100000;

        while (!_survey_in_stopped && nowUs() < poll_deadline) {
            bool read_error;
            receiveInternal(100, read_error);

            if (read_error) {
                return -1;
            }
        }
    }

    if (!_survey_in_stopped) {
        log(GPSProtocolLogLevel::Warning, "Time mode did not stop");
        failCommandWrite(GPSCommandOutcome::TimedOut);
        return -1;
    }

    failCommandWrite(GPSCommandOutcome::ReadbackVerified);
    return 0;
}

int GPSNativeUBX::restartSurveyInPreV27()
{
    ubx_payload_tx_cfg_tmode3_t payload_tx_cfg_tmode3{};

    // disable RTCM (MSM7) output
    configureMessageRate(UBX_MSG_RTCM3_1005, 0);
    configureMessageRate(UBX_MSG_RTCM3_1077, 0);
    configureMessageRate(UBX_MSG_RTCM3_1087, 0);
    configureMessageRate(UBX_MSG_RTCM3_1230, 0);
    configureMessageRate(UBX_MSG_RTCM3_1097, 0);
    configureMessageRate(UBX_MSG_RTCM3_1127, 0);

    if (disableTimeMode() < 0 ||
        (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) && waitForSurveyStop() < 0)) {
        return -1;
    }

    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        payload_tx_cfg_tmode3 = {};
        payload_tx_cfg_tmode3.flags = 1; /* start survey-in */
        payload_tx_cfg_tmode3.svinMinDur = std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).durationSecs;
        payload_tx_cfg_tmode3.svinAccLimit =
            static_cast<uint32_t>(std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).accuracyMeters * 10000.0);

        if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(payload_tx_cfg_tmode3))) {
            return -1;
        }

        if (!waitForAck(UBX_MSG_CFG_TMODE3).succeeded()) {
            return -1;
        }

        /* enable status output of survey-in */
        if (!configureMessageRateAndAck(UBX_MSG_NAV_SVIN, 5, true)) {
            return -1;
        }

    } else {
        const auto& settings = std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode);

        payload_tx_cfg_tmode3 = {};
        payload_tx_cfg_tmode3.flags = 2 /* fixed mode */ | (1 << 8) /* lat/lon mode */;
        int64_t lat64 = (int64_t) (settings.position.latitudeDegrees * 1e9);
        payload_tx_cfg_tmode3.ecefXOrLat = (int32_t) (lat64 / 100);
        payload_tx_cfg_tmode3.ecefXOrLatHP = lat64 % 100;  // range [-99, 99]
        int64_t lon64 = (int64_t) (settings.position.longitudeDegrees * 1e9);
        payload_tx_cfg_tmode3.ecefYOrLon = (int32_t) (lon64 / 100);
        payload_tx_cfg_tmode3.ecefYOrLonHP = lon64 % 100;
        int64_t alt64 = (int64_t) ((double) settings.position.altitudeMeters * 1e4);
        payload_tx_cfg_tmode3.ecefZOrAlt = (int32_t) (alt64 / 100);  // cm
        payload_tx_cfg_tmode3.ecefZOrAltHP = alt64 % 100;            // 0.1mm

        payload_tx_cfg_tmode3.fixedPosAcc = fixedAccuracyWireUnits(settings.accuracyMeters);

        if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(payload_tx_cfg_tmode3))) {
            return -1;
        }

        if (!waitForAck(UBX_MSG_CFG_TMODE3).succeeded()) {
            return -1;
        }

        // directly enable RTCM3 output
        return activateRTCMOutput();
    }

    return 0;
}

int GPSNativeUBX::restartSurveyIn()
{
    if (!_identity.protocol27) {
        return restartSurveyInPreV27();
    }

    // disable RTCM output
    initCfgValset();
    cfgValsetPort(RTCM_BASE_MSGOUT_I2C, 0);
    sendCfgValsetAcked(false);

    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        // Reapplying survey-in mode does not restart an existing survey.
        if (disableTimeMode() < 0 || waitForSurveyStop() < 0) {
            return -1;
        }

        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 1 /* Survey-in */);
        cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_SVIN_MIN_DUR,
                            std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).durationSecs);
        cfgValset<uint32_t>(
            UBX_CFG_KEY_TMODE_SVIN_ACC_LIMIT,
            static_cast<uint32_t>(std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).accuracyMeters * 10000.0));
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 5);

        if (!sendCfgValsetAcked().succeeded()) {
            return -1;
        }

    } else {
        const auto& settings = std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode);
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 2 /* Fixed Mode */);
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_POS_TYPE, 1 /* Lat/Lon/Height */);
        int64_t lat64 = (int64_t) (settings.position.latitudeDegrees * 1e9);
        cfgValset<int32_t>(UBX_CFG_KEY_TMODE_LAT, (int32_t) (lat64 / 100));
        cfgValset<int8_t>(UBX_CFG_KEY_TMODE_LAT_HP, lat64 % 100 /* range [-99, 99] */);
        int64_t lon64 = (int64_t) (settings.position.longitudeDegrees * 1e9);
        cfgValset<int32_t>(UBX_CFG_KEY_TMODE_LON, (int32_t) (lon64 / 100));
        cfgValset<int8_t>(UBX_CFG_KEY_TMODE_LON_HP, lon64 % 100 /* range [-99, 99] */);
        int64_t alt64 = (int64_t) ((double) settings.position.altitudeMeters * 1e4);
        cfgValset<int32_t>(UBX_CFG_KEY_TMODE_HEIGHT, (int32_t) (alt64 / 100) /* cm */);
        cfgValset<int8_t>(UBX_CFG_KEY_TMODE_HEIGHT_HP, alt64 % 100 /* 0.1mm */);
        cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_FIXED_POS_ACC, fixedAccuracyWireUnits(settings.accuracyMeters));

        if (!sendCfgValsetAcked().succeeded()) {
            return -1;
        }

        // directly enable RTCM3 output
        return activateRTCMOutput();
    }

    return 0;
}

GPSCommandResult GPSNativeUBX::waitForAck(uint16_t msg)
{
    const Operation operation(*this, remainingMilliseconds(_commandDeadline.untilUs));
    _operationDeadline.untilUs = std::min(_operationDeadline.untilUs, _commandDeadline.untilUs);
    _controller.beginAcknowledgement(msg);
    if (msg == UBX_MSG_CFG_VALSET && _controller.configurationReadbackRequired()) {
        _controller.finishAcknowledgement();
        return verifyCfgValset({"UBX-CFG-VALSET readback",
                                std::chrono::milliseconds(remainingMilliseconds(_commandDeadline.untilUs)),
                                _commandWrite.affectedSettings, _commandWrite.evidence.required});
    }
    const auto clearAcknowledgement = qScopeGuard([this] { _controller.finishAcknowledgement(); });
    const auto result = awaitCommand([this] { return _controller.acknowledgement(); });
    if (msg == UBX_MSG_CFG_VALSET && result.evidence.outcome == GPSCommandOutcome::TimedOut) {
        _valsetAckAmbiguous = true;
    }
    return result;
}

GPSCommandResult GPSNativeUBX::verifyCfgValset(GPSConfigurationStep step)
{
    const Operation operation(*this, static_cast<unsigned>(step.timeout.count()));
    UBX::ConfigurationValueCursor cursor(std::span<const uint8_t>(_valset.bytes).subspan(4, _valset.size - 4));
    GPSCommandResult result;
    while (!cursor.empty()) {
        UBX::ConfigurationValues expected;
        std::array<uint8_t, 40> request{};
        while (!cursor.empty() && expected.count < expected.values.size()) {
            const auto entry = cursor.next();
            if (!entry) {
                beginCommandWrite(step);
                return completeCommand(GPSCommandOutcome::Rejected);
            }
            (void) LittleEndian::write(request, 4 + expected.count * 4, entry->key);
            expected.values[expected.count++] = *entry;
        }
        std::array<uint32_t, 9> keys{};
        for (size_t index = 0; index < expected.count; ++index) {
            keys[index] = expected.values[index].key;
        }
        _controller.beginReadback(std::span(keys).first(expected.count));
        const auto clearReadback = qScopeGuard([this] { _controller.finishReadback(); });
        if (!sendMessage(UBX_MSG_CFG_VALGET, std::span(request).first(4 + expected.count * 4), step)) {
            return completeCommand(GPSCommandOutcome::TransportError);
        }
        result = awaitCommand([this, &expected] {
            if (_controller.lateRejection()) {
                return GPSCommandOutcome::Rejected;
            }
            if (!_controller.readbackReady()) {
                return GPSCommandOutcome::Pending;
            }
            return _controller.readback().values == expected.values ? GPSCommandOutcome::ReadbackVerified
                                                                    : GPSCommandOutcome::Rejected;
        });
        if (result.evidence.outcome != GPSCommandOutcome::ReadbackVerified) {
            return result;
        }
    }
    return result;
}

void GPSNativeUBX::requestCommsDiagnostics()
{
    const uint64_t now = nowUs();

    if (now < _comms.nextUs) {
        return;
    }

    // A congested receiver must not be flooded with diagnostic requests.
    _comms.nextUs = now + 5000000;
    _comms.deadlineUs = sendMessage(UBX_MSG_MON_COMMS, nullptr, 0) ? now + 2000000 : 0;
}

int GPSNativeUBX::activateRTCMOutput()
{
    ubx_payload_tx_cfg_rate_t payload_tx_cfg_rate{};

    /* For base stations we switch to 1 Hz update rate, which is enough for RTCM output.
     * For the survey-in, we still want 5/10 Hz, because this speeds up the process */

    if (_identity.protocol27) {
        initCfgValset();

        cfgValset<uint16_t>(UBX_CFG_KEY_RATE_MEAS, 1000);

        cfgValsetPort(RTCM_BASE_MSGOUT_I2C, 1);
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 0);

        if (!sendCfgValset(false)) {
            return -1;
        }

        if (!waitForAck(UBX_MSG_CFG_VALSET).succeeded()) {
            return -1;
        }

    } else {
        payload_tx_cfg_rate = {};
        payload_tx_cfg_rate.measRate = 1000;
        payload_tx_cfg_rate.navRate = UBX_TX_CFG_RATE_NAVRATE;
        payload_tx_cfg_rate.timeRef = UBX_TX_CFG_RATE_TIMEREF;

        if (!sendMessage(UBX_MSG_CFG_RATE, UBX::encode(payload_tx_cfg_rate))) {
            return -1;
        }

        configureMessageRate(UBX_MSG_NAV_SVIN, 0);

        // stationary RTK reference station ARP (can be sent at lower rate)
        if (!configureMessageRate(UBX_MSG_RTCM3_1005, 5)) {
            return -1;
        }

        // GPS
        if (!configureMessageRate(UBX_MSG_RTCM3_1077, 1)) {
            return -1;
        }

        // GLONASS
        if (!configureMessageRate(UBX_MSG_RTCM3_1087, 1)) {
            return -1;
        }

        // GLONASS code-phase biases
        if (!configureMessageRate(UBX_MSG_RTCM3_1230, 1)) {
            return -1;
        }

        // Galileo
        if (!configureMessageRate(UBX_MSG_RTCM3_1097, 1)) {
            return -1;
        }

        // BeiDou
        if (!configureMessageRate(UBX_MSG_RTCM3_1127, 1)) {
            return -1;
        }
    }

    return 0;
}

bool GPSNativeUBX::configureMessageRate(const uint16_t msg, const uint8_t rate, bool required)
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

bool GPSNativeUBX::configureMessageRateAndAck(uint16_t msg, uint8_t rate, bool report_ack_error)
{
    if (!configureMessageRate(msg, rate, report_ack_error)) {
        return false;
    }

    return waitForAck(UBX_MSG_CFG_MSG).succeeded();
}

bool GPSNativeUBX::sendMessage(uint16_t msg, const uint8_t* payload, uint16_t length, GPSConfigurationStep step)
{
    if (msg == UBX_MSG_CFG_RATE) {
        step.affectedSettings.add(GPSReceiverSetting::OutputRateHz);
    }
    if (step.command.empty()) {
        step.command = std::to_string(msg);
    }
    beginCommandWrite(std::move(step));
    // Identity replies can need two seconds, but multipart writes retain their shorter shared cap.
    const Operation operation(*this, UBX_CONFIG_TIMEOUT);
    _operationDeadline.untilUs =
        std::min(_operationDeadline.untilUs, _commandWrite.evidence.startedAtUs + uint64_t(UBX_CONFIG_TIMEOUT) * 1000);
    std::array<uint8_t, 6> header{UBX_SYNC1, UBX_SYNC2};
    (void) LittleEndian::write(header, 2, msg);
    (void) LittleEndian::write(header, 4, length);
    ubx_checksum_t checksum = {0, 0};
    calcChecksum(header.data() + 2, header.size() - 2, &checksum);

    if (payload != nullptr) {
        calcChecksum(payload, length, &checksum);
    }

    // Send message
    if (write(header.data(), header.size()) != static_cast<int>(header.size())) {
        return false;
    }

    if (payload && write((void*) payload, length) != length) {
        return false;
    }

    const std::array<uint8_t, 2> checksumBytes{checksum.ck_a, checksum.ck_b};
    if (write(checksumBytes.data(), checksumBytes.size()) != int(checksumBytes.size())) {
        return false;
    }

    return true;
}
