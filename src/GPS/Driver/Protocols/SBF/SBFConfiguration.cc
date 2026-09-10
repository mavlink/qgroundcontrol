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

#include "SBFPrivate.h"

int GPSDriverSBF::configure(unsigned& baudrate, const GPSConfig& config)
{
    _baseConfig = config.base;
    _survey_duration = 0;
    resetIOError();
    char buf[GPS_READ_BUFFER_SIZE];
    char msg[MSG_SIZE];

    _configured = false;
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
        sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);
    }

    for (int i = 1; i <= 4; i++) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_DISABLE_OUTPUT, "USB", i);
        sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);
    }

    char com_port[5]{};
    size_t offset = 1;
    bool response_detected = false;
    gps_abstime time_started = nowUs();
    sendMessage("\n\r");

    // Read buffer to get the COM port
    do {
        --offset;  // overwrite the null-char
        int ret = read(reinterpret_cast<uint8_t*>(buf) + offset, sizeof(buf) - offset - 1, SBF_CONFIG_TIMEOUT);

        if (ret < 0) {
            // something went wrong when reading
            if (ret != ReadCancelled) {
                SBF_WARN("sbf read err");
            }
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
        SBF_INFO("Septentrio GNSS receiver COM port: %s", com_port);
        response_detected = false;  // for future use

    } else {
        SBF_WARN("No COM port detected")
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

    // Set the type of dynamics the GNSS antenna is subjected to.
    if (_output_mode != OutputMode::RTCM) {
        // Release a previously configured static base position before starting navigation.
        if (!sendMessageAndWaitForAck("setPVTMode, Rover, All, auto\n", SBF_CONFIG_TIMEOUT)) {
            return -1;
        }

        // Specify the offsets that the receiver applies to the computed attitude angles.
        snprintf(msg, sizeof(msg), SBF_CONFIG_ATTITUDE_OFFSET, (double) (_heading_offset * 180 / M_PI_F),
                 (double) _pitch_offset);

        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
            return -1;
        }

        if (_dynamic_model < 6) {
            snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "low");

        } else if (_dynamic_model < 7) {
            snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "moderate");

        } else if (_dynamic_model < 8) {
            snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "high");

        } else {
            snprintf(msg, sizeof(msg), SBF_CONFIG_RECEIVER_DYNAMICS, "max");
        }

        sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);

        snprintf(msg, sizeof(msg), SBF_CONFIG, com_port);

        sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);
    }

    int i = 0;

    do {
        ++i;

        if (!sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT)) {
            if (i >= 5) {
                return -1;  // connection and/or baudrate detection failed
            }

        } else {
            response_detected = true;
        }
    } while (i < 5 && !response_detected);

    if (_output_mode == OutputMode::GPSAndRTCM || _output_mode == OutputMode::RTCM) {
        if (!_rtcm_parsing) {
            _rtcm_parsing.emplace();
        }

        _rtcm_parsing->reset();
    }

    if (_output_mode != OutputMode::GPS) {
        sendMessageAndWaitForAck(SBF_CONFIG_OUTPUT_RTCM3, SBF_CONFIG_TIMEOUT);
    }

    if (_output_mode == OutputMode::RTCM) {
        if (_baseConfig.useFixedBase) {
            snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_COORDINATES, _baseConfig.fixedBaseLatitude,
                     _baseConfig.fixedBaseLongitude, static_cast<double>(_baseConfig.fixedBaseAltitudeMeters));
            sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);

            snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_OFFSET, 0.0, 0.0, 0.0);
            sendMessageAndWaitForAck(msg, SBF_CONFIG_TIMEOUT);

            sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC1, SBF_CONFIG_TIMEOUT);
            sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC2, SBF_CONFIG_TIMEOUT);
        } else {
            sendMessageAndWaitForAck(SBF_CONFIG_RTCM_SURVEY_IN, SBF_CONFIG_TIMEOUT);
        }

        sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATUS, SBF_CONFIG_TIMEOUT);
        _survey_active = true;
        _survey_activation_date = nowUs();
    }

    _configured = true;
    return ioError();
}

bool GPSDriverSBF::sendMessage(const char* msg)
{
    // Send message

    int length = static_cast<int>(strlen(msg));

    return (write(msg, length) == length);
}

bool GPSDriverSBF::sendMessageAndWaitForAck(const char* msg, const int timeout)
{
    const Operation operation(*this, timeout);

    beginCommandWrite();
    const int length = static_cast<int>(strlen(msg));
    if (write(msg, length) != length)
        return false;
    const std::string expected = "$R: " + std::string(msg);
    std::string received;
    bool acknowledged = false;
    const auto result = awaitCommand(
        msg, timeout,
        [&] {
            uint8_t bytes[GPS_READ_BUFFER_SIZE];
            const int count = read(bytes, sizeof(bytes), timeout);
            if (count <= 0)
                return;
            received.append(reinterpret_cast<const char*>(bytes), count);
            acknowledged = received.find(expected) != std::string::npos;
            if (received.size() > expected.size())
                received.erase(0, received.size() - expected.size());
        },
        [&] { return acknowledged ? GPSCommandOutcome::Acknowledged : GPSCommandOutcome::Pending; });
    return result.outcome == GPSCommandOutcome::Acknowledged;
}
