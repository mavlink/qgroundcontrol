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

#include <cmath>
#include <cstddef>
#include <ctime>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CRC32.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSRawAckMatcher.h"
#include "NMEAFields.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"

namespace {
constexpr unsigned FEMTO_RESPONSE_TIMEOUT = 200;
}

int GPSNativeFemto::writeAckedCommandFemto(const char* command, const char* reply, const unsigned int timeout)
{
    const GPSConfigurationStep step{command, std::chrono::milliseconds(timeout)};
    const size_t command_length = strlen(command);
    uint8_t buf[GPS_READ_BUFFER_SIZE];
    GPSRawAckMatcher matcher(reply, "<ERROR");
    if (!matcher.valid() || !writeCommand(step, {reinterpret_cast<const uint8_t*>(command), command_length})) {
        return -1;
    }

    const auto result = awaitCommand(
        [&] {
            const int count = read(buf, sizeof(buf), timeout);
            if (count <= 0) {
                return;
            }
            matcher.append(std::span(buf).first(count));
        },
        [&] { return matcher.outcome(); });
    return result.evidence.outcome == GPSCommandOutcome::Acknowledged ? 0 : -1;
}

int GPSNativeFemto::configure(unsigned& baudrate, const GPSConfig& config)
{
    _configure_done = false;
    resetIOError();
    _survey_duration = 0;
    _survey_in_start = 0;
    _correction_output_activated = false;
    _rtcmActivationPending = false;
    _rtcm_parsing.reset();
    decodeInit();
    if (!validateConfiguration(config)) {
        return -1;
    }
    _baseConfig = config.base;
    constexpr unsigned supportedBaudrate = 115200;
    bool success = false;

    if (baudrate == 0 || baudrate == supportedBaudrate) {
        setBaudrate(supportedBaudrate);
        for (int run = 0; run < 2; ++run) {
            if (writeAckedCommandFemto("UNLOGALL THISPORT\r\n", "<UNLOGALL OK", FEMTO_RESPONSE_TIMEOUT) == 0 &&
                writeAckedCommandFemto("VERSION\r\n", "<VERSION OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
                success = true;
                break;
            }
        }
    }

    if (!success) {
        return -1;
    }

    baudrate = supportedBaudrate;
    decodeInit();

    /** init rtcm parsing */
    if (!_rtcm_parsing) {
        _rtcm_parsing.emplace();
    }
    _rtcm_parsing->reset();
    activateCorrectionOutput();

    _configure_done = true;

    return ioError();
}

void GPSNativeFemto::activateCorrectionOutput()
{
    if (_correction_output_activated) {
        return;
    }
    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        if (writeAckedCommandFemto("POSAVE ON \r\n", "<POSAVE OK", FEMTO_RESPONSE_TIMEOUT) != 0 ||
            writeAckedCommandFemto("LOG GPGGA 1 \r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) != 0) {
            controlFailed();
            return;
        }
        _survey_duration = 0;
        _survey_in_start = nowUs();
        sendSurveyInStatusUpdate(true, false);
        return;
    }
    const auto& settings = std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode);
    char buffer[100];
    const int length =
        snprintf(buffer, sizeof(buffer), "FIX POSITION %.8lf %.8lf %.5f\r\n", settings.position.latitudeDegrees,
                 settings.position.longitudeDegrees, double(settings.position.altitudeMeters));
    if (length < 0 || length >= int(sizeof(buffer)) ||
        writeAckedCommandFemto(buffer, "FIX OK", FEMTO_RESPONSE_TIMEOUT) != 0 ||
        writeAckedCommandFemto("LOG GPGGA 1 \r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) != 0) {
        controlFailed();
        return;
    }
    activateRTCMOutput();
    if (_correction_output_activated) {
        sendSurveyInStatusUpdate(false, true, settings.position.latitudeDegrees, settings.position.longitudeDegrees,
                                 settings.position.altitudeMeters);
    }
}

void GPSNativeFemto::activateRTCMOutput()
{
    if (writeAckedCommandFemto("LOG RTCM 1\r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) != 0) {
        controlFailed();
        return;
    }
    _correction_output_activated = true;
}
