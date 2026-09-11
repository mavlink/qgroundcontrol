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

void GPSDriverAshtech::activateRTCMOutput()
{
    char buffer[40];
    const char* rtcm_options[] = {
        "$PASHS,NME,POS,%c,ON,0.2\r\n",  // reduce position updates to 5 Hz

        "$PASHS,RT3,1074,%c,ON,1\r\n",   // GPS observations
        "$PASHS,RT3,1084,%c,ON,1\r\n",   // GLONASS observations
        "$PASHS,RT3,1094,%c,ON,1\r\n",   // Galileo observations

        "$PASHS,RT3,1114,%c,ON,1\r\n",   // QZSS observations
        "$PASHS,RT3,1124,%c,ON,1\r\n",   // BDS observations
        "$PASHS,RT3,1006,%c,ON,1\r\n",   // Static position
        "$PASHS,RT3,1033,%c,ON,31\r\n",  // Antenna and receiver name
        "$PASHS,RT3,1013,%c,ON,1\r\n",   // System parameters
        "$PASHS,RT3,1029,%c,ON,1\r\n",   // ASCII message
        "$PASHS,RT3,1230,%c,ON\r\n",     // GLONASS code phase bias

        // TODO: are these required (these are the ones from u-blox)?
        "$PASHS,RT3,1005,%c,ON,1\r\n",
        "$PASHS,RT3,1077,%c,ON,1\r\n",
        "$PASHS,RT3,1087,%c,ON,1\r\n",
    };

    for (unsigned int conf_i = 0; conf_i < sizeof(rtcm_options) / sizeof(rtcm_options[0]); conf_i++) {
        int str_len = snprintf(buffer, sizeof(buffer), rtcm_options[conf_i], _port);

        if (writeAckedCommand(buffer, str_len, ASH_RESPONSE_TIMEOUT) != 0) {
            controlFailed();
            return;
        }
    }
}

int GPSDriverAshtech::writeAckedCommand(const void* buf, int buf_length, unsigned timeout)
{
    const Operation operation(*this, timeout);
    beginCommandWrite(std::string(static_cast<const char*>(buf), buf_length));
    if (write(buf, buf_length) != buf_length) {
        return -1;
    }

    return waitForReply(NMEACommand::Acked, timeout);
}

int GPSDriverAshtech::waitForReply(NMEACommand command, const unsigned timeout)
{
    const Operation operation(*this, timeout);

    _command_state = NMEACommandState::waiting;
    _waiting_for_command = command;

    const auto result = awaitCommand(
        {std::to_string(static_cast<int>(command)), std::chrono::milliseconds(timeout)},
        [this, timeout] { receiveDecoded(timeout); },
        [this] {
            return _command_state == NMEACommandState::received ? GPSCommandOutcome::Acknowledged
                   : _command_state == NMEACommandState::nack   ? GPSCommandOutcome::Rejected
                                                                : GPSCommandOutcome::Pending;
        });
    return result.outcome == GPSCommandOutcome::Acknowledged ? 0 : -1;
}

int GPSDriverAshtech::configure(unsigned& baudrate, const GPSConfig& config)
{
    _baseConfig = config.base;
    _survey_duration = 0;
    resetIOError();
    _output_mode = config.output_mode;
    _correction_output_activated = false;
    _configure_done = false;

    /* Try different baudrates (115200 is the default for Trimble) and request the baudrate that we want.
     *
     * These are Ashtech proprietary commands, we can use them for auto-detection:
     * $PASHS for setting
     * $PASHQ for querying
     * $PASHR for a response
     */
    const unsigned baudrates_to_try[] = {9600, 38400, 19200, 57600, 115200};
    bool success = false;

    unsigned test_baudrate;

    for (unsigned int baud_i = 0; !success && baud_i < sizeof(baudrates_to_try) / sizeof(baudrates_to_try[0]);
         baud_i++) {
        test_baudrate = baudrates_to_try[baud_i];

        if (baudrate > 0 && baudrate != test_baudrate) {
            continue;  // skip to next baudrate
        }

        setBaudrate(test_baudrate);

        const char port_config[] = "$PASHQ,PRT\r\n";  // ask for the current port configuration

        for (int run = 0; run < 2; ++run) {           // try several times
            beginCommandWrite();
            write(port_config, sizeof(port_config) - 1);

            if (waitForReply(NMEACommand::PRT, ASH_RESPONSE_TIMEOUT) == 0) {
                success = true;
                break;
            }
        }
    }

    if (!success) {
        return -1;
    }

    // We successfully got a response and know to which port we are connected. Now set the desired baudrate
    // if it's different from the current one.
    const unsigned desired_baudrate = 115200;  // changing this requires also changing the SPD command

    baudrate = test_baudrate;

    if (baudrate != desired_baudrate) {
        baudrate = desired_baudrate;
        const char baud_config[] = "$PASHS,SPD,%c,9\r\n";  // configure baudrate to 115200
        char baud_config_str[sizeof(baud_config)];
        int len = snprintf(baud_config_str, sizeof(baud_config_str), baud_config, _port);
        beginCommandWrite();
        write(baud_config_str, len);
        decodeInit();
        receiveWait(200);
        decodeInit();
        setBaudrate(baudrate);

        success = false;

        for (int run = 0; run < 10; ++run) {
            // We ask for the port config again. If we get a reply, we know that the changed settings work.
            const char port_config[] = "$PASHQ,PRT\r\n";
            beginCommandWrite();
            write(port_config, sizeof(port_config) - 1);

            if (waitForReply(NMEACommand::PRT, ASH_RESPONSE_TIMEOUT) == 0) {
                success = true;
                break;
            }
        }

        if (!success) {
            return -1;
        }
    }

    // Additional commands that might be useful:
    //		Reading firmware version:
    //			$PASHQ,VER
    //		Reading installed firmware options:
    //			$PASHQ,OPTION
    //		The output for the Trimble MB-two is:
    //			$PASHR,OPTION,0,SERIAL NUMBER,5730C00370*3E
    //			$PASHR,OPTION,@1,GEOFENCING_WW,034017C7114ED*36
    //			$PASHR,OPTION,N,GPS,0340173F8924D*66
    //			$PASHR,OPTION,G,GLONASS,0340178A9E138*69
    //			$PASHR,OPTION,B,BEIDOU,03401434EC35A*4D
    //			$PASHR,OPTION,X,L1TRACKING,0340119C547B8*40
    //			$PASHR,OPTION,Y,L2TRACKING,034012CD03607*42
    //			$PASHR,OPTION,W,20HZ,034016B5A5225*2A
    //			$PASHR,OPTION,J,RTKROVER,034010C800693*41
    //			$PASHR,OPTION,K,RTKBASE,03401065AB099*7E
    //			$PASHR,OPTION,D,DUO,0340138851415*70
    //			$PASHR,OPTION,S,L3TRACKING,034011C7AB73D*48
    //		Reset the full configuration (however it will lead to a reboot and requires about 15s waiting time)
    //			$PASHS,RST

    // get the board identification
    const char board_identification[] = "$PASHQ,RID\r\n";

    beginCommandWrite();
    if (write(board_identification, sizeof(board_identification) - 1) == sizeof(board_identification) - 1) {
        if (waitForReply(NMEACommand::RID, ASH_RESPONSE_TIMEOUT) != 0) {
            return -1;
        }
    }

    // Now configure the messages we want

    const char update_rate[] = "$PASHS,POP,20\r\n";  // set internal update rate to 20 Hz

    if (writeAckedCommand(update_rate, sizeof(update_rate) - 1, ASH_RESPONSE_TIMEOUT) != 0) {
        // for some reason we don't get a response here
    }

    // Enable dual antenna mode (2: both antennas are L1/L2 GNSS capable, flex mode, avoids the need to determine
    // the baseline length through a prior calibration stage)
    // Needs to be set before other commands
    const bool use_dual_mode = _output_mode != OutputMode::RTCM && _board == AshtechBoard::trimble_mb_two;

    if (use_dual_mode) {
        const char duo_mode[] = "$PASHS,SNS,DUO,2\r\n";

        if (writeAckedCommand(duo_mode, sizeof(duo_mode) - 1, ASH_RESPONSE_TIMEOUT) != 0) {
        }

    } else {
        const char solo_mode[] = "$PASHS,SNS,SOL\r\n";

        if (writeAckedCommand(solo_mode, sizeof(solo_mode) - 1, ASH_RESPONSE_TIMEOUT) != 0) {
        }
    }

    char buffer[40];
    const char* config_options[] = {
        "$PASHS,NME,ALL,%c,OFF\r\n",      // disable all NMEA and NMEA-Like Messages
        "$PASHS,ATM,ALL,%c,OFF\r\n",      // disable all ATM (ATOM) Messages
        "$PASHS,OUT,%c,ON\r\n",           // enable periodic output
        "$PASHS,NME,ZDA,%c,ON,3\r\n",     // enable ZDA (date & time) output every 3s
        "$PASHS,NME,GST,%c,ON,3\r\n",     // position accuracy messages
        "$PASHS,NME,POS,%c,ON,0.05\r\n",  // position & velocity (we can go up to 20Hz if FW option [W] is given and to
                                          // 50Hz if [8] is given)
        "$PASHS,NME,GSV,%c,ON,1\r\n"      // satellite status
    };

    for (unsigned int conf_i = 0; conf_i < sizeof(config_options) / sizeof(config_options[0]); conf_i++) {
        int len = snprintf(buffer, sizeof(buffer), config_options[conf_i], _port);

        if (writeAckedCommand(buffer, len, ASH_RESPONSE_TIMEOUT) != 0) {
            // some commands are not acked (e.g. GSV), so don't make this fatal
        }
    }

    if (use_dual_mode) {
        // enable heading output
        const char heading_output[] = "$PASHS,NME,HDT,%c,ON,0.05\r\n";
        int len = snprintf(buffer, sizeof(buffer), heading_output, _port);

        if (writeAckedCommand(buffer, len, ASH_RESPONSE_TIMEOUT) != 0) {
        }
    }

    if (_output_mode == OutputMode::RTCM) {
        if (!_rtcm_parsing) {
            _rtcm_parsing.emplace();
        }

        _rtcm_parsing->reset();
    }

    if (_output_mode == OutputMode::RTCM && _board == AshtechBoard::trimble_mb_two) {
        GPSSurveyReport status{};
        status.latitude = status.longitude = (double) NAN;
        status.altitude = NAN;
        status.duration = 0;
        status.mean_accuracy = 0;
        const bool valid = false;
        const bool active = true;
        status.flags = (int) valid | ((int) active << 1);
        surveyInStatus(status);
    }

    _configure_done = true;
    return ioError();
}

void GPSDriverAshtech::activateCorrectionOutput()
{
    if (_correction_output_activated || _output_mode != OutputMode::RTCM) {
        return;
    }

    char buffer[100];

    if (!_baseConfig.useFixedBase) {
        // setup the base reference: average the position over N seconds
        const char avg_pos[] = "$PASHS,POS,AVG,%i\r\n";
        // alternatively use the current position as reference: "$PASHS,POS,CUR\r\n"
        int len = snprintf(buffer, sizeof(buffer), avg_pos, (int) _baseConfig.surveyInDurationSecs);

        beginCommandWrite();
        write(buffer, len);

        if (waitForReply(NMEACommand::RECEIPT, ASH_RESPONSE_TIMEOUT) != 0) {
            controlFailed();
            return;
        }

        const char* config_options[] = {
            "$PASHS,ANP,OWN,TRM55971.00\r\n",  // set antenna name (arbitrary)
            "$PASHS,STI,0001\r\n"              // enter a base ID
        };

        for (unsigned int conf_i = 0; conf_i < sizeof(config_options) / sizeof(config_options[0]); conf_i++) {
            if (writeAckedCommand(config_options[conf_i], strlen(config_options[conf_i]), ASH_RESPONSE_TIMEOUT) != 0) {
                controlFailed();
                return;
            }
        }

        _survey_duration = 0;  // use it as counter how long survey-in has been active
        _survey_in_start = nowUs();
        sendSurveyInStatusUpdate(true, false);

    } else {
        const GPSBaseStationConfig& settings = _baseConfig;
        char ns, ew;
        double latitude = settings.fixedBaseLatitude;

        if (latitude < 0.) {
            latitude = -latitude;
            ns = 'S';

        } else {
            ns = 'N';
        }

        // convert to ddmm.mmmmmm format
        latitude = ((int) latitude) * 100. + (latitude - ((int) latitude)) * 60.;

        double longitude = settings.fixedBaseLongitude;

        if (longitude < 0.) {
            longitude = -longitude;
            ew = 'W';

        } else {
            ew = 'E';
        }

        // convert to ddmm.mmmmmm format
        longitude = ((int) longitude) * 100. + (longitude - ((int) longitude)) * 60.;

        int len = snprintf(buffer, sizeof(buffer), "$PASHS,POS,%.8f,%c,%.8f,%c,%.5f,PC1", latitude, ns, longitude, ew,
                           (double) settings.fixedBaseAltitudeMeters);

        if (len >= 0 && len < (int) sizeof(buffer)) {
            if (writeAckedCommand(buffer, len, ASH_RESPONSE_TIMEOUT) != 0) {
                controlFailed();
                return;
            }

        } else {
            controlFailed();
            return;
        }

        activateRTCMOutput();
        if (ioError())
            return;
        sendSurveyInStatusUpdate(false, true, settings.fixedBaseLatitude, settings.fixedBaseLongitude,
                                 settings.fixedBaseAltitudeMeters);
    }
    _correction_output_activated = true;
}
