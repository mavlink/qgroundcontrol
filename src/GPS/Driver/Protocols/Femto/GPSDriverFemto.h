/****************************************************************************
 *
 *   Copyright (C) 2019. All rights reserved.
 *   Author: Rui Zheng <ruizheng@femtomes.com>
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

/** @file Femtomes protocol definitions */
#pragma once

#include <optional>

#include "FemtoMessages.h"
#include "GPSBaseProtocol.h"
#include "RTCMFramer.h"

class GPSDriverFemto : public GPSBaseProtocol
{
public:
    /**
     * @param heading_offset heading offset in radians [-pi, pi]. It is substracted from the measurement.
     */
    GPSDriverFemto(GPSProtocolIO io, struct GPSPositionReport* gps_position,
                   GPSSatelliteReport* satellite_info = nullptr, float heading_offset = 0.f);
    virtual ~GPSDriverFemto();

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    const GPSPositionReport* positionReport() const override { return _gps_position; }

    const GPSSatelliteReport* satelliteReport() const override { return _satellite_info; }

    int configure(unsigned& baudrate, const GPSConfig& config) override;

private:
    void servicePendingCommands() override;
    bool _rtcmActivationPending = false;

    /**
     * when Constructor is work, initialize parameters
     */
    void decodeInit(void);

    /**
     * check the message if whether is 8001,memcpy data to _gps_position
     */
    int handleMessage(int len);

    /**
     * analysis frame data from buf[] to _femto_msg and check the frame is suceess or not
     */
    int parseChar(uint8_t b);

    /**
     * Write a command and wait for a (N)Ack
     * @return 0 on success, <0 otherwise
     */
    int writeAckedCommandFemto(const char* command, const char* reply, const unsigned timeout);

    /**
     * receive data for at least the specified amount of time
     */
    void receiveWait(unsigned timeout_min);

    /**
     * enable output of correction output
     */
    void activateCorrectionOutput();

    /**
     * enable output of rtcm
     */
    void activateRTCMOutput();

    /**
     * update survery in status of QGC RTK GPS
     */
    void sendSurveyInStatusUpdate(bool active, bool valid, double latitude = (double) NAN,
                                  double longitude = (double) NAN, float altitude = NAN);

    struct GPSPositionReport* _gps_position{nullptr};
    FemtoDecodeState _decode_state{FemtoDecodeState::pream_ble1};
    femto_msg_t _femto_msg;
    GPSSatelliteReport* _satellite_info{nullptr};
    float _heading_offset;

    std::optional<RTCMFramer> _rtcm_parsing;
    OutputMode _output_mode{OutputMode::GPS};
    bool _configure_done{false};
    bool _correction_output_activated{false};

    uint64_t _survey_in_start{0};
};
