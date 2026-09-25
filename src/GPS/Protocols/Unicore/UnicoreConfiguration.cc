#include <iomanip>
#include <locale>
#include <sstream>

#include "UnicoreProtocol.h"

bool UnicoreProtocol::_execute(std::string command, Reply reply)
{
    _command = {.text = std::move(command), .expected = reply};
    _configurationDetail.clear();
    const auto wire = _command.text + "\r\n";
    const auto result = transact({_command.text, std::chrono::milliseconds(COMMAND_TIMEOUT_MS)}, wire);
    if (result.evidence.writtenBytes < static_cast<int>(wire.size())) {
        _configurationDetail =
            QStringLiteral("Unicore command '%1' could not be written").arg(QString::fromStdString(_command.text));
        return false;
    }
    if (result.evidence.outcome != GPSCommandOutcome::Acknowledged &&
        result.evidence.outcome != GPSCommandOutcome::ReadbackVerified) {
        if (result.evidence.outcome != GPSCommandOutcome::Cancelled) {
            if (_configurationDetail.isEmpty()) {
                const auto failure = result.evidence.outcome == GPSCommandOutcome::TimedOut
                                         ? QStringLiteral("timed out")
                                     : result.evidence.outcome == GPSCommandOutcome::Rejected
                                         ? QStringLiteral("was rejected or its readback did not match")
                                         : QStringLiteral("failed");
                _configurationDetail =
                    QStringLiteral("Unicore command '%1' %2").arg(QString::fromStdString(_command.text), failure);
            }
            log(GPSProtocolLogLevel::Warning, "Unicore command failed (%d): %s",
                static_cast<int>(result.evidence.outcome), _command.text.c_str());
        }
        return false;
    }
    return true;
}

bool UnicoreProtocol::_identify(unsigned& baud)
{
    constexpr std::array<unsigned, 8> BAUD_RATES{115200, 230400, 460800, 921600, 57600, 38400, 19200, 9600};
    const auto speedFailure = [](unsigned rate) {
        return QStringLiteral("Cannot configure Unicore host serial speed %1").arg(rate);
    };
    const auto detection = detectBaud(BAUD_RATES, baud, [this, &speedFailure](unsigned rate) {
        resetStream();
        _configurationDetail = speedFailure(rate);
        if (_execute("VERSIONA", Reply::Version)) {
            return BaudProbe::Found;
        }
        // A receiver that reported its model answered at this rate.
        return _model.empty() ? BaudProbe::TryNext : BaudProbe::Stop;
    });
    if (!detection.found) {
        if (detection.linkFailed) {
            _configurationDetail = speedFailure(detection.baud);
        }
        return false;
    }
    baud = detection.baud;
    log(GPSProtocolLogLevel::Debug, "Unicore %s firmware %s", _model.c_str(), _firmware.c_str());
    return true;
}

bool UnicoreProtocol::configure(unsigned& baud, const GPSConfig& config)
{
    const bool wasBase = _base;
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    _lastBaseEpoch.reset();
    _base = true;
    _averaging = !std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode);
    setRTCMEnabled(false);
    resetIOError();
    resetStream();
    _model.clear();
    _firmware.clear();
    _configurationDetail.clear();
    if (wasBase || _base) {
        _publishBase(false, false);
        consume({});
    }

    if (!validateConfiguration(config, {.receiverAveraging = true})) {
        return _configurationFailed(
            QStringLiteral("Invalid Unicore receiver configuration: check the role, base position and survey settings; "
                           "persistent changes are not supported"));
    }
    if (_averaging && !std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode)) {
        log(GPSProtocolLogLevel::Warning,
            "Unicore supports receiver-managed averaging, not accuracy-controlled survey");
        return _configurationFailed(
            QStringLiteral("Unicore requires receiver-managed averaging with a duration between 1 and 3600 seconds"));
    }
    _baseConfig = config.base;
    if (!_averaging) {
        _fixedECEF = toEcef(std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position);
    }
    const Operation operation(*this, 45000);
    if (!_identify(baud) || !_execute("UNLOG")) {
        return _configurationFailed();
    }
    // Force a new role transition even when reconnecting to a receiver left in base mode.
    _expectedMode = Mode::Rover;
    if (!_execute("MODE ROVER") || !_execute("MODE", Reply::Mode)) {
        return _configurationFailed();
    }
    for (const auto command : {"GPGGA 1", "GPGST 1", "GPGSV 1", "GPGSA 1"}) {
        if (!_execute(command)) {
            return _configurationFailed();
        }
    }
    std::ostringstream mode;
    mode.imbue(std::locale::classic());
    if (_averaging) {
        // Distance=0 forces newly averaged coordinates; it is NOT an accuracy threshold.
        mode << "MODE BASE TIME "
             << std::get<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode).maximumDurationSecs << " 0";
        _expectedMode = Mode::AveragingBase;
    } else {
        mode << std::fixed << std::setprecision(4) << "MODE BASE " << _fixedECEF.x << ' ' << _fixedECEF.y << ' '
             << _fixedECEF.z;
        _expectedMode = Mode::FixedBase;
    }
    if (!_execute(mode.str()) || !_execute("MODE", Reply::Mode)) {
        return _configurationFailed();
    }
    _monitorBase = true;
    _publishBase(false, _averaging);
    // Read back before subscribing, so queued periodic output cannot satisfy the query.
    if (!_averaging && !_execute("BESTNAVXYZA", Reply::FixedPosition)) {
        return _configurationFailed();
    }
    if (!_execute("BESTNAVXYZA 1")) {
        return _configurationFailed();
    }
    for (const auto command : {"RTCM1005 1", "RTCM1033 10", "RTCM1074 1", "RTCM1084 1", "RTCM1094 1", "RTCM1124 1"}) {
        if (!_execute(command)) {
            return _configurationFailed();
        }
    }
    _ready = true;
    setRTCMEnabled(_baseValid);
    return true;
}

bool UnicoreProtocol::_configurationFailed(const QString& reason)
{
    if (!reason.isEmpty()) {
        _configurationDetail = reason;
    }
    if (_monitorBase) {
        _publishBase(false, false);
    }
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    setRTCMEnabled(false);
    consume({});
    if (ioError() != GPSProtocolError::Cancelled && ioErrorDetail().isEmpty()) {
        _ioErrorDetail = _configurationDetail;
    }
    return false;
}
