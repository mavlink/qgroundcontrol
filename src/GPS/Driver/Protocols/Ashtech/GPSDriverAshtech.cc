/****************************************************************************
 *
 *   Copyright (c) 2012-2018 PX4 Development Team. All rights reserved.
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

#include "AshtechPrivate.h"

GPSDriverAshtech::GPSDriverAshtech(GPSProtocolIO io, GPSPositionReport* gps_position,
                                   GPSSatelliteReport* satellite_info, float heading_offset)
    : GPSBaseProtocol(std::move(io))
    , _heading_offset(heading_offset)
    , _gps_position(gps_position)
    , _satellite_info(satellite_info)
{
    decodeInit();
}

GPSDriverAshtech::~GPSDriverAshtech() {}

void GPSDriverAshtech::receiveWait(unsigned timeout_min)
{
    gps_abstime time_started = nowUs();

    while (nowUs() < time_started + timeout_min * 1000) {
        receive(timeout_min);
        if (ioError()) {
            return;
        }
    }
}

int GPSDriverAshtech::receive(unsigned timeout)
{
    const int result = receiveDecoded(timeout);
    serviceControls();
    return ioError() ? ioError() : result;
}

void GPSDriverAshtech::servicePendingCommands()
{
    if (_correctionSetupPending) {
        _correctionSetupPending = false;
        activateCorrectionOutput();
    }
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        activateRTCMOutput();
    }
}
