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

/**
 * @file ubx.h
 *
 * U-Blox protocol definition. Following u-blox 6/7/8 Receiver Description
 * including Prototol Specification.
 *
 * @author Thomas Gubler <thomasgubler@student.ethz.ch>
 * @author Julian Oes <julian@oes.ch>
 * @author Anton Babushkin <anton.babushkin@me.com>
 * @author Beat Küng <beat-kueng@gmx.net>
 *
 * @author Hannes Delago
 *   (rework, add ubx7+ compatibility)
 *
 */

#pragma once

#include "GPSBaseProtocol.h"
#include "UBXFrameDecoder.h"
#include "UBXMessages.h"
#include "UBXNavigationEpoch.h"
#include "UBXReceiverController.h"
#include "UBXReceiverProfile.h"

class GPSDriverUBX : public GPSBaseProtocol
{
public:
    GPSDriverUBX(GPSProtocolIO io, GPSPositionReport* gps_position, GPSSatelliteReport* satellite_info);

    virtual ~GPSDriverUBX();

    enum class OutputProtocol : uint8_t
    {
        Native = 0,
        NMEA
    };

    int configure(unsigned& baudrate, const GPSConfig& config) override;
    /** Configure navigation and its output protocol together. NMEA requires normal GPS on UART/USB. */
    int configure(unsigned& baudrate, const GPSConfig& config, OutputProtocol output_protocol);

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    const GPSPositionReport* positionReport() const override { return _gps_position; }

    const GPSSatelliteReport* satelliteReport() const override { return _satellite_info; }

    bool receiverReady() const override { return _configured; }

    using Board = UBX::Board;

    const Board& board() const { return _board; }

    const char* modelName() const { return _model_name; }

    const char* firmwareVersion() const { return _firmware_version; }
    enum class BaseStationCapability : uint8_t
    {
        Unknown,
        Unsupported,
        Supported,
    };
    BaseStationCapability baseStationCapability() const;
    bool supportsConstellationSelection() const;
    bool supportsOutputRateSelection() const;

    struct DecodeContext
    {
        bool navigation = false;
        bool useNavPvt = true;
        bool corrections = false;
        bool assembleEpochs = false;
    };

    void setDecodeContext(DecodeContext context);

    bool constellationConfigurationRejected() const { return _constellation_configuration_rejected; }

    bool constellationRequestRejected() const { return _constellation_request_rejected; }

    struct ConfigurationReadback
    {
        uint8_t dynamic_model = 0;
        uint16_t measurement_interval_ms = 0;
        uint16_t navigation_rate = 0;
        uint32_t constellation_mask = 0;
        bool constellations_reported = false;
    };

    bool readConfiguration(ConfigurationReadback& report, unsigned timeout_ms);

    /**
     * What UART1 carries in a given mode, for status output
     */

private:
    void servicePendingCommands() override;
    bool _rtcmActivationPending = false;
    bool _comms_request_pending = false;
    uint16_t _pendingDisableMessage = 0;
    GPSReceiverSettingSet _valsetSettings;
    GPSReceiverSettingSet _pendingCommandSettings;
    UBX::ReceiverController _controller;
    int enableNmeaOutput(unsigned baudrate);

    /** Like receive(), but reports a negative device read separately from a timeout. */
    int receiveInternal(unsigned timeout, bool& read_error);

    void requestCommsDiagnostics();
    void logCommsDiagnostics();

    int activateRTCMOutput();

    /**
     * Convert a relative position heading into a vehicle heading.
     * @param heading baseline heading as reported by the receiver [1e-5 deg]
     * @return heading normalized to [-pi, pi]
     */
    float relPosHeadingToYaw(int32_t heading) const;

    /**
     * While parsing add every byte (except the sync bytes) to the checksum
     */

    /**
     * Calculate & add checksum for given buffer
     */
    void calcChecksum(const uint8_t* buffer, const uint16_t length, ubx_checksum_t* checksum);

    /**
     * Configure message rate.
     * Note: this is deprecated with protocol version >= 27
     * @return true on success, false on write error
     */
    bool configureMessageRate(const uint16_t msg, const uint8_t rate);

    /**
     * Combines the configure_message_rate & wait_for_ack calls.
     * Note: this is deprecated with protocol version >= 27
     * @return true on success
     */
    inline bool configureMessageRateAndAck(uint16_t msg, uint8_t rate, bool report_ack_error = false);

    /**
     * Send configuration values and desired message rates
     * @param config The configuration includes GNSS systems to use and protocol for interfaces
     * @param uart2_baudrate Baudrate of F9P's UART2 port
     * @return 0 on success, <0 on error
     */
    int configureDevice(const GPSConfig& config);
    /**
     * Send configuration values and desired message rates (for protocol version < 27)
     * @param gnssSystems Set of GNSS systems to use
     * @return 0 on success, <0 on error
     */
    int configureDevicePreV27(const GNSSSystemsMask& gnssSystems);

    /**
     * Add a configuration value to the pending CFG-VALSET in _tx_cfg_valset_buf.
     * The value width on the wire comes from the key ID's size field, not from T; T documents the
     * call site and must agree with the key.
     * @param key_id one of the UBX_CFG_KEY_* constants
     * @param value configuration value
     * @return true on success, false if buffer too small
     */
    template <typename T>
    bool cfgValset(uint32_t key_id, T value)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "CFG-VALSET values wider than 4 bytes are not supported");
        return cfgValsetRaw(key_id, static_cast<uint32_t>(value));
    }

    /**
     * Non-template body for cfgValset(): appends key_id and the low N bytes of value, where N is
     * given by the key ID's size field (bits 28-30: 1 and 2 -> 1 byte, 3 -> 2 bytes, 4 -> 4 bytes).
     */
    bool cfgValsetRaw(uint32_t key_id, uint32_t value);

    /** A fixed key/value pair for cfgValset(items) */
    struct CfgValsetItem
    {
        uint32_t key;
        uint8_t value;
    } __attribute__((packed));

    /**
     * Add a fixed list of 1-byte configuration values
     * @return true on success, false if buffer too small
     */
    template <size_t N>
    bool cfgValset(const CfgValsetItem (&items)[N])
    {
        return cfgValsetItems(items, N);
    }

    bool cfgValsetItems(const CfgValsetItem* items, size_t count);

    /**
     * Add the same 1-byte value for a list of keys
     * @return true on success, false if buffer too small
     */
    template <size_t N>
    bool cfgValset(const uint32_t (&keys)[N], uint8_t value)
    {
        return cfgValsetKeys(keys, N, value);
    }

    bool cfgValsetKeys(const uint32_t* keys, size_t count, uint8_t value);

    /**
     * Add a configuration value that is port-specific (MSGOUT messages).
     * Note: Key ID must be the one for I2C, and the implementation assumes the
     *       Key ID's are in increasing order for the other ports: I2C, UART1, UART2, USB, SPI
     *       (this is a safe assumption for all MSGOUT messages according to u-blox).
     *
     * @param key_id I2C key ID
     * @param value configuration value
     * @return true on success, false if buffer too small
     */
    bool cfgValsetPort(uint32_t key_id, uint8_t value);

    /** cfgValsetPort() for a list of I2C key IDs sharing one value */
    template <size_t N>
    bool cfgValsetPort(const uint32_t (&keys)[N], uint8_t value)
    {
        return cfgValsetPortKeys(keys, N, value);
    }

    bool cfgValsetPortKeys(const uint32_t* keys, size_t count, uint8_t value);

    /**
     * Reset the parse state machine for a fresh start
     */
    void decodeInit(void);

    /**
     * Start a new CFG-VALSET in _tx_cfg_valset_buf (header only, no config values yet)
     */
    void initCfgValset();

    /**
     * Send the CFG-VALSET built up by initCfgValset() and cfgValset*() calls
     * @return true on success
     */
    bool sendCfgValset();

    /**
     * sendCfgValset() followed by waitForAck()
     * @param report_ack_error log a NAK or timeout
     * @return 0 on ACK, <0 if sending failed or no ACK was received
     */
    int sendCfgValsetAcked(bool report_ack_error = true);

    /**
     * Start or restart the survey-in procees. This is only used in RTCM ouput mode.
     * It will be called automatically after configuring.
     * @return 0 on success, <0 on error
     */
    int restartSurveyIn();
    int disableTimeMode();

    /**
     * restartSurveyIn for protocol version < 27 (_proto_ver_27_or_higher == false)
     */
    int restartSurveyInPreV27();

    /**
     * Parse the binary UBX packet
     */
    int parseChar(const uint8_t b);

    /**
     * Start payload rx
     */
    int payloadRxInit(void);

    /**
     * Add payload rx byte
     */
    void decodeMonVer(std::span<const uint8_t> payload);
    void decodeNavSat(std::span<const uint8_t> payload);
    void decodeNavSvinfo(std::span<const uint8_t> payload);

    /**
     * Finish payload rx
     */
    int payloadRxDone(GPSPositionReport& position);

    /**
     * Send a message
     * @return true on success, false on write error (errno set)
     */
    bool sendMessage(const uint16_t msg, const uint8_t* payload, const uint16_t length);

    bool sendMessage(uint16_t msg, std::span<const uint8_t> payload)
    {
        return payload.size() <= UINT16_MAX && sendMessage(msg, payload.data(), static_cast<uint16_t>(payload.size()));
    }

    /**
     * Wait for message acknowledge
     */
    int waitForAck(const uint16_t msg, const unsigned timeout, const bool report);

    /**
     * Wait out the GNSS subsystem reset that follows a constellation change
     */
    void waitForGnssReset();

    uint64_t _disable_cmd_last{0};
    uint64_t _next_comms_poll{0};
    uint64_t _comms_poll_deadline{0};
    GPSPositionReport* _gps_position{nullptr};
    GPSSatelliteReport* _satellite_info{nullptr};
    std::array<uint8_t, 4096> _framePayload{};
    UBX::FrameDecoder _frameDecoder;
    int decodeValidatedPayload();
    void flushDecoded() override;
    void publishEpoch(const GPSPositionReport& report);
    UBXNavigationEpoch _navigationEpochs;
    bool _assembleEpochs = false;
    bool _epochHasHighPrecision = false;
    uint8_t _tx_cfg_valset_buf[UBX_CFG_VALSET_BUF_SIZE]{};
    int _tx_cfg_valset_size{0};
    ubx_rxmsg_state_t _rx_state{UBX_RXMSG_IGNORE};

    bool _configured{false};
    bool _decodeNavigation = false;
    bool _survey_in_stopped{false};
    bool _is_m8p{false};
    char _model_name[30]{};
    char _firmware_version[30]{};
    bool _got_posllh{false};
    bool _got_velned{false};
    bool _got_sec_sig{false};             ///< SEC-SIG jammingState supersedes deprecated MON-RF flags
    bool _proto_ver_27_or_higher{false};  ///< true if protocol version 27 or higher detected
    bool _use_nav_pvt{false};

    uint8_t _dyn_model{7};    ///< ublox Dynamic platform model default 7: airborne with <2g acceleration

    uint8_t _output_rate{0};  ///< ublox output rate in Hz, 0 = auto-select based on module
    bool _constellation_configuration_rejected{false};
    bool _constellation_request_rejected{false};
    bool _last_ack_rejected{false};

    uint16_t _rx_msg{};
    uint16_t _rx_payload_length{0};

    uint64_t _last_timestamp_time{0};

    Board _board{Board::unknown};

    OutputMode _output_mode{OutputMode::GPS};

    std::optional<RTCMFramer> _rtcm_parsing;
};
