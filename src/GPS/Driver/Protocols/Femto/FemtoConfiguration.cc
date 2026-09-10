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

int GPSDriverFemto::writeAckedCommandFemto(const char* command, const char* reply, const unsigned int timeout)
{
    const Operation operation(*this, timeout);
    const size_t command_length = strlen(command);
    const size_t reply_length = strlen(reply);
    uint8_t buf[GPS_READ_BUFFER_SIZE];

    // Keep one full ACK in the bounded receive window.
    if (reply_length == 0 || reply_length > sizeof(buf) ||
        write(command, command_length) != static_cast<int>(command_length)) {
        return -1;
    }

    size_t buffered = 0;

    while (nowUs() < _operationDeadline.untilUs) {
        int ret = read(buf + buffered, sizeof(buf) - buffered, timeout);

        if (ret < 0) {
            return ret;
        }

        buffered += static_cast<size_t>(ret);

        for (size_t i = 0; i + reply_length <= buffered; ++i) {
            if (memcmp(buf + i, reply, reply_length) == 0) {
                return 0;
            }
        }

        // Retain enough bytes to recognize an ACK split across reads, even after a full buffer of noise.
        if (buffered >= reply_length) {
            const size_t retained = reply_length - 1;
            memmove(buf, buf + buffered - retained, retained);
            buffered = retained;
        }
    }

    return -1;
}

int GPSDriverFemto::configure(unsigned& baudrate, const GPSConfig& config)
{
    _baseConfig = config.base;
    _survey_duration = 0;
    resetIOError();

    if (config.output_mode != OutputMode::GPS && config.output_mode != OutputMode::RTCM) {
        return -1;
    }

    _output_mode = config.output_mode;
    _configure_done = false;
    _correction_output_activated = false;
    /** Try different baudrates (115200 is the default for Femtomes) and request the baudrate that we want.	 */
    const unsigned baudrates_to_try[] = {115200};
    bool success = false;

    unsigned test_baudrate = 0;

    for (unsigned int baud_i = 0; !success && baud_i < sizeof(baudrates_to_try) / sizeof(baudrates_to_try[0]);
         baud_i++) {
        test_baudrate = baudrates_to_try[baud_i];

        if (baudrate > 0 && baudrate != test_baudrate) {
            continue; /**< skip to next baudrate*/
        }

        setBaudrate(test_baudrate);

        for (int run = 0; run < 2; ++run) { /** try several times*/
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

    /**
     * We successfully got a response and know to which port we are connected. Now set the desired baudrate
     * if it's different from the current one.
     */
    const unsigned desired_baudrate = 115200; /**< changing this requires also changing the SPD command*/

    baudrate = test_baudrate;

    if (baudrate != desired_baudrate) {
        baudrate = desired_baudrate;
        const char baud_config[] = "com 115200\r\n";  // configure baudrate to 115200
        write(baud_config, sizeof(baud_config));
        decodeInit();
        receiveWait(200);
        decodeInit();
        setBaudrate(baudrate);

        success = false;

        for (int run = 0; run < 10; ++run) {
            /** We ask for the port config again. If we get a reply, we know that the changed settings work.*/
            if (writeAckedCommandFemto("UNLOGALL THISPORT\r\n", "<UNLOGALL OK", FEMTO_RESPONSE_TIMEOUT) == 0 &&
                writeAckedCommandFemto("VERSION\r\n", "<VERSION OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
                success = true;
                break;
            }
        }

        if (!success) {
            return -1;
        }

    } else {
        decodeInit();
    }

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
            } else {
                FEMTO_ERR("Femto: command LOG UAVGPSB 0.05 failed,maybe no authorization");
            }

        } else {
            FEMTO_ERR("Femto: command LOG UAVGPSB 0.1 failed");
        }

        if (_satellite_info) {
            if (writeAckedCommandFemto("LOG UAVSTATUSB 1\r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
            } else {
                FEMTO_ERR("Femto: command LOG UAVSTATUSB 1 failed");
            }
        }

    } else { /**< RTCM mode for base station */
        activateCorrectionOutput();
    }

    _configure_done = true;

    return ioError();
}

void GPSDriverFemto::activateCorrectionOutput()
{
    if (_output_mode != OutputMode::RTCM) {
        return; /**< only for base station */
    }

    char buffer[100];

    if (!_baseConfig.useFixedBase) {
        if (writeAckedCommandFemto("POSAVE ON \r\n", "<POSAVE OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
            if (writeAckedCommandFemto("LOG GPGGA 1 \r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) ==
                0) { /**< for updating GPS satellite count of RTK */

            } else {
                FEMTO_ERR("Femto: LOG GPGGA command failed")
            }

        } else {
            FEMTO_ERR("Femto: command POSAVE ON failed")
        }

        _survey_duration = 0;  // use it as counter how long survey-in has been active
        _survey_in_start = nowUs();
        sendSurveyInStatusUpdate(true, false);

    } else {
        const GPSBaseStationConfig& settings = _baseConfig;
        int len = snprintf(buffer, sizeof(buffer), "FIX POSITION %.8lf %.8lf %.5f\r\n", settings.fixedBaseLatitude,
                           settings.fixedBaseLongitude, (double) settings.fixedBaseAltitudeMeters);

        if (len >= 0 && len < (int) (sizeof(buffer))) {
            if (writeAckedCommandFemto(buffer, "FIX OK", FEMTO_RESPONSE_TIMEOUT) == 0) {
                activateRTCMOutput();
                sendSurveyInStatusUpdate(false, true, settings.fixedBaseLatitude, settings.fixedBaseLongitude,
                                         settings.fixedBaseAltitudeMeters);

                if (writeAckedCommandFemto("LOG GPGGA 1 \r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) ==
                    0) { /**< for updating GPS satellite count of RTK */

                } else {
                    FEMTO_ERR("Femto: LOG GPGGA command failed")
                }

            } else {
                FEMTO_ERR("Femto: fix base station position failed.")
            }

        } else {
        }
    }
}

void GPSDriverFemto::activateRTCMOutput()
{
    if (writeAckedCommandFemto("LOG RTCM 1\r\n", "<LOG OK", FEMTO_RESPONSE_TIMEOUT) != 0) {
        FEMTO_ERR("Femto: command LOG RTCM failed")

    } else {
    }

    _correction_output_activated = true;
}
