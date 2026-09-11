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

#include "GPSWire.h"
#include "UBXMessageSchema.h"
#include "UBXPrivate.h"

int GPSDriverUBX::enableNmeaOutput(unsigned baudrate)
{
    if (!_configured || _output_mode != OutputMode::GPS || baudrate == 0) {
        return -1;
    }

    _configured = false;
    const int result = [&]() -> int {
        if (_proto_ver_27_or_higher) {
            // CFG-MSGOUT NMEA RMC, GGA, GSA and GSV keys for the I2C port; cfgValsetPort
            // selects UART1 and USB without changing unrelated receiver ports.
            static constexpr uint32_t nmea_messages[] = {
                UBX_CFG_KEY_MSGOUT_NMEA_RMC_I2C, UBX_CFG_KEY_MSGOUT_NMEA_GGA_I2C, UBX_CFG_KEY_MSGOUT_NMEA_GSA_I2C,
                UBX_CFG_KEY_MSGOUT_NMEA_GSV_I2C};
            initCfgValset();
            cfgValsetPort(nmea_messages, 1);
            cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1OUTPROT_NMEA, 1);
            if (UBX::receiverProfile(_board).usb)
                cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA, 1);
            return sendCfgValsetAcked();
        }

        // Legacy receivers use CFG-MSG and CFG-PRT rather than configuration keys.
        static constexpr uint16_t legacy_messages[] = {0x04f0, 0x00f0, 0x02f0, 0x03f0};
        for (const uint16_t message : legacy_messages) {
            if (!configureMessageRateAndAck(message, 1, true)) {
                return -1;
            }
        }
        ubx_payload_tx_cfg_prt_t ports[2]{};
        for (unsigned i = 0; i < 2; ++i) {
            ports[i].portID = i == 0 ? UBX_TX_CFG_PRT_PORTID : UBX_TX_CFG_PRT_PORTID_USB;
            ports[i].mode = UBX_TX_CFG_PRT_MODE;
            ports[i].baudRate = baudrate;
            ports[i].inProtoMask = UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_RTCM;
            ports[i].outProtoMask = UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_NMEA;
        }
        if (!sendMessage(UBX_MSG_CFG_PRT, UBX::encode(ports))) {
            return -1;
        }
        return waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, true);
    }();
    _configured = result == 0;
    return result;
}

int GPSDriverUBX::configure(unsigned& baudrate, const GPSConfig& config)
{
    _baseConfig = config.base;
    _dyn_model = config.dynamicModel;
    _output_rate = config.outputRateHz;
    _survey_duration = 0;
    return configure(baudrate, config, OutputProtocol::Native);
}

GPSDriverUBX::BaseStationCapability GPSDriverUBX::baseStationCapability() const
{
    if (_board == Board::u_blox8)
        return _is_m8p ? BaseStationCapability::Supported
                       : (_model_name[0] ? BaseStationCapability::Unsupported : BaseStationCapability::Unknown);
    const auto profile = UBX::receiverProfile(_board);
    return profile.rtcmOutput            ? BaseStationCapability::Supported
           : profile.baseCapabilityKnown ? BaseStationCapability::Unsupported
                                         : BaseStationCapability::Unknown;
}

bool GPSDriverUBX::supportsConstellationSelection() const
{
    // M10 combinations have additional restrictions; the legacy path does not
    // provide strict acknowledgement of every requested constellation change.
    return _proto_ver_27_or_higher && UBX::receiverProfile(_board).constellationSelection;
}

bool GPSDriverUBX::supportsOutputRateSelection() const
{
    return _proto_ver_27_or_higher && _model_name[0] && _board != Board::unknown;
}

bool GPSDriverUBX::readConfiguration(ConfigurationReadback& report, unsigned timeout_ms)
{
    const Operation operation(*this, timeout_ms);
    report = {};
    if (!_configured || !supportsOutputRateSelection() || !timeout_ms || ioError()) {
        return false;
    }
    _configuration_readback_keys[0] = UBX_CFG_KEY_NAVSPG_DYNMODEL;
    _configuration_readback_keys[1] = UBX_CFG_KEY_RATE_MEAS;
    _configuration_readback_keys[2] = UBX_CFG_KEY_RATE_NAV;
    _configuration_readback_count = 3;
    if (supportsConstellationSelection()) {
        const uint32_t keys[] = {UBX_CFG_KEY_SIGNAL_GPS_ENA, UBX_CFG_KEY_SIGNAL_QZSS_ENA, UBX_CFG_KEY_SIGNAL_SBAS_ENA,
                                 UBX_CFG_KEY_SIGNAL_GAL_ENA, UBX_CFG_KEY_SIGNAL_BDS_ENA,  UBX_CFG_KEY_SIGNAL_GLO_ENA};
        for (uint32_t key : keys) {
            _configuration_readback_keys[_configuration_readback_count++] = key;
        }
    }
    uint8_t request[4 + sizeof(_configuration_readback_keys)]{};
    for (unsigned i = 0; i < _configuration_readback_count; ++i) {
        GPSWire::write(request, 4 + i * 4, _configuration_readback_keys[i]);
    }
    _configuration_readback_ready = false;
    _configuration_readback_pending = true;
    const gps_abstime deadline = nowUs() + uint64_t(timeout_ms) * 1000;
    if (sendMessage(UBX_MSG_CFG_VALGET, request, 4 + _configuration_readback_count * 4)) {
        while (!_configuration_readback_ready && !ioError()) {
            const gps_abstime now = nowUs();
            if (now >= deadline) {
                break;
            }
            const unsigned remaining_ms = unsigned((deadline - now + 999) / 1000);
            bool read_error = false;
            receiveInternal(remaining_ms < 50 ? remaining_ms : 50, read_error);
            if (read_error) {
                break;
            }
        }
    }
    _configuration_readback_pending = false;
    if (!_configuration_readback_ready) {
        return false;
    }
    report.dynamic_model = uint8_t(_configuration_readback_values[0]);
    report.measurement_interval_ms = uint16_t(_configuration_readback_values[1]);
    report.navigation_rate = uint16_t(_configuration_readback_values[2]);
    report.constellations_reported = _configuration_readback_count == 9;
    if (report.constellations_reported) {
        // QGC's GPS selection includes QZSS; differing receiver enables are not a match.
        report.constellation_mask = (_configuration_readback_values[3] && _configuration_readback_values[4]) ? 1 : 0;
        for (unsigned i = 5; i < 9; ++i) {
            if (_configuration_readback_values[i]) {
                report.constellation_mask |= 1u << (i - 4);
            }
        }
    }
    return true;
}

int GPSDriverUBX::configure(unsigned& baudrate, const GPSConfig& config, OutputProtocol output_protocol)
{
    _baseConfig = config.base;
    _dyn_model = config.dynamicModel;
    _output_rate = config.outputRateHz;
    _survey_duration = 0;
    resetIOError();
    _settingOutcomes.fill(GPSCommandOutcome::Pending);
    _constellation_configuration_rejected = false;
    _constellation_request_rejected = false;
    _configured = false;
    _decodeNavigation = false;
    _assembleEpochs = false;
    _navigationEpochs = {};
    if (output_protocol != OutputProtocol::Native &&
        (output_protocol != OutputProtocol::NMEA || config.output_mode != OutputMode::GPS)) {
        return -1;
    }
    _output_mode = config.output_mode;

    ubx_payload_tx_cfg_prt_t cfg_prt[2];

    uint16_t out_proto_mask = _output_mode == OutputMode::GPS ? UBX_TX_CFG_PRT_PROTO_UBX
                                                              : (UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_RTCM);

    uint16_t in_proto_mask = (_output_mode == OutputMode::GPS) ? (UBX_TX_CFG_PRT_PROTO_UBX | UBX_TX_CFG_PRT_PROTO_RTCM)
                                                               : UBX_TX_CFG_PRT_PROTO_UBX;

    const bool auto_baudrate = baudrate == 0;

    {
        /* try different baudrates */
        const unsigned baudrates[] = {38400, 57600, 9600, 115200, 230400, 460800, 921600};

        unsigned baud_i;
        unsigned desired_baudrate = auto_baudrate ? UBX_BAUDRATE_M8_AND_NEWER : baudrate;

        for (baud_i = 0; baud_i < sizeof(baudrates) / sizeof(baudrates[0]); baud_i++) {
            unsigned test_baudrate = baudrates[baud_i];

            if (!auto_baudrate && baudrate != test_baudrate) {
                continue;  // skip to next baudrate
            }

            setBaudrate(test_baudrate);

            /* flush input and wait for at least 20 ms silence */
            decodeInit();
            receive(20);
            decodeInit();

            // try CFG-VALSET: if we get an ACK we know we can use protocol version 27+
            static constexpr CfgValsetItem uart1_ubx[] = {
                {UBX_CFG_KEY_CFG_UART1_STOPBITS, 1},    {UBX_CFG_KEY_CFG_UART1_DATABITS, 0},
                {UBX_CFG_KEY_CFG_UART1_PARITY, 0},      {UBX_CFG_KEY_CFG_UART1INPROT_UBX, 1},
                {UBX_CFG_KEY_CFG_UART1INPROT_NMEA, 0},  {UBX_CFG_KEY_CFG_UART1OUTPROT_UBX, 1},
                {UBX_CFG_KEY_CFG_UART1OUTPROT_NMEA, 0},
            };
            initCfgValset();
            cfgValset(uart1_ubx);
            // TODO: are we ever connected to UART2?

            // Note: USB protocol settings are handled later in the configureDevice function.

            bool cfg_valset_success = false;

            if (sendCfgValset()) {
                // Note: The M10 comes up sending NMEA sentences at 9600. It can't
                // respond with an ACK until the current sentence has completed transmission.
                // This can take over a second so need a large timeout on this particular wait.
                // Once it has acked this it will turn off the NMEA sentences and all is good
                // for future transactions.
                if (waitForAck(UBX_MSG_CFG_VALSET, 2000, true) == 0) {
                    cfg_valset_success = true;
                }
            }

            if (cfg_valset_success) {
                _proto_ver_27_or_higher = true;
                // Now we only have to change the baudrate
                initCfgValset();
                cfgValset<uint32_t>(UBX_CFG_KEY_CFG_UART1_BAUDRATE, desired_baudrate);

                if (!sendCfgValset()) {
                    continue;
                }

                /* no ACK is expected here, but read the buffer anyway in case we actually get an ACK */
                waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false);

            } else {
                _proto_ver_27_or_higher = false;

                /* Send a CFG-PRT message to set the UBX protocol for in and out
                 * and leave the baudrate as it is, we just want an ACK-ACK for this */
                memset(cfg_prt, 0, 2 * sizeof(ubx_payload_tx_cfg_prt_t));
                cfg_prt[0].portID = UBX_TX_CFG_PRT_PORTID;
                cfg_prt[0].mode = UBX_TX_CFG_PRT_MODE;
                cfg_prt[0].baudRate = test_baudrate;
                cfg_prt[0].inProtoMask = in_proto_mask;
                cfg_prt[0].outProtoMask = out_proto_mask;
                cfg_prt[1].portID = UBX_TX_CFG_PRT_PORTID_USB;
                cfg_prt[1].mode = UBX_TX_CFG_PRT_MODE;
                cfg_prt[1].baudRate = test_baudrate;
                cfg_prt[1].inProtoMask = in_proto_mask;
                cfg_prt[1].outProtoMask = out_proto_mask;

                if (!sendMessage(UBX_MSG_CFG_PRT, UBX::encode(cfg_prt))) {
                    continue;
                }

                if (waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false) < 0) {
                    /* try next baudrate */
                    continue;
                }

                if (auto_baudrate) {
                    desired_baudrate = UBX_TX_CFG_PRT_BAUDRATE;
                }

                /* Send a CFG-PRT message again, this time change the baudrate */
                cfg_prt[0].baudRate = desired_baudrate;
                cfg_prt[1].baudRate = desired_baudrate;

                if (!sendMessage(UBX_MSG_CFG_PRT, UBX::encode(cfg_prt))) {
                    continue;
                }

                /* no ACK is expected here, but read the buffer anyway in case we actually get an ACK */
                waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false);
            }

            if (desired_baudrate != test_baudrate) {
                setBaudrate(desired_baudrate);

                decodeInit();
                receive(20);
                decodeInit();
            }

            /* at this point we have correct baudrate on both ends */
            baudrate = desired_baudrate;
            break;
        }

        if (baud_i >= sizeof(baudrates) / sizeof(baudrates[0])) {
            return -1;  // connection and/or baudrate detection failed
        }
    }

    /* Request module version information by sending an empty MON-VER message */
    if (!sendMessage(UBX_MSG_MON_VER, nullptr, 0)) {
        return -1;
    }

    /* Wait for the reply so that we know to which device we're connected (_board will be set).
     * Note: we won't actually get an ACK-ACK, but UBX_MSG_MON_VER will also set the ack state.
     */
    if (waitForAck(UBX_MSG_MON_VER, UBX_CONFIG_TIMEOUT, true) < 0) {
        return -1;
    }
    if (_output_mode == OutputMode::RTCM && baseStationCapability() == BaseStationCapability::Unsupported) {
        return -1;
    }
    if ((_output_rate && !supportsOutputRateSelection()) ||
        (config.require_gnss_config && !supportsConstellationSelection())) {
        return -1;
    }

    /* Now that we know the board, update the baudrate on M8 boards (on F9+ we already used the
     * higher baudrate with CFG-VALSET) */
    if (auto_baudrate && _board == Board::u_blox8) {
        cfg_prt[0].baudRate = UBX_BAUDRATE_M8_AND_NEWER;
        cfg_prt[1].baudRate = UBX_BAUDRATE_M8_AND_NEWER;

        if (sendMessage(UBX_MSG_CFG_PRT, UBX::encode(cfg_prt))) {
            /* no ACK is expected here, but read the buffer anyway in case we actually get an ACK */
            waitForAck(UBX_MSG_CFG_PRT, UBX_CONFIG_TIMEOUT, false);

            setBaudrate(UBX_BAUDRATE_M8_AND_NEWER);
            baudrate = UBX_BAUDRATE_M8_AND_NEWER;
        }
    }

    if (_output_mode == OutputMode::RTCM) {
        if (!_rtcm_parsing) {
            _rtcm_parsing.emplace();
        }

        _rtcm_parsing->reset();
    }

    if (_output_mode == OutputMode::RTCM) {
        // RTCM mode force stationary dynamic model
        _dyn_model = 2;
    }

    int ret;

    /* Configure the device, use config commands depending on protocol version */
    if (_proto_ver_27_or_higher) {
        ret = configureDevice(config);

    } else {
        ret = configureDevicePreV27(config.gnss_systems);
    }

    if (ret != 0) {
        return ret;
    }

    // A position source must leave any previous base-station time mode.
    if (_output_mode != OutputMode::RTCM && (_is_m8p || _board == Board::u_blox9_F9P_L1L2 ||
                                             _board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox_X20)) {
        if (disableTimeMode() < 0) {
            return -1;
        }
    }

    if (_output_mode == OutputMode::RTCM) {
        if (restartSurveyIn() < 0) {
            return -1;
        }
    }

    _configured = true;
    _decodeNavigation = true;
    _assembleEpochs = true;
    return output_protocol == OutputProtocol::NMEA ? enableNmeaOutput(baudrate) : 0;
}

int GPSDriverUBX::configureDevicePreV27(const GNSSSystemsMask& gnssSystems)
{
    /* Send a CFG-RATE message to define update rate */
    memset(&_buf.payload_tx_cfg_rate, 0, sizeof(_buf.payload_tx_cfg_rate));
    _buf.payload_tx_cfg_rate.measRate = UBX_TX_CFG_RATE_MEASINTERVAL;
    _buf.payload_tx_cfg_rate.navRate = UBX_TX_CFG_RATE_NAVRATE;
    _buf.payload_tx_cfg_rate.timeRef = UBX_TX_CFG_RATE_TIMEREF;

    if (!sendMessage(UBX_MSG_CFG_RATE, UBX::encode(_buf.payload_tx_cfg_rate))) {
        return -1;
    }

    if (waitForAck(UBX_MSG_CFG_RATE, UBX_CONFIG_TIMEOUT, true) < 0) {
        return -1;
    }

    /* send a NAV5 message to set the options for the internal filter */
    memset(&_buf.payload_tx_cfg_nav5, 0, sizeof(_buf.payload_tx_cfg_nav5));
    _buf.payload_tx_cfg_nav5.mask = UBX_TX_CFG_NAV5_MASK;
    _buf.payload_tx_cfg_nav5.dynModel = _dyn_model;
    _buf.payload_tx_cfg_nav5.fixMode = UBX_TX_CFG_NAV5_FIXMODE;

    if (!sendMessage(UBX_MSG_CFG_NAV5, UBX::encode(_buf.payload_tx_cfg_nav5))) {
        return -1;
    }

    if (waitForAck(UBX_MSG_CFG_NAV5, UBX_CONFIG_TIMEOUT, true) < 0) {
        return -1;
    }

    /* configure active GNSS systems (number of channels and used signals taken from U-Center default) */
    if (static_cast<int32_t>(gnssSystems) != 0) {
        memset(&_buf.payload_tx_cfg_gnss, 0, sizeof(_buf.payload_tx_cfg_gnss));
        _buf.payload_tx_cfg_gnss.msgVer = 0x00;
        _buf.payload_tx_cfg_gnss.numTrkChHw = 0x00;    // read only
        _buf.payload_tx_cfg_gnss.numTrkChUse = 0xFF;   // use max number of HW channels
        _buf.payload_tx_cfg_gnss.numConfigBlocks = 7;  // always configure all systems

        // GPS and QZSS should always be enabled and disabled together, according to uBlox
        _buf.payload_tx_cfg_gnss.block[0].gnssId = UBX_TX_CFG_GNSS_GNSSID_GPS;
        _buf.payload_tx_cfg_gnss.block[1].gnssId = UBX_TX_CFG_GNSS_GNSSID_QZSS;

        if (gnssSystems & GNSSSystemsMask::ENABLE_GPS) {
            _buf.payload_tx_cfg_gnss.block[0].resTrkCh = 8;
            _buf.payload_tx_cfg_gnss.block[0].maxTrkCh = 16;
            _buf.payload_tx_cfg_gnss.block[0].flags = UBX_TX_CFG_GNSS_FLAGS_GPS_L1CA | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
            _buf.payload_tx_cfg_gnss.block[1].resTrkCh = 0;
            _buf.payload_tx_cfg_gnss.block[1].maxTrkCh = 3;
            _buf.payload_tx_cfg_gnss.block[1].flags = UBX_TX_CFG_GNSS_FLAGS_QZSS_L1CA | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
        }

        _buf.payload_tx_cfg_gnss.block[2].gnssId = UBX_TX_CFG_GNSS_GNSSID_SBAS;

        if (gnssSystems & GNSSSystemsMask::ENABLE_SBAS) {
            _buf.payload_tx_cfg_gnss.block[2].resTrkCh = 1;
            _buf.payload_tx_cfg_gnss.block[2].maxTrkCh = 3;
            _buf.payload_tx_cfg_gnss.block[2].flags = UBX_TX_CFG_GNSS_FLAGS_SBAS_L1CA | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
        }

        _buf.payload_tx_cfg_gnss.block[3].gnssId = UBX_TX_CFG_GNSS_GNSSID_GALILEO;

        if (gnssSystems & GNSSSystemsMask::ENABLE_GALILEO) {
            _buf.payload_tx_cfg_gnss.block[3].resTrkCh = 4;
            _buf.payload_tx_cfg_gnss.block[3].maxTrkCh = 8;
            _buf.payload_tx_cfg_gnss.block[3].flags = UBX_TX_CFG_GNSS_FLAGS_GALILEO_E1 | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
        }

        _buf.payload_tx_cfg_gnss.block[4].gnssId = UBX_TX_CFG_GNSS_GNSSID_BEIDOU;

        if (gnssSystems & GNSSSystemsMask::ENABLE_BEIDOU) {
            _buf.payload_tx_cfg_gnss.block[4].resTrkCh = 8;
            _buf.payload_tx_cfg_gnss.block[4].maxTrkCh = 16;
            _buf.payload_tx_cfg_gnss.block[4].flags = UBX_TX_CFG_GNSS_FLAGS_BEIDOU_B1I | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
        }

        _buf.payload_tx_cfg_gnss.block[5].gnssId = UBX_TX_CFG_GNSS_GNSSID_GLONASS;

        if (gnssSystems & GNSSSystemsMask::ENABLE_GLONASS) {
            _buf.payload_tx_cfg_gnss.block[5].resTrkCh = 8;
            _buf.payload_tx_cfg_gnss.block[5].maxTrkCh = 14;
            _buf.payload_tx_cfg_gnss.block[5].flags = UBX_TX_CFG_GNSS_FLAGS_GLONASS_L1 | UBX_TX_CFG_GNSS_FLAGS_ENABLE;
        }

        // IMES always disabled
        _buf.payload_tx_cfg_gnss.block[6].gnssId = UBX_TX_CFG_GNSS_GNSSID_IMES;
        _buf.payload_tx_cfg_gnss.block[6].flags = 0;

        // send message
        if (!sendMessage(UBX_MSG_CFG_GNSS, UBX::encode(_buf.payload_tx_cfg_gnss))) {
            return -1;
        }

        if (waitForAck(UBX_MSG_CFG_GNSS, UBX_CONFIG_TIMEOUT, true) < 0) {
            // The receiver rejects the configuration as a whole if it names a constellation it
            // cannot receive, e.g. BeiDou on a SAM-M8Q, or more of them than it can track at
            // once. Keep the receiver's own selection rather than losing the fix over it.
            GPS_WARN("GNSS constellation config rejected, keeping receiver config");
        }

        // On u-blox 8 the Galileo change only takes effect once the configuration has been
        // saved and the receiver hardware reset, which we cannot do without dropping the
        // rest of this session's configuration
        if (gnssSystems & GNSSSystemsMask::ENABLE_GALILEO) {
            GPS_WARN("Galileo needs a receiver power cycle to take effect");
        }

        waitForGnssReset();
    }

    /* configure message rates */
    /* the last argument is divisor for measurement rate (set by CFG RATE), i.e. 1 means 5Hz */

    /* try to set rate for NAV-PVT */
    /* (implemented for ubx7+ modules only, use NAV-SOL, NAV-POSLLH, NAV-VELNED and NAV-TIMEUTC for ubx6) */
    if (!configureMessageRate(UBX_MSG_NAV_PVT, 1)) {
        return -1;
    }

    if (waitForAck(UBX_MSG_CFG_MSG, UBX_CONFIG_TIMEOUT, true) < 0) {
        _use_nav_pvt = false;

    } else {
        _use_nav_pvt = true;
    }

    if (!_use_nav_pvt) {
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

    if (!configureMessageRateAndAck(UBX_MSG_NAV_SVINFO, (_satellite_info != nullptr) ? 5 : 0, true)) {
        return -1;
    }

    if (!configureMessageRateAndAck(UBX_MSG_MON_HW, 1, true)) {
        return -1;
    }

    return 0;
}

int GPSDriverUBX::configureDevice(const GPSConfig& config)
{
    // There is no RTCM or USB interface on M10
    if (UBX::receiverProfile(_board).usb) {
        initCfgValset();

        const uint8_t enable_corrections_in = (_output_mode == OutputMode::RTCM) ? 0 : 1;

        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1INPROT_RTCM3X, enable_corrections_in);

        // USB
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_UBX, 1);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_RTCM3X, enable_corrections_in);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_NMEA, 0);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_UBX, 1);

        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_NMEA, 0);

        // Only RTCM-output-capable receivers expose these keys. M9 SPG rejects
        // the entire VALSET if they are included, even with a value of zero.
        if (UBX::receiverProfile(_board).rtcmOutput) {
            cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X, _output_mode != OutputMode::GPS);
            cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X, _output_mode != OutputMode::GPS);
        }

        if (sendCfgValsetAcked() < 0) {
            return -1;
        }

        // Optional SPARTN input (PointPerfect). Sent separately so modules without
        // SPARTN support can NACK without failing the rest of configuration.
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_UART1INPROT_SPARTN, enable_corrections_in);
        cfgValset<uint8_t>(UBX_CFG_KEY_CFG_USBINPROT_SPARTN, enable_corrections_in);

        sendCfgValsetAcked(false);
    }

    /* set configuration parameters */
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_FIXMODE, 3 /* Auto 2d/3d */);
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_UTCSTANDARD, 3 /* USNO (U.S. Naval Observatory derived from GPS) */);
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVSPG_DYNMODEL, _dyn_model);

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

    if (_output_rate > 0) {
        if (_output_rate > 25) {
            GPS_WARN("Rate %u Hz exceeds max, limiting to 25Hz", _output_rate);
            _output_rate = 25;
        }

        // convert hz to ms
        rate_meas = 1000 / _output_rate;

    } else {
        switch (_board) {
            case Board::u_blox9:
                rate_meas = 125;  // 8Hz
                break;

            case Board::u_blox9_F9P_L1L2:
                rate_meas = 200;  // 5Hz
                break;

            case Board::u_blox9_F9P_L1L5:
                rate_meas = 200;  // 5Hz
                break;

            default:
                break;
        }
    }

    cfgValset<uint16_t>(UBX_CFG_KEY_RATE_MEAS, rate_meas);
    cfgValset<uint16_t>(UBX_CFG_KEY_RATE_NAV, 1);
    cfgValset<uint8_t>(UBX_CFG_KEY_RATE_TIMEREF, 0);

    if (sendCfgValsetAcked() < 0) {
        return -1;
    }

    // Disable odometer. Separate, non-fatal VALSET: CFG-ODO-* was removed on the
    // F20 platform (ZED-X20P HPG 2.10+), so a NAK here must not abort config.
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_ODO_USE_ODO, 0);

    // M9 (SPG) only has USE_ODO and PROFILE in CFG-ODO
    if (_board != Board::u_blox9) {
        static constexpr uint32_t odo_keys[] = {UBX_CFG_KEY_ODO_USE_COG, UBX_CFG_KEY_ODO_OUTLPVEL,
                                                UBX_CFG_KEY_ODO_OUTLPCOG};
        cfgValset(odo_keys, 0);
    }

    sendCfgValsetAcked(false);

    // RTK (optional, as only RTK devices like F9P support it)
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_NAVHPG_DGNSSMODE, 3 /* RTK Fixed */);

    if (!sendCfgValset()) {
        return -1;
    }

    waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false);

    // Jamming detection. Firmware with CFG-SEC-JAMDET (F9 HPG 1.50+, F9 L1L5, F20/X20) has
    // detection always on and no CFG-ITFM; everything older has CFG-ITFM and no JAMDET key.
    // A NAK on the sensitivity key is therefore the signal that the monitor still needs enabling.
    initCfgValset();
    cfgValset<uint8_t>(UBX_CFG_KEY_SEC_JAMDET_SENSITIVITY_HI, 0);

    if (sendCfgValsetAcked(false) < 0) {
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_ITFM_ENABLE, 1);

        if (sendCfgValsetAcked(false) < 0) {
            GPS_WARN("Jamming monitor not supported by this receiver");
        }
    }

    // configure active GNSS systems (leave signal bands as is)
    // Note: For M10 configuration if changing from default. As per the
    //       MAX-M10S integration guide UBX-20053088 - R03, see section
    //       2.1.1.3 GNSS signal configuration for details on some restrictions.
    //       Implementing these restrictions are a TODO item for M10.
    if (static_cast<int32_t>(config.gnss_systems) != 0) {
        initCfgValset();

        // GPS and QZSS should always be enabled and disabled together, according to uBlox
        if (config.gnss_systems & GNSSSystemsMask::ENABLE_GPS) {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_ENA, 1);
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_ENA, 1);

            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L1CA_ENA, 1);

            // M9 (SPG) has no CFG-SIGNAL key for QZSS L1S
            if (_board != Board::u_blox9) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L1S_ENA, 1);
            }

            if (_board == Board::u_blox_X20) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L2C_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L5_ENA, 1);

            } else if (_board == Board::u_blox9_F9P_L1L2) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 1);

                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L2C_ENA, 1);

            } else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 1);

                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_L5_ENA, 1);
            }

        } else {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_ENA, 0);
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_ENA, 0);

            if (_board == Board::u_blox_X20) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 0);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 0);

            } else if (_board == Board::u_blox9_F9P_L1L2) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L2C_ENA, 0);

            } else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_L5_ENA, 0);
            }
        }

        if (config.gnss_systems & GNSSSystemsMask::ENABLE_GALILEO) {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_ENA, 1);

            if (_board == Board::u_blox_X20) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E6_ENA, 1);

            } else if (_board == Board::u_blox9_F9P_L1L2) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5B_ENA, 1);

            } else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 1);
            }

        } else {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_ENA, 0);

            if (_board == Board::u_blox_X20) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 0);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E6_ENA, 0);

            } else if (_board == Board::u_blox9_F9P_L1L2) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5B_ENA, 0);

            } else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_E5A_ENA, 0);
            }
        }

        if (config.gnss_systems & GNSSSystemsMask::ENABLE_BEIDOU) {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_ENA, 1);

            if (_board == Board::u_blox_X20) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B1C_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B3_ENA, 1);

            } else if (_board == Board::u_blox9_F9P_L1L2) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2_ENA, 1);

            } else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 1);
            }

        } else {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_ENA, 0);

            if (_board == Board::u_blox_X20) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B1C_ENA, 0);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 0);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B3_ENA, 0);

            } else if (_board == Board::u_blox9_F9P_L1L2) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2_ENA, 0);

            } else if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_B2A_ENA, 0);
            }
        }

        // GLONASS is not supported on DAN-F10N and X20
        if (_board != Board::u_blox10_L1L5 && _board != Board::u_blox_X20) {
            if (config.gnss_systems & GNSSSystemsMask::ENABLE_GLONASS) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L1_ENA, 1);

                if (_board == Board::u_blox9_F9P_L1L2) {
                    cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L2_ENA, 1);
                }

            } else {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_ENA, 0);
                // cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L1_ENA, 0);

                if (_board == Board::u_blox9_F9P_L1L2) {
                    cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_L2_ENA, 0);
                }
            }
        }

        if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5 || _board == Board::u_blox_X20) {
            if (config.gnss_systems & GNSSSystemsMask::ENABLE_NAVIC) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_ENA, 1);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_L5_ENA, 1);

            } else {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_ENA, 0);
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_NAVIC_L5_ENA, 0);
            }
        }

        if (!sendCfgValset()) {
            return -1;
        }

        if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, true) < 0) {
            // The receiver NAKs the whole message and applies nothing if it does not know a
            // single key, so a signal key missing on this generation would leave the receiver
            // unconfigured. Retry with the constellation enables, those exist everywhere.
            GPS_WARN("GNSS signal config rejected, retrying without signal bands");

            initCfgValset();

            const uint8_t use_gps = (config.gnss_systems & GNSSSystemsMask::ENABLE_GPS) ? 1 : 0;
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GPS_ENA, use_gps);
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_QZSS_ENA, use_gps);
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GAL_ENA,
                               (config.gnss_systems & GNSSSystemsMask::ENABLE_GALILEO) ? 1 : 0);
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_BDS_ENA,
                               (config.gnss_systems & GNSSSystemsMask::ENABLE_BEIDOU) ? 1 : 0);

            if (_board != Board::u_blox10_L1L5 && _board != Board::u_blox_X20) {
                cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_GLO_ENA,
                                   (config.gnss_systems & GNSSSystemsMask::ENABLE_GLONASS) ? 1 : 0);
            }

            if (!sendCfgValset()) {
                return -1;
            }

            if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, true) < 0) {
                if (config.require_gnss_config) {
                    _constellation_configuration_rejected = true;
                    _constellation_request_rejected = _last_ack_rejected;
                    return -1;
                }
                // Keep going with whatever the receiver already has, a refused constellation
                // selection must not cost us the fix
                GPS_WARN("GNSS constellation config rejected, keeping receiver config");
            }
        }

        waitForGnssReset();

        // send SBAS config separately, because it seems to be buggy (with u-center, too)
        initCfgValset();

        if (config.gnss_systems & GNSSSystemsMask::ENABLE_SBAS) {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_SBAS_ENA, 1);
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_SBAS_L1CA_ENA, 1);

        } else {
            cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_SBAS_ENA, 0);
        }

        if (!sendCfgValset()) {
            return -1;
        }

        if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, true) < 0 && config.require_gnss_config) {
            _constellation_configuration_rejected = true;
            _constellation_request_rejected = _last_ack_rejected;
            return -1;
        }

        waitForGnssReset();
    }

    // GPS L5 is broadcast unhealthy while it is pre-operational, so tell the receiver to take
    // the L1 health flag instead. The key is not in any interface description, only in app note
    // UBX-21038688, so it gets a message of its own rather than putting the constellation
    // config at the mercy of a firmware that has never heard of it.
    if (_board == Board::u_blox9_F9P_L1L5 || _board == Board::u_blox10_L1L5 || _board == Board::u_blox_X20) {
        const bool use_gps =
            (static_cast<int32_t>(config.gnss_systems) == 0) || (config.gnss_systems & GNSSSystemsMask::ENABLE_GPS);

        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_SIGNAL_L5_HEALTH_OVERRIDE, use_gps ? 1 : 0);

        if (sendCfgValsetAcked(false) < 0) {
            GPS_WARN("GPS L5 health override not supported by this receiver");
        }
    }

    // Configure message rates
    // Send a new CFG-VALSET message to make sure it does not get too large
    initCfgValset();
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_PVT_I2C, 1);

    // There is no RTCM on M10 and M9* (except F9P)
    if (_board != Board::u_blox10 && _board != Board::u_blox9 && _board != Board::u_blox10_L1L5) {
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_RELPOSNED_I2C, 0);
    }

    _use_nav_pvt = true;
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_DOP_I2C, 1);
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SAT_I2C, (_satellite_info != nullptr) ? 10 : 0);
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_STATUS_I2C, 1);
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_MON_RF_I2C, 1);
    _got_sec_sig = false;

    if (sendCfgValsetAcked() < 0) {
        return -1;
    }

    // Optional on older firmware. A rejected EOE key leaves the bounded epoch deadline in use.
    initCfgValset();
    cfgValsetPort(UBX::NAV_EOE_MSGOUT_I2C, 1);
    (void) sendCfgValsetAcked(false);
    if (ioError())
        return -1;

    // Correction input status. RXM-COR reports every protocol (RTCM3, SPARTN, HAS) and is
    // the only form on the X20, which has no RXM-RTCM; receivers without it get RXM-RTCM.
    initCfgValset();
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_COR_I2C, 1);

    if (sendCfgValsetAcked(false) < 0 &&
        ((_board == Board::u_blox9) || (_board == Board::u_blox9_F9P_L1L2) || (_board == Board::u_blox9_F9P_L1L5))) {
        initCfgValset();
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_RTCM_I2C, 1);
        sendCfgValsetAcked(false);
    }

    // UBX-SEC-SIG carries jammingState. MON-RF jammingState is always 0 on
    // firmware that supports this message (F9 HPG 1.51, X20). The rate key is
    // absent on older protocol-27 receivers; a NAK must not abort config.
    initCfgValset();
    cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_SEC_SIG_I2C, 1);

    if (sendCfgValsetAcked(false) < 0) {
    }

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
    if (UBX::receiverProfile(_board).usb) {
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_RXM_RAWX_I2C, 0);
    }

    if (sendCfgValsetAcked(false) < 0) {
        GPS_WARN("Could not disable unused messages");
    }

    // Dual antenna heading, not used in a moving base setup where NAV-RELPOSNED provides it. The rate is
    // always written so a mode change is idempotent; a NAK from a position-only X20P must not abort config.
    // The receiver side heading offset is zeroed, leaving GPS_YAW_OFFSET as the only offset applied.
    if (_board == Board::u_blox_X20) {
        initCfgValset();
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_DAHEADING_I2C, 1);
        cfgValset<int32_t>(UBX_CFG_KEY_NAVSPG_DAHEADING_OFFSET, 0);

        if (sendCfgValset()) {
            if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false) < 0) {
            }
        }

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

void GPSDriverUBX::initCfgValset()
{
    _valsetSettings = 0;
    static_assert(sizeof(_tx_cfg_valset_buf) >= sizeof(ubx_payload_tx_cfg_valset_t),
                  "_tx_cfg_valset_buf must hold at least the CFG-VALSET header");
    auto* header = reinterpret_cast<ubx_payload_tx_cfg_valset_t*>(_tx_cfg_valset_buf);
    memset(_tx_cfg_valset_buf, 0, sizeof(_tx_cfg_valset_buf));
    header->layers = UBX_CFG_LAYER_RAM;
    _tx_cfg_valset_size = sizeof(*header) - sizeof(header->cfgData);
}

bool GPSDriverUBX::sendCfgValset()
{
    return sendMessage(UBX_MSG_CFG_VALSET, _tx_cfg_valset_buf, _tx_cfg_valset_size);
}

int GPSDriverUBX::sendCfgValsetAcked(bool report_ack_error)
{
    if (!sendCfgValset()) {
        return -1;
    }

    return waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, report_ack_error);
}

bool GPSDriverUBX::cfgValsetRaw(uint32_t key_id, uint32_t value)
{
    if (key_id == UBX_CFG_KEY_NAVSPG_DYNMODEL)
        _valsetSettings |= 1;
    if (key_id == UBX_CFG_KEY_RATE_MEAS || key_id == UBX_CFG_KEY_RATE_NAV)
        _valsetSettings |= 2;
    if ((key_id & 0xffff0000u) == 0x10310000u)
        _valsetSettings |= 4;

    const unsigned value_size = UBX::configurationValueBytes(key_id);
    if (!value_size || (value_size < 4 && value >= (1u << (value_size * 8))) || ((key_id >> 28) == 1 && value > 1))
        return false;
    if ((key_id == UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X || key_id == UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X) &&
        !UBX::receiverProfile(_board).rtcmOutput)
        return false;

    if (_tx_cfg_valset_size + sizeof(key_id) + value_size > sizeof(_tx_cfg_valset_buf)) {
        // If this ever fires, either bump UBX_CFG_VALSET_BUF_SIZE or split the
        // batch into multiple CFG-VALSET messages at the call site.
        GPS_WARN("buf for CFG_VALSET too small");
        return false;
    }

    const std::span<uint8_t> output(_tx_cfg_valset_buf);
    if (!GPSWire::write(output, _tx_cfg_valset_size, key_id))
        return false;
    const auto valueOffset = _tx_cfg_valset_size + sizeof(key_id);
    const bool written = value_size == 1   ? GPSWire::write(output, valueOffset, static_cast<uint8_t>(value))
                         : value_size == 2 ? GPSWire::write(output, valueOffset, static_cast<uint16_t>(value))
                                           : GPSWire::write(output, valueOffset, value);
    if (!written)
        return false;
    _tx_cfg_valset_size = valueOffset + value_size;
    return true;
}

bool GPSDriverUBX::cfgValsetPort(uint32_t key_id, uint8_t value)
{
    for (const auto port : UBX::OUTPUT_PORTS) {
        if ((!port.requiresUsb || UBX::receiverProfile(_board).usb) &&
            !cfgValset<uint8_t>(key_id + port.messageKeyOffset, value))
            return false;
    }

    return true;
}

bool GPSDriverUBX::cfgValsetItems(const CfgValsetItem* items, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (!cfgValsetRaw(items[i].key, items[i].value)) {
            return false;
        }
    }

    return true;
}

bool GPSDriverUBX::cfgValsetKeys(const uint32_t* keys, size_t count, uint8_t value)
{
    for (size_t i = 0; i < count; i++) {
        if (!cfgValsetRaw(keys[i], value)) {
            return false;
        }
    }

    return true;
}

bool GPSDriverUBX::cfgValsetPortKeys(const uint32_t* keys, size_t count, uint8_t value)
{
    for (size_t i = 0; i < count; i++) {
        if (!cfgValsetPort(keys[i], value)) {
            return false;
        }
    }

    return true;
}

int GPSDriverUBX::disableTimeMode()
{
    // GPS output alone does not release a receiver previously configured as a fixed base.
    if (_proto_ver_27_or_higher) {
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 0);
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 0);

        if (sendCfgValsetAcked() < 0) {
            return -1;
        }

    } else {
        if (!configureMessageRateAndAck(UBX_MSG_NAV_SVIN, 0, true)) {
            return -1;
        }

        memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));

        if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(_buf.payload_tx_cfg_tmode3)) ||
            waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
            return -1;
        }
    }

    _survey_in_stopped = false;
    const gps_abstime stop_deadline = nowUs() + 3000000;

    while (!_survey_in_stopped && nowUs() < stop_deadline) {
        if (!sendMessage(UBX_MSG_NAV_SVIN, nullptr, 0)) {
            return -1;
        }

        const gps_abstime poll_deadline = nowUs() + 100000;

        while (!_survey_in_stopped && nowUs() < poll_deadline) {
            bool read_error;
            receiveInternal(100, read_error);

            if (read_error) {
                return -1;
            }
        }
    }

    if (!_survey_in_stopped) {
        GPS_WARN("Time mode did not stop");
        return -1;
    }

    return 0;
}

int GPSDriverUBX::restartSurveyInPreV27()
{
    // disable RTCM (MSM7) output
    configureMessageRate(UBX_MSG_RTCM3_1005, 0);
    configureMessageRate(UBX_MSG_RTCM3_1077, 0);
    configureMessageRate(UBX_MSG_RTCM3_1087, 0);
    configureMessageRate(UBX_MSG_RTCM3_1230, 0);
    configureMessageRate(UBX_MSG_RTCM3_1097, 0);
    configureMessageRate(UBX_MSG_RTCM3_1127, 0);

    // stop it first
    // FIXME: stopping the survey-in process does not seem to work
    memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
    _buf.payload_tx_cfg_tmode3.flags = 0; /* disable time mode */

    if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(_buf.payload_tx_cfg_tmode3))) {
        GPS_WARN("TMODE3 failed. Device w/o base station support?");
        return -1;
    }

    if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
        return -1;
    }

    if (!_baseConfig.useFixedBase) {
        memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
        _buf.payload_tx_cfg_tmode3.flags = 1; /* start survey-in */
        _buf.payload_tx_cfg_tmode3.svinMinDur = _baseConfig.surveyInDurationSecs;
        _buf.payload_tx_cfg_tmode3.svinAccLimit = static_cast<uint32_t>(_baseConfig.surveyInAccMeters * 10000.0);

        if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(_buf.payload_tx_cfg_tmode3))) {
            return -1;
        }

        if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
            return -1;
        }

        /* enable status output of survey-in */
        if (!configureMessageRateAndAck(UBX_MSG_NAV_SVIN, 5, true)) {
            return -1;
        }

    } else {
        const GPSBaseStationConfig& settings = _baseConfig;

        memset(&_buf.payload_tx_cfg_tmode3, 0, sizeof(_buf.payload_tx_cfg_tmode3));
        _buf.payload_tx_cfg_tmode3.flags = 2 /* fixed mode */ | (1 << 8) /* lat/lon mode */;
        int64_t lat64 = (int64_t) (settings.fixedBaseLatitude * 1e9);
        _buf.payload_tx_cfg_tmode3.ecefXOrLat = (int32_t) (lat64 / 100);
        _buf.payload_tx_cfg_tmode3.ecefXOrLatHP = lat64 % 100;  // range [-99, 99]
        int64_t lon64 = (int64_t) (settings.fixedBaseLongitude * 1e9);
        _buf.payload_tx_cfg_tmode3.ecefYOrLon = (int32_t) (lon64 / 100);
        _buf.payload_tx_cfg_tmode3.ecefYOrLonHP = lon64 % 100;
        int64_t alt64 = (int64_t) ((double) settings.fixedBaseAltitudeMeters * 1e4);
        _buf.payload_tx_cfg_tmode3.ecefZOrAlt = (int32_t) (alt64 / 100);  // cm
        _buf.payload_tx_cfg_tmode3.ecefZOrAltHP = alt64 % 100;            // 0.1mm

        _buf.payload_tx_cfg_tmode3.fixedPosAcc = (uint32_t) (settings.fixedBaseAccuracyMeters * 10000.f);

        if (!sendMessage(UBX_MSG_CFG_TMODE3, UBX::encode(_buf.payload_tx_cfg_tmode3))) {
            return -1;
        }

        if (waitForAck(UBX_MSG_CFG_TMODE3, UBX_CONFIG_TIMEOUT, true) < 0) {
            return -1;
        }

        // directly enable RTCM3 output
        return activateRTCMOutput();
    }

    return 0;
}

int GPSDriverUBX::restartSurveyIn()
{
    if (_output_mode != OutputMode::RTCM) {
        return -1;
    }

    if (!_proto_ver_27_or_higher) {
        return restartSurveyInPreV27();
    }

    // disable RTCM output
    initCfgValset();
    cfgValsetPort(RTCM_BASE_MSGOUT_I2C, 0);
    sendCfgValsetAcked(false);

    if (!_baseConfig.useFixedBase) {
        // Reapplying survey-in mode does not restart an existing survey.
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 0 /* Disabled */);

        if (sendCfgValsetAcked() < 0) {
            return -1;
        }

        // Time-mode changes take effect on a navigation epoch, not on the ACK.
        _survey_in_stopped = false;
        const gps_abstime stop_deadline = nowUs() + 3000000;

        while (!_survey_in_stopped && nowUs() < stop_deadline) {
            if (!sendMessage(UBX_MSG_NAV_SVIN, nullptr, 0)) {
                return -1;
            }

            const gps_abstime poll_deadline = nowUs() + 100000;

            while (!_survey_in_stopped && nowUs() < poll_deadline) {
                bool read_error;
                receiveInternal(100, read_error);

                if (read_error) {
                    return -1;
                }
            }
        }

        if (!_survey_in_stopped) {
            GPS_WARN("Survey-in did not stop");
            return -1;
        }

        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 1 /* Survey-in */);
        cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_SVIN_MIN_DUR, _baseConfig.surveyInDurationSecs);
        cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_SVIN_ACC_LIMIT,
                            static_cast<uint32_t>(_baseConfig.surveyInAccMeters * 10000.0));
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 5);

        if (sendCfgValsetAcked() < 0) {
            return -1;
        }

    } else {
        const GPSBaseStationConfig& settings = _baseConfig;
        initCfgValset();
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_MODE, 2 /* Fixed Mode */);
        cfgValset<uint8_t>(UBX_CFG_KEY_TMODE_POS_TYPE, 1 /* Lat/Lon/Height */);
        int64_t lat64 = (int64_t) (settings.fixedBaseLatitude * 1e9);
        cfgValset<int32_t>(UBX_CFG_KEY_TMODE_LAT, (int32_t) (lat64 / 100));
        cfgValset<int8_t>(UBX_CFG_KEY_TMODE_LAT_HP, lat64 % 100 /* range [-99, 99] */);
        int64_t lon64 = (int64_t) (settings.fixedBaseLongitude * 1e9);
        cfgValset<int32_t>(UBX_CFG_KEY_TMODE_LON, (int32_t) (lon64 / 100));
        cfgValset<int8_t>(UBX_CFG_KEY_TMODE_LON_HP, lon64 % 100 /* range [-99, 99] */);
        int64_t alt64 = (int64_t) ((double) settings.fixedBaseAltitudeMeters * 1e4);
        cfgValset<int32_t>(UBX_CFG_KEY_TMODE_HEIGHT, (int32_t) (alt64 / 100) /* cm */);
        cfgValset<int8_t>(UBX_CFG_KEY_TMODE_HEIGHT_HP, alt64 % 100 /* 0.1mm */);
        cfgValset<uint32_t>(UBX_CFG_KEY_TMODE_FIXED_POS_ACC, (uint32_t) (settings.fixedBaseAccuracyMeters * 10000.f));

        if (sendCfgValsetAcked() < 0) {
            return -1;
        }

        // directly enable RTCM3 output
        return activateRTCMOutput();
    }

    return 0;
}

int  // -1 = NAK, error or timeout, 0 = ACK
GPSDriverUBX::waitForAck(const uint16_t msg, const unsigned timeout, const bool report)
{
    const Operation operation(*this, timeout);
    _last_ack_rejected = false;
    _ack_state = UBX_ACK_WAITING;
    _ack_waiting_msg = msg;
    const auto result = awaitCommand(
        std::to_string(msg), timeout,
        [this, timeout] {
            bool error;
            receiveInternal(timeout, error);
        },
        [this] {
            return _ack_state == UBX_ACK_GOT_ACK   ? GPSCommandOutcome::Acknowledged
                   : _ack_state == UBX_ACK_GOT_NAK ? GPSCommandOutcome::Rejected
                                                   : GPSCommandOutcome::Pending;
        },
        report, _pendingCommandSettings);
    for (unsigned bit = 0; bit < _settingOutcomes.size(); ++bit) {
        if (_pendingCommandSettings & (1u << bit))
            _settingOutcomes[bit] = result.outcome;
    }
    _pendingCommandSettings = 0;
    _last_ack_rejected = result.outcome == GPSCommandOutcome::Rejected;
    _ack_state = UBX_ACK_IDLE;
    return result.outcome == GPSCommandOutcome::Acknowledged ? 0 : -1;
}

void GPSDriverUBX::waitForGnssReset()
{
    // Changing the enabled constellations resets the GNSS subsystem, and every u-blox
    // interface description asks for 0.5 s after the acknowledgement before the next
    // command. Keep reading while we wait, the receiver is still streaming.
    const gps_abstime time_started = nowUs();

    while (nowUs() < time_started + UBX_GNSS_RESET_TIME) {
        receive(UBX_CONFIG_TIMEOUT);
        if (ioError()) {
            return;
        }
    }
}

void GPSDriverUBX::requestCommsDiagnostics()
{
    const gps_abstime now = nowUs();

    if (now < _next_comms_poll) {
        return;
    }

    // A congested receiver must not be flooded with diagnostic requests.
    _next_comms_poll = now + 5000000;
    _comms_poll_deadline = sendMessage(UBX_MSG_MON_COMMS, nullptr, 0) ? now + 2000000 : 0;
}

int GPSDriverUBX::activateRTCMOutput()
{
    /* For base stations we switch to 1 Hz update rate, which is enough for RTCM output.
     * For the survey-in, we still want 5/10 Hz, because this speeds up the process */

    if (_proto_ver_27_or_higher) {
        initCfgValset();

        cfgValset<uint16_t>(UBX_CFG_KEY_RATE_MEAS, 1000);

        cfgValsetPort(RTCM_BASE_MSGOUT_I2C, 1);
        cfgValsetPort(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C, 0);

        if (!sendCfgValset()) {
            return -1;
        }

        if (waitForAck(UBX_MSG_CFG_VALSET, UBX_CONFIG_TIMEOUT, false) < 0) {
            return -1;
        }

    } else {
        memset(&_buf.payload_tx_cfg_rate, 0, sizeof(_buf.payload_tx_cfg_rate));
        _buf.payload_tx_cfg_rate.measRate = 1000;
        _buf.payload_tx_cfg_rate.navRate = UBX_TX_CFG_RATE_NAVRATE;
        _buf.payload_tx_cfg_rate.timeRef = UBX_TX_CFG_RATE_TIMEREF;

        if (!sendMessage(UBX_MSG_CFG_RATE, UBX::encode(_buf.payload_tx_cfg_rate))) {
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

bool GPSDriverUBX::configureMessageRate(const uint16_t msg, const uint8_t rate)
{
    if (_proto_ver_27_or_higher) {
        // configureMessageRate() should not be called if _proto_ver_27_or_higher is true.
        // If you see this message the calling code needs to be fixed.
        GPS_WARN("FIXME: use of deprecated msg CFG_MSG (%i %i)", msg, rate);
    }

    ubx_payload_tx_cfg_msg_t cfg_msg;  // don't use _buf (allow interleaved operation)
    memset(&cfg_msg, 0, sizeof(cfg_msg));

    cfg_msg.msg = msg;
    cfg_msg.rate = rate;

    return sendMessage(UBX_MSG_CFG_MSG, UBX::encode(cfg_msg));
}

bool GPSDriverUBX::configureMessageRateAndAck(uint16_t msg, uint8_t rate, bool report_ack_error)
{
    if (!configureMessageRate(msg, rate)) {
        return false;
    }

    return waitForAck(UBX_MSG_CFG_MSG, UBX_CONFIG_TIMEOUT, report_ack_error) >= 0;
}

bool GPSDriverUBX::sendMessage(const uint16_t msg, const uint8_t* payload, const uint16_t length)
{
    const Operation operation(*this, UBX_CONFIG_TIMEOUT);
    beginCommandWrite();
    _pendingCommandSettings = msg == UBX_MSG_CFG_NAV5     ? 1u
                              : msg == UBX_MSG_CFG_RATE   ? 2u
                              : msg == UBX_MSG_CFG_GNSS   ? 4u
                              : msg == UBX_MSG_CFG_VALSET ? _valsetSettings
                                                          : 0u;
    std::array<uint8_t, 6> header{UBX_SYNC1, UBX_SYNC2};
    GPSWire::write(header, 2, msg);
    GPSWire::write(header, 4, length);
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

    if (write((void*) &checksum, sizeof(checksum)) != sizeof(checksum)) {
        return false;
    }

    return true;
}
