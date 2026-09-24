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

#include "GPSProtocol.h"
#include "UBXFrameDecoder.h"
#include "UBXMessages.h"
#include "UBXNavigationEpoch.h"
#include "UBXReceiverController.h"
#include "UBXReceiverProfile.h"

class GPSNativeUBX : public GPSProtocol
{
public:
    explicit GPSNativeUBX(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    virtual ~GPSNativeUBX();

    int configure(unsigned& baudrate, const GPSConfig& config) override;

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    bool receiverReady() const override { return _configured; }

    using Board = UBX::Board;

    const Board& board() const { return _identity.board; }

    const char* modelName() const { return _identity.modelName; }

    const char* firmwareVersion() const { return _identity.firmwareVersion; }
    enum class BaseStationCapability : uint8_t
    {
        Unknown,
        Unsupported,
        Supported,
    };
    BaseStationCapability baseStationCapability() const;

    struct DecodeContext
    {
        bool navigation = false;
        bool useNavPvt = true;
        bool corrections = false;
        bool assembleEpochs = false;
    };

    void setDecodeContext(DecodeContext context);

private:
    void servicePendingCommands() override;
    bool _rtcmActivationPending = false;
    uint16_t _pendingDisableMessage = 0;
    UBX::ReceiverController _controller;

    /** Like receive(), but reports a negative device read separately from a timeout. */
    int receiveInternal(unsigned timeout, bool& read_error);

    void requestCommsDiagnostics();
    void logCommsDiagnostics(std::span<const uint8_t> payload);

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
    bool configureMessageRate(const uint16_t msg, const uint8_t rate, bool required = true);

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
     * Add a configuration value to the pending CFG-VALSET batch.
     * The value width on the wire comes from the key ID's size field, not from T; T documents the
     * call site and must agree with the key.
     * @param key_id one of the UBX_CFG_KEY_* constants
     * @param value configuration value
     * @return false for an invalid value or full batch; failure prevents sending until initCfgValset()
     */
    template <typename T>
    bool cfgValset(uint32_t key_id, T value)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "CFG-VALSET values wider than 4 bytes are not supported");
        return cfgValsetRaw(key_id, static_cast<uint32_t>(static_cast<std::make_unsigned_t<T>>(value)));
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
    };

    /**
     * Add a fixed list of 1-byte configuration values
     * @return true on success, false if buffer too small
     */
    bool cfgValset(std::span<const CfgValsetItem> items);

    /**
     * Add the same 1-byte value for a list of keys
     * @return true on success, false if buffer too small
     */
    bool cfgValset(std::span<const uint32_t> keys, uint8_t value);

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
    bool cfgValsetPort(std::span<const uint32_t> keys, uint8_t value);

    /**
     * Reset the parse state machine for a fresh start
     */
    void decodeInit(void);

    /**
     * Start a new CFG-VALSET batch (header only, no config values yet)
     */
    void initCfgValset();

    /**
     * Send the CFG-VALSET built up by initCfgValset() and cfgValset*() calls
     * @return true on success
     */
    bool sendCfgValset(bool required = true, unsigned timeout = UBX_CONFIG_TIMEOUT);

    /**
     * sendCfgValset() followed by waitForAck()
     * @param required whether receiver acknowledgement is required by the configuration policy
     * @return retained evidence for this attempt
     */
    GPSCommandResult sendCfgValsetAcked(bool required = true);

    /**
     * Start or restart the survey-in procees. This is only used in RTCM ouput mode.
     * It will be called automatically after configuring.
     * @return 0 on success, <0 on error
     */
    int restartSurveyIn();
    int disableTimeMode();
    int verifyConfigValue(uint32_t key, uint8_t value);
    int waitForSurveyStop();
    bool _valsetAckAmbiguous = false;

    /**
     * restartSurveyIn for protocol version < 27
     */
    int restartSurveyInPreV27();

    /**
     * Parse the binary UBX packet
     */
    int parseChar(const uint8_t b);

    /**
     * Start payload rx
     */
    bool payloadRxInit(uint16_t message, std::span<const uint8_t> payload);

    /**
     * Add payload rx byte
     */
    void decodeMonVer(std::span<const uint8_t> payload);
    void decodeNavSat(std::span<const uint8_t> payload);
    void decodeNavSvinfo(std::span<const uint8_t> payload);

    /**
     * Finish payload rx
     */
    int payloadRxDone(uint16_t message, std::span<const uint8_t> payload, GPSNativePositionReport& position);

    /**
     * Send a message
     * @return true on success, false on write error (errno set)
     */
    bool sendMessage(uint16_t msg, const uint8_t* payload, uint16_t length,
                     GPSConfigurationStep step = {{}, std::chrono::milliseconds(UBX_CONFIG_TIMEOUT)});

    bool sendMessage(uint16_t msg, std::span<const uint8_t> payload,
                     GPSConfigurationStep step = {{}, std::chrono::milliseconds(UBX_CONFIG_TIMEOUT)})
    {
        return payload.size() <= UINT16_MAX &&
               sendMessage(msg, payload.data(), static_cast<uint16_t>(payload.size()), std::move(step));
    }

    /**
     * Wait for message acknowledge
     */
    GPSCommandResult waitForAck(uint16_t msg);
    GPSCommandResult verifyCfgValset(GPSConfigurationStep step);

    /**
     * Wait out the GNSS subsystem reset that follows a constellation change
     */
    void waitForGnssReset();

    uint64_t _disable_cmd_last{0};
    UBX::FrameDecoder _frameDecoder;
    int decodeValidatedPayload(uint16_t message, std::span<const uint8_t> payload);
    void flushDecoded() override;
    void publishEpoch(const GPSNativePositionReport& report);
    UBXNavigationEpoch _navigationEpochs;
    DecodeContext _decodeContext;
    bool _epochHasHighPrecision = false;

    struct ReceiverIdentity
    {
        Board board = Board::unknown;
        bool isM8p = false;
        bool protocol27 = false;
        bool timeModeUnsupported = false;
        char modelName[30]{};
        char firmwareVersion[30]{};
    };

    struct CommsPoll
    {
        bool pending = false;
        uint64_t nextUs = 0;
        uint64_t deadlineUs = 0;
    };

    struct TimeModeReadback
    {
        bool pending = false;
        std::optional<uint8_t> response = std::nullopt;
    };

    ReceiverIdentity _identity;
    UBX::CheckedValsetBatch<UBX_CFG_VALSET_BUF_SIZE> _valset;
    CommsPoll _comms;
    TimeModeReadback _timeModeReadback;

    bool _configured{false};
    bool _survey_in_stopped{false};
    bool _got_posllh{false};
    bool _got_velned{false};
    bool _got_sec_sig{false};             ///< SEC-SIG jammingState supersedes deprecated MON-RF flags

    uint8_t _dyn_model{7};  ///< ublox Dynamic platform model default 7: airborne with <2g acceleration

    OutputMode _output_mode{OutputMode::GPS};

    std::optional<RTCMStreamDecoder> _rtcm_parsing;
};
