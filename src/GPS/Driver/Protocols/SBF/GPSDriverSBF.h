/****************************************************************************
 *
 *   Copyright (c) 2018 PX4 Development Team. All rights reserved.
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
 * @file sbf.h
 *
 * Septentrio protocol as defined in PPSDK SBF Reference Guide 4.1.8
 *
 * @author Matej Franceskin <Matej.Franceskin@gmail.com>
 * @author <a href="https://github.com/SeppeG">Seppe Geuens</a>
 *
 */

#pragma once

#include "GPSBaseProtocol.h"
#include "RTCMFramer.h"
#include "SBFMessages.h"

class GPSDriverSBF : public GPSBaseProtocol
{
public:
    /**
     * @param heading_offset heading offset in radians [-pi, pi]. It is subtracted from the measurement.
     * @param pitch_offset pitch_offset in deg [-90, 90]. This will be send as a cmd to the receiver.
     */
    GPSDriverSBF(GPSProtocolIO io, struct GPSPositionReport* gps_position, GPSSatelliteReport* satellite_info = nullptr,
                 float heading_offset = 0.f, float pitch_offset = 0.f);

    virtual ~GPSDriverSBF();

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    const GPSPositionReport* positionReport() const override { return _gps_position; }

    const GPSSatelliteReport* satelliteReport() const override { return _satellite_info; }

    int configure(unsigned& baudrate, const GPSConfig& config) override;

private:
    /**
     * @brief Parse the binary SBF packet
     */
    int parseChar(const uint8_t b);

    struct NavigationEpoch
    {
        uint64_t receiverTimeMs = 0;
        uint64_t receiptUs = 0;
        GPSPositionReport position;
        bool hasPosition = false;
    };

    NavigationEpoch* navigationEpoch(uint64_t receiverTimeMs);
    void finishEpoch(std::optional<NavigationEpoch>& epoch);
    void flushDecoded() override;
    std::array<std::optional<NavigationEpoch>, 2> _epochs;
    std::optional<uint64_t> _lastPublishedEpoch;
    static constexpr uint64_t EPOCH_MAX_AGE_US = 200000;
    static constexpr uint64_t WEEK_MS = 604800000;

    /**
     * @brief Add payload rx byte
     */
    int payloadRxAdd(const uint8_t b);

    /**
     * @brief Parses incoming SBF blocks
     */
    int payloadRxDone(void);

    /**
     * @brief Reset the parse state machine for a fresh start
     */
    void decodeInit(void);

    /**
     * @brief Send a message
     * @return true on success, false on write error (errno set)
     */
    bool sendMessage(const char* msg);

    /**
     * @brief Send a message and waits for acknowledge
     * @return true on success, false on write error (errno set) or ack wait timeout
     */
    bool sendMessageAndWaitForAck(const char* msg, const int timeout);

    /**
     * @brief Configures the SBF Output blocks
     * @return true on success, false on write error (errno set) or ack wait timeout
     */
    bool configSBFOutput(const char* com_port);

    GPSPositionReport* _gps_position{nullptr};
    GPSSatelliteReport* _satellite_info{nullptr};
    uint8_t _dynamic_model{7};
    bool _configured{false};
    sbf_decode_state_t _decode_state{SBF_DECODE_SYNC1};
    uint16_t _rx_payload_index{0};
    sbf_buf_t _buf;
    std::array<uint8_t, sizeof(sbf_buf_t)> _wire{};
    OutputMode _output_mode{OutputMode::GPS};
    std::optional<RTCMFramer> _rtcm_parsing;

    const float _heading_offset;
    const float _pitch_offset;
    bool _survey_active{false};
    gps_abstime _survey_activation_date{0};
};

uint16_t crc16(const uint8_t* buf, uint32_t len);
