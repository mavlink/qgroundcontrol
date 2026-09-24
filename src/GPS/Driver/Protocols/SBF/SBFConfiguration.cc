#include <cmath>
#include <cstddef>
#include <string.h>

#include "GPSRawAckMatcher.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"
#include "SBF/GPSDriverSBF.h"

QGC_LOGGING_CATEGORY(GPSNativeSBFLog, "GPS.Driver.Protocols.SBF")

namespace {
constexpr int SBF_CONFIG_TIMEOUT = 1000;
constexpr size_t MSG_SIZE = 100;
}  // namespace

const QLoggingCategory& GPSNativeSBF::logCategory() const
{
    return GPSNativeSBFLog();
}

bool GPSNativeSBF::configure(unsigned& baudrate, const GPSConfig& config)
{
    _configured = false;
    resetIOError();
    _surveyClock.reset();
    _survey_active = false;
    if (!validateConfiguration(config)) {
        return false;
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

    // Make sure we can send commands to the receiver
    sendMessage(SBF_CONFIG_FORCE_INPUT);

    // Disable previous output for now so we can detect the COM port
    for (int i = 1; i <= 2; i++) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_DISABLE_OUTPUT, "COM", i);
        sendMessageAndWaitForAck(msg, false);
    }

    for (int i = 1; i <= 4; i++) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_DISABLE_OUTPUT, "USB", i);
        sendMessageAndWaitForAck(msg, false);
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
            return false;
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

    if (!response_detected) {
        log(GPSProtocolLogLevel::Warning, "No COM port detected");
        return false;
    }
    log(GPSProtocolLogLevel::Debug, "Septentrio GNSS receiver COM port: %s", com_port);

    // Delete all sbf outputs on current COM port to remove clutter data
    snprintf(msg, sizeof(msg), SBF_CONFIG_RESET, com_port);

    if (!sendMessageAndWaitForAck(msg)) {
        return false;  // connection and/or baudrate detection failed
    }

    // Only serial COM ports have a baud rate; USB and IP connection descriptors do not.
    if (strncmp(com_port, "COM", 3) == 0) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_BAUDRATE, com_port, baudrate);

        if (!sendMessageAndWaitForAck(msg)) {
            return false;  // connection and/or baudrate detection failed
        }
    }

    // At this point we have correct baudrate on both ends

    // Define/inquire the type of data that the receiver should accept/send on a given connection descriptor
    snprintf(msg, sizeof(msg), SBF_DATA_IO, com_port);

    if (!sendMessageAndWaitForAck(msg)) {
        return false;
    }
    std::string outputConfirmation = msg;

    // Septentrio's WGS84/Default selects the global datum except when external corrections supply a datum.
    if (!sendMessageAndWaitForAck("setGeodeticDatum, WGS84\n")) {
        return false;
    }

    // Preserve the receiver's historical second application after the first required ACK.
    // Navigation confirms its SBF stream; base mode confirms the selected input/output port.
    constexpr unsigned OUTPUT_CONFIRMATION_ATTEMPTS = 5;
    bool outputConfirmed = false;
    for (unsigned attempt = 0; attempt < OUTPUT_CONFIRMATION_ATTEMPTS && !hasIOError(); ++attempt) {
        if (sendMessageAndWaitForAck(outputConfirmation.c_str())) {
            outputConfirmed = true;
            break;
        }
    }
    if (!outputConfirmed) {
        return false;
    }

    if (!_rtcm_parsing) {
        _rtcm_parsing.emplace();
    }
    _rtcm_parsing->reset();

    snprintf(msg, sizeof(msg), SBF_CONFIG_OUTPUT_RTCM3, com_port);
    if (!sendMessageAndWaitForAck(msg)) {
        return false;
    }

    if (std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_COORDINATES,
                 std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode).position.latitudeDegrees,
                 std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode).position.longitudeDegrees,
                 static_cast<double>(std::get<GPSBaseStationConfig::Fixed>(_baseConfig.mode).position.altitudeMeters));
        if (!sendMessageAndWaitForAck(msg)) {
            return false;
        }

        snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATIC_OFFSET, 0.0, 0.0, 0.0);
        if (!sendMessageAndWaitForAck(msg)) {
            return false;
        }

        if (!sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC1)) {
            return false;
        }
        if (!sendMessageAndWaitForAck(SBF_CONFIG_RTCM_STATIC2)) {
            return false;
        }
    } else {
        if (!sendMessageAndWaitForAck(SBF_CONFIG_RTCM_SURVEY_IN)) {
            return false;
        }
    }

    snprintf(msg, sizeof(msg), SBF_CONFIG_RTCM_STATUS, com_port);
    if (!sendMessageAndWaitForAck(msg)) {
        return false;
    }
    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        _surveyClock.start(nowUs());
    }

    _configured = !hasIOError();
    return _configured;
}

bool GPSNativeSBF::sendMessage(const char* msg)
{
    return write(msg, static_cast<int>(strlen(msg)));
}

bool GPSNativeSBF::sendMessageAndWaitForAck(const char* msg, bool required)
{
    std::string_view command(msg);
    while (command.ends_with('\r') || command.ends_with('\n')) {
        command.remove_suffix(1);
    }
    const std::string expected = "$R: " + std::string(command);
    GPSRawAckMatcher matcher(expected, "$R?");
    const GPSConfigurationStep step{msg, std::chrono::milliseconds(SBF_CONFIG_TIMEOUT), {}, required};
    return transact(step, msg, matcher).evidence.outcome == GPSCommandOutcome::Acknowledged;
}
