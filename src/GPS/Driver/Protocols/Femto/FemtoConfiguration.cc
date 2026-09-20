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

#include "FemtoPrivate.h"

int GPSNativeFemto::writeAckedCommandFemto(const char* command, const char* reply, const unsigned int timeout)
{
    const GPSConfigurationStep step{command, std::chrono::milliseconds(timeout)};
    const size_t command_length = strlen(command);
    const size_t reply_length = strlen(reply);
    uint8_t buf[GPS_READ_BUFFER_SIZE];

    // Keep one full ACK in the bounded receive window.
    if (reply_length == 0 || reply_length > sizeof(buf) ||
        !writeCommand(step, {reinterpret_cast<const uint8_t*>(command), command_length})) {
        return -1;
    }

    size_t buffered = 0;

    bool acknowledged = false;
    const auto result = awaitCommand(
        step,
        [&] {
            const int count = read(buf + buffered, sizeof(buf) - buffered, timeout);
            if (count <= 0) {
                return;
            }
            buffered += static_cast<size_t>(count);
            for (size_t index = 0; index + reply_length <= buffered; ++index) {
                if (memcmp(buf + index, reply, reply_length) == 0) {
                    acknowledged = true;
                }
            }
            if (buffered >= reply_length) {
                const size_t retained = reply_length - 1;
                memmove(buf, buf + buffered - retained, retained);
                buffered = retained;
            }
        },
        [&] { return acknowledged ? GPSCommandOutcome::Acknowledged : GPSCommandOutcome::Pending; });
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
    _output_mode = config.output_mode;
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
    if (_output_mode == OutputMode::RTCM) {
        if (!_rtcm_parsing) {
            _rtcm_parsing.emplace();
        }

        _rtcm_parsing->reset();
    }

    if (_output_mode == OutputMode::GPS) {
        // Stop base averaging and release its fixed position before enabling navigation output.
        if (writeAckedCommandFemto("POSAVE OFF\r\n", "<POSAVE OK", FEMTO_RESPONSE_TIMEOUT) != 0 ||
            writeAckedCommandFemto("FIX NONE\r\n", "<FIX OK", FEMTO_RESPONSE_TIMEOUT) != 0) {
            return -1;
        }

        if (writeAckedCommandFemto("LOG UAVGPSB 0.1\r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
            /** 20Hz need authorization in femtomes device */
            if (writeAckedCommandFemto("LOG UAVGPSB 0.05\r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
            } else if (!ioError()) {
                log(GPSProtocolLogLevel::Warning, "Femto: command LOG UAVGPSB 0.05 failed,maybe no authorization");
            }

        } else {
            if (!ioError()) {
                log(GPSProtocolLogLevel::Warning, "Femto: command LOG UAVGPSB 0.1 failed");
            }
            return -1;
        }

        if (_satellite_info) {
            if (writeAckedCommandFemto("LOG UAVSTATUSB 1\r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
            } else if (!ioError()) {
                log(GPSProtocolLogLevel::Warning, "Femto: command LOG UAVSTATUSB 1 failed");
            }
        }

    } else { /**< RTCM mode for base station */
        activateCorrectionOutput();
    }

    _configure_done = true;

    return ioError();
}

void GPSNativeFemto::activateCorrectionOutput()
{
    if (_output_mode != OutputMode::RTCM || _correction_output_activated) {
        return;
    }
    if (!_baseConfig.useFixedBase) {
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
    const auto& settings = _baseConfig;
    char buffer[100];
    const int length =
        snprintf(buffer, sizeof(buffer), "FIX POSITION %.8lf %.8lf %.5f\r\n", settings.fixedPosition.latitudeDegrees,
                 settings.fixedPosition.longitudeDegrees, double(settings.fixedPosition.altitudeMeters));
    if (length < 0 || length >= int(sizeof(buffer)) ||
        writeAckedCommandFemto(buffer, "FIX OK", FEMTO_RESPONSE_TIMEOUT) != 0 ||
        writeAckedCommandFemto("LOG GPGGA 1 \r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) != 0) {
        controlFailed();
        return;
    }
    activateRTCMOutput();
    if (_correction_output_activated) {
        sendSurveyInStatusUpdate(false, true, settings.fixedPosition.latitudeDegrees,
                                 settings.fixedPosition.longitudeDegrees, settings.fixedPosition.altitudeMeters);
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
