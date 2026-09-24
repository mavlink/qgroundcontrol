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
