#include <cmath>
#include <cstddef>
#include <ctime>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CRC32.h"
#include "Femto/FemtoProtocol.h"
#include "GPSRawAckMatcher.h"
#include "NMEAFields.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"

QGC_LOGGING_CATEGORY(FemtoProtocolLog, "GPS.Driver.Protocols.Femto")

namespace {
constexpr unsigned FEMTO_RESPONSE_TIMEOUT = 200;
}

const QLoggingCategory& FemtoProtocol::logCategory() const
{
    return FemtoProtocolLog();
}

bool FemtoProtocol::writeAckedCommandFemto(const char* command, const char* reply)
{
    GPSRawAckMatcher matcher(reply, "<ERROR");
    const GPSConfigurationStep step{command, std::chrono::milliseconds(FEMTO_RESPONSE_TIMEOUT)};
    return transact(step, command, matcher).evidence.outcome == GPSCommandOutcome::Acknowledged;
}

bool FemtoProtocol::configure(unsigned& baudrate, const GPSConfig& config)
{
    _configure_done = false;
    resetIOError();
    _surveyClock.reset();
    _correction_output_activated = false;
    _rtcmActivationPending = false;
    _rtcm_parsing.reset();
    decodeInit();
    if (!validateConfiguration(config)) {
        return false;
    }
    _baseConfig = config.base;
    constexpr unsigned supportedBaudrate = 115200;
    bool success = false;

    if (baudrate == 0 || baudrate == supportedBaudrate) {
        setBaudrate(supportedBaudrate);
        for (int run = 0; run < 2; ++run) {
            if (writeAckedCommandFemto("UNLOGALL THISPORT\r\n", "<UNLOGALL OK") &&
                writeAckedCommandFemto("VERSION\r\n", "<VERSION OK")) {
                success = true;
                break;
            }
        }
    }

    if (!success) {
        return false;
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

    return !hasIOError();
}

void FemtoProtocol::activateCorrectionOutput()
{
    if (_correction_output_activated) {
        return;
    }
    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        if (!writeAckedCommandFemto("POSAVE ON \r\n", "<POSAVE OK") ||
            !writeAckedCommandFemto("LOG GPGGA 1 \r\n", "<LOG OK")) {
            controlFailed();
            return;
        }
        _surveyClock.start(nowUs());
        publishSurvey(true, false, _surveyClock.duration());
        return;
    }
    const auto& settings = std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode);
    char buffer[100];
    const int length =
        snprintf(buffer, sizeof(buffer), "FIX POSITION %.8lf %.8lf %.5f\r\n", settings.position.latitudeDegrees,
                 settings.position.longitudeDegrees, double(settings.position.altitudeMeters));
    if (length < 0 || length >= int(sizeof(buffer)) || !writeAckedCommandFemto(buffer, "FIX OK") ||
        !writeAckedCommandFemto("LOG GPGGA 1 \r\n", "<LOG OK")) {
        controlFailed();
        return;
    }
    activateRTCMOutput();
    if (_correction_output_activated) {
        publishSurvey(false, true, {}, settings.position);
    }
}

void FemtoProtocol::activateRTCMOutput()
{
    if (!writeAckedCommandFemto("LOG RTCM 1\r\n", "<LOG OK")) {
        controlFailed();
        return;
    }
    _correction_output_activated = true;
}
