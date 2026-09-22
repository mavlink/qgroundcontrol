/****************************************************************************
 *
 *   Copyright (c) 2018-2024 PX4 Development Team. All rights reserved.
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
#include <string.h>

#include "GPSRawAckMatcher.h"
#include "RTCMFramer.h"
#include "SBF/GPSDriverSBF.h"

namespace {
constexpr int SBF_CONFIG_TIMEOUT = 1000;
constexpr size_t MSG_SIZE = 100;
}  // namespace

int GPSNativeSBF::configure(unsigned& baudrate, const GPSConfig& config)
{
    _configured = false;
    resetIOError();
    _survey_duration = 0;
    _survey_active = false;
    _survey_activation_date = 0;
    if (!validateConfiguration(config)) {
        return -1;
    }
    _baseConfig = config.base;
    char buf[GPS_READ_BUFFER_SIZE];
    char msg[MSG_SIZE];

    _epochs = {};
    _lastPublishedEpoch.reset();
    _rtcm_parsing.reset();
    decodeInit();

    setBaudrate(SBF_TX_CFG_PRT_BAUDRATE);
    baudrate = SBF_TX_CFG_PRT_BAUDRATE;
    _output_mode = config.output_mode;

    // Make sure we can send commands to the receiver
    sendMessage(SBF_CONFIG_FORCE_INPUT);

    // Disable previous output for now so we can detect the COM port
    for (int i = 1; i <= 2; i++) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_DISABLE_OUTPUT, "COM", i);
        sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT, {}, false);
    }

    for (int i = 1; i <= 4; i++) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_DISABLE_OUTPUT, "USB", i);
        sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT, {}, false);
    }

    char com_port[5]{};
    size_t offset = 1;
    bool response_detected = false;
    uint64_t time_started = nowUs();
    sendMessage("\n\r");

    // Read buffer to get the COM port
    do {
        --offset;  // overwrite the null-char
        int ret = read(reinterpret_cast<uint8_t*>(buf) + offset, sizeof(buf) - offset - 1, SBF_CONFIG_TIMEOUT);

        if (ret < 0) {
            return ret;
        }

        offset += ret;
        buf[offset++] = '\0';

        char* p = strstr(buf, ">");

        if (p) {  // check if the length of the com port == 4 and contains a > sign
            for (int i = 0; i < 4; i++) {
                com_port[i] = buf[i];
            }

            response_detected = true;
        }

        if (offset >= sizeof(buf)) {
            offset = 1;
        }

    } while (time_started + 1000 * SBF_CONFIG_TIMEOUT > nowUs() && !response_detected);

    if (response_detected) {
        log(GPSProtocolLogLevel::Debug, "Septentrio GNSS receiver COM port: %s", com_port);
        response_detected = false;  // for future use

    } else {
        log(GPSProtocolLogLevel::Warning, "No COM port detected");
        return -1;
    }

    // Delete all sbf outputs on current COM port to remove clutter data
    snprintf(msg, sizeof(msg), SBF_CONFIG_RESET, com_port);

    if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
        return -1;  // connection and/or baudrate detection failed
    }

    // Set baudrate, unless we're connected over USB
    if (strncmp(com_port, "USB1", 4) != 0 && strncmp(com_port, "USB2", 4) != 0) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_BAUDRATE, com_port, baudrate);

        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
            return -1;  // connection and/or baudrate detection failed
        }
    }

    // At this point we have correct baudrate on both ends

    // Define/inquire the type of data that the receiver should accept/send on a given connection descriptor
    snprintf(msg, sizeof(msg), SBF_DATA_IO, com_port);

    if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
        return -1;
    }
    std::string outputConfirmation = msg;

    // Septentrio's WGS84/Default selects the global datum except when external corrections supply a datum.
    if (!sendMessageAndWaitForAck("setGeodeticDatum, WGS84\n", SBF_CONFIG_TIMEOUT)) {
        return -1;
    }

    // Set the type of dynamics the GNSS antenna is subjected to.
    if (_output_mode != OutputMode::RTCM) {
        // Release a previously configured static base position before starting navigation.
        if (!sendMessageAndWaitForAck("setPVTMode, Rover, All, auto\n", SBF_CONFIG_TIMEOUT)) {
            return -1;
        }

        // Specify the offsets that the receiver applies to the computed attitude angles.
        snprintf(msg, sizeof(msg), SBF_CONFIG_ATTITUDE_OFFSET, 0.0, 0.0);

        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT, {GPSReceiverSetting::HeadingOffsetDeg})) {
            return -1;
        }

        snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "high");

        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
            return -1;
        }

        snprintf(msg, sizeof(msg), SBF_CONFIG, com_port);

        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
            return -1;
        }
        outputConfirmation = msg;
    }

    // Preserve the receiver's historical second application after the first required ACK.
    // Navigation confirms its SBF stream; base mode confirms the selected input/output port.
    constexpr unsigned OUTPUT_CONFIRMATION_ATTEMPTS = 5;
    bool outputConfirmed = false;
    for (unsigned attempt = 0; attempt < OUTPUT_CONFIRMATION_ATTEMPTS && !ioError(); ++attempt) {
        if (sendMessageAndWaitForAck(outputConfirmation.c_str(), SBF_CONFIG_TIMEOUT)) {
            outputConfirmed = true;
            break;
        }
    }
    if (!outputConfirmed) {
        return -1;
    }

    if (_output_mode == OutputMode::RTCM) {
        if (!_rtcm_parsing) {
            _rtcm_parsing.emplace();
        }

        _rtcm_parsing->reset();
    }

    if (_output_mode != OutputMode::GPS) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_OUTPUT_RTCM3, com_port);
        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
            return -1;
        }
    }

    if (_output_mode == OutputMode::RTCM) {
        if (std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
            snprintf(
                msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_COORDINATES,
                std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode).position.latitudeDegrees,
                std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode).position.longitudeDegrees,
                static_cast<double>(std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode).position.altitudeMeters));
            if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
                return -1;
            }

            snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_OFFSET, 0.0, 0.0, 0.0);
            if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
                return -1;
            }

            if (!sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC1, SBF_CONFIG_TIMEOUT)) {
                return -1;
            }
            if (!sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC2, SBF_CONFIG_TIMEOUT)) {
                return -1;
            }
        } else {
            if (!sendMessageAndWaitForAck(SBF_CONFIG_RTCM_SURVEY_IN, SBF_CONFIG_TIMEOUT)) {
                return -1;
            }
        }

        snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATUS, com_port);
        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
            return -1;
        }
        _survey_activation_date = std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) ? 0 : nowUs();
    }

    _configured = true;
    return ioError();
}

bool GPSNativeSBF::sendMessage(const char* msg)
{
    // Send message

    int length = static_cast<int>(strlen(msg));

    return (write(msg, length) == length);
}

bool GPSNativeSBF::sendMessageAndWaitForAck(const char* msg, int timeout, GPSReceiverSettingSet settings, bool required)
{
    const GPSConfigurationStep step{msg, std::chrono::milliseconds(timeout), settings, required};
    if (!writeCommand(step, {reinterpret_cast<const uint8_t*>(msg), strlen(msg)})) {
        return false;
    }
    std::string_view command(msg);
    while (command.ends_with('\r') || command.ends_with('\n')) {
        command.remove_suffix(1);
    }
    const std::string expected = "$R: " + std::string(command);
    GPSRawAckMatcher matcher(expected, "$R?");
    const auto result = awaitCommand(
        [&] {
            uint8_t bytes[GPS_READ_BUFFER_SIZE];
            const int count = read(bytes, sizeof(bytes), timeout);
            if (count <= 0) {
                return;
            }
            matcher.append(std::span(bytes).first(count));
        },
        [&] { return matcher.outcome(); });
    return result.evidence.outcome == GPSCommandOutcome::Acknowledged;
}
