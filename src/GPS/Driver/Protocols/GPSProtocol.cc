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

#include "GPSProtocol.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "GPSProtocolTime.h"
#include <GeographicLib/Geocentric.hpp>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383280
#endif

/**
 * @file gps_helper.cpp
 *
 * @author Thomas Gubler <thomasgubler@student.ethz.ch>
 * @author Julian Oes <julian@oes.ch>
 */

GPSProtocol::GPSProtocol(GPSProtocolIO io)
    : _io(std::move(io))
{
    if (!_io.nowUs)
        _io.nowUs = [] { return gps_absolute_time(); };
    if (!_io.wait)
        _io.wait = [](std::chrono::microseconds duration) {
            gps_usleep(duration.count());
            return true;
        };
}

void GPSProtocol::ECEF2lla(double ecef_x, double ecef_y, double ecef_z, double& latitude, double& longitude,
                           float& altitude)
{
    double height;
    GeographicLib::Geocentric::WGS84().Reverse(ecef_x, ecef_y, ecef_z, latitude, longitude, height);
    altitude = static_cast<float>(height);
}

double GPSProtocol::nmeaToDegrees(double ddmm)
{
    if (!std::isfinite(ddmm) || std::abs(ddmm) > 18000.0) {
        return NAN;
    }
    const double degrees = std::trunc(ddmm / 100.0);
    const double minutes = ddmm - degrees * 100.0;
    return std::abs(minutes) < 60.0 ? degrees + minutes / 60.0 : NAN;
}

uint64_t GPSProtocol::timeFromUtc(tm& utc, int32_t nsec)
{
#ifndef NO_MKTIME
    const time_t epoch = gpsTimeToEpoch(utc);

    if (epoch > GPS_EPOCH_SECS) {
        return static_cast<uint64_t>(epoch) * 1000000ULL + nsec / 1000;
    }

#endif
    return 0;
}

int GPSProtocol::receiveDecoded(unsigned timeout)
{
    const Operation operation(*this, timeout);
    const uint64_t deadline = _operationDeadline.untilUs;
    uint8_t buffer[GPS_READ_BUFFER_SIZE];
    do {
        const int count = read(buffer, sizeof(buffer), remainingMilliseconds(deadline));
        if (count < 0) {
            return count;
        }
        const int handled = consume({buffer, static_cast<size_t>(count)});
        if (ioError()) {
            return ioError();
        }
        if (handled) {
            return handled;
        }
    } while (nowUs() < deadline);
    return -1;
}

void GPSProtocol::serviceControls()
{
    if (_servicingControls || ioError())
        return;
    _servicingControls = true;
    {
        // This budget belongs to pending receiver commands, independently of the completed read slice.
        const Operation operation(*this, 5000);
        servicePendingCommands();
    }
    _servicingControls = false;
}
