#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <string.h>

#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"
#include "SBF/SBFProtocol.h"

QGC_LOGGING_CATEGORY(SBFProtocolLog, "GPS.Driver.Protocols.SBF")

namespace {
constexpr int SBF_CONFIG_TIMEOUT = 1000;
constexpr size_t MSG_SIZE = 100;

std::string printed(const char* format, ...) Q_ATTRIBUTE_FORMAT_PRINTF(1, 2);

std::string printed(const char* format, ...)
{
    char message[MSG_SIZE];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    return message;
}

/// The receiver echoes an accepted command as "$R: <command>" and answers a rejected one with "$R?".
GPSConfigurationSequence::Command command(const std::string& message, bool required = true, unsigned attempts = 1)
{
    std::string_view echoed(message);
    while (echoed.ends_with('\r') || echoed.ends_with('\n')) {
        echoed.remove_suffix(1);
    }
    return {.step = {message, std::chrono::milliseconds(SBF_CONFIG_TIMEOUT), {}, required},
            .wire = message,
            .reply = GPSConfigurationSequence::RawReply{"$R: " + std::string(echoed), "$R?"},
            .attempts = attempts};
}
}  // namespace

const QLoggingCategory& SBFProtocol::logCategory() const
{
    return SBFProtocolLog();
}

bool SBFProtocol::configure(unsigned& baudrate, const GPSConfig& config)
{
    _configured = false;
    resetIOError();
    _surveyClock.reset();
    _survey_active = false;
    if (!validateConfiguration(config)) {
        return false;
    }
    _baseConfig = config.base;

    _epochs = {};
    _lastPublishedEpoch.reset();
    _rtcm_parsing.reset();
    decodeInit();

    setBaudrate(SBF_TX_CFG_PRT_BAUDRATE);
    baudrate = SBF_TX_CFG_PRT_BAUDRATE;

    // Make sure we can send commands to the receiver
    sendMessage(SBF_CONFIG_FORCE_INPUT);

    // Disable previous output for now so we can detect the COM port
    char com_port[5]{};
    GPSConfigurationSequence discovery;
    for (int i = 1; i <= 2; i++) {
        discovery.steps.emplace_back(command(printed(SBF_CONFIG_DISABLE_OUTPUT, "COM", i), false));
    }
    for (int i = 1; i <= 4; i++) {
        discovery.steps.emplace_back(command(printed(SBF_CONFIG_DISABLE_OUTPUT, "USB", i), false));
    }
    discovery.steps.emplace_back(
        GPSConfigurationSequence::Custom{"COM port detection", [this, &com_port] { return detectPort(com_port); }});
    if (!runSequence(discovery).succeeded()) {
        return false;
    }

    // Delete all SBF outputs on the current COM port to remove clutter data, then match its baud rate and
    // define the type of data that the receiver should accept and send on it.
    const std::string dataIO = printed(SBF_DATA_IO, com_port);
    GPSConfigurationSequence port{{command(printed(SBF_CONFIG_RESET, com_port))}};
    // Only serial COM ports have a baud rate; USB and IP connection descriptors do not.
    if (strncmp(com_port, "COM", 3) == 0) {
        port.steps.emplace_back(command(printed(SBF_CONFIG_BAUDRATE, com_port, baudrate)));
    }
    port.steps.emplace_back(command(dataIO));
    // Septentrio's WGS84/Default selects the global datum except when external corrections supply a datum.
    port.steps.emplace_back(command("setGeodeticDatum, WGS84\n"));
    // Preserve the receiver's historical second application after the first required ACK.
    // Navigation confirms its SBF stream; base mode confirms the selected input/output port.
    port.steps.emplace_back(command(dataIO, true, 5));
    if (!runSequence(port).succeeded()) {
        return false;
    }

    if (!_rtcm_parsing) {
        _rtcm_parsing.emplace();
    }
    _rtcm_parsing->reset();

    GPSConfigurationSequence base{{command(printed(SBF_CONFIG_OUTPUT_RTCM3, com_port))}};
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_baseConfig.mode)) {
        base.steps.emplace_back(
            command(printed(SBF_CONFIG_RTCM_STATIC_COORDINATES, fixed->position.latitudeDegrees,
                            fixed->position.longitudeDegrees, static_cast<double>(fixed->position.altitudeMeters))));
        base.steps.emplace_back(command(printed(SBF_CONFIG_RTCM_STATIC_OFFSET, 0.0, 0.0, 0.0)));
        base.steps.emplace_back(command(SBF_CONFIG_RTCM_STATIC1));
        base.steps.emplace_back(command(SBF_CONFIG_RTCM_STATIC2));
    } else {
        base.steps.emplace_back(command(SBF_CONFIG_RTCM_SURVEY_IN));
    }
    base.steps.emplace_back(command(printed(SBF_CONFIG_RTCM_STATUS, com_port)));
    if (!runSequence(base).succeeded()) {
        return false;
    }
    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        _surveyClock.start(nowUs());
    }

    _configured = !hasIOError();
    return _configured;
}

bool SBFProtocol::detectPort(char (&com_port)[5])
{
    char buf[GPS_READ_BUFFER_SIZE];
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
    return true;
}

bool SBFProtocol::sendMessage(const char* msg)
{
    return write(msg, static_cast<int>(strlen(msg)));
}
