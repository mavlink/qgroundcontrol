#include "GPSDriver.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

#include "GPSDriverBackend.h"
#include "GPSDriverData.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.Driver.GPSDriver")
QGC_LOGGING_CATEGORY(GPSDriversLog, "GPS.Driver.Drivers")

struct GPSDriver::Private
{
    std::unique_ptr<GPSDriverBackend> driver;
    GPSPositionReport sensorGps{};
    GPSSatelliteReport satelliteInfo{};
    std::optional<GPSReadResult> readFailure;
    std::optional<GPSWriteResult> writeFailure;
};

namespace {
GPSConfigurationReport requestedSettings(const GPSReceiverConfig& config, const GPSReceiverCapabilities& capabilities)
{
    GPSConfigurationReport report;
    for (const auto& descriptor : capabilities.settings(config.role == GPSReceiverConfig::Role::RTKBase)) {
        const double value = GPSReceiverSettings::value(descriptor.id, config);
        if (descriptor.support == GPSReceiverCapabilities::Support::Unsupported && value == descriptor.defaultValue) {
            continue;
        }
        GPSSettingReport setting;
        setting.id = descriptor.id;
        setting.label = descriptor.label;
        setting.units = descriptor.units;
        setting.requestedValue = value;
        report.settings.append(setting);
    }
    return report;
}

void rejectUnsupportedSettings(GPSConfigurationReport& report, const GPSReceiverConfig& config,
                               const GPSReceiverCapabilities& capabilities)
{
    const auto descriptors = capabilities.settings(config.role == GPSReceiverConfig::Role::RTKBase);
    for (auto& setting : report.settings) {
        for (const auto& descriptor : descriptors) {
            if (setting.id == descriptor.id && !descriptor.accepts(setting.requestedValue.toDouble())) {
                setting.requestState = GPSSettingReport::RequestState::Rejected;
                setting.detail = QCoreApplication::translate(
                    "GPSDriver", "This setting is unsupported in the selected receiver mode");
            }
        }
    }
}
}  // namespace

GPSDriver::GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks,
                     GPSDriverClock clock)
    : _clock(std::move(clock))
    , _type(type)
    , _transport(transport)
    , _config(config)
    , _sinks(std::move(sinks))
    , _capabilities(GPSReceiverCapabilities::forType(type))
    , _private(std::make_unique<Private>())
{
    qCDebug(GPSDriverLog) << this;
    if (!_clock.nowUs)
        _clock.nowUs = GPSObservation::monotonicNowUs;
    if (!_clock.wait)
        _clock.wait = [](std::chrono::microseconds duration) { QThread::usleep(duration.count()); };
    GPSDriverData::initialize(_private->sensorGps);
    _configurationReport = requestedSettings(_config, _capabilities);
}

GPSDriver::~GPSDriver()
{
    qCDebug(GPSDriverLog) << this;
}

bool GPSDriver::configure()
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSDriver", text); };
    _baudrate = 0;
    _private->driver.reset();
    _private->readFailure.reset();
    _private->writeFailure.reset();
    _capabilities = GPSReceiverCapabilities::forType(_type);
    _configurationResult = {};
    _configurationReport = requestedSettings(_config, _capabilities);
    if (_transport.isCancelled()) {
        _configurationResult.status = ConfigurationStatus::Cancelled;
        return false;
    }
    const QString validationError = _capabilities.validationError(_config);
    if (!validationError.isEmpty()) {
        _configurationResult = {ConfigurationStatus::Unsupported, validationError};
        rejectUnsupportedSettings(_configurationReport, _config, _capabilities);
        if (!_capabilities.recognized()) {
            qCWarning(GPSDriverLog) << "Unsupported GPS type:" << static_cast<int>(_type);
        } else if (_config.role != GPSReceiverConfig::Role::RTKBase &&
                   _config.role != GPSReceiverConfig::Role::Position) {
            qCWarning(GPSDriverLog) << "Unsupported receiver role:" << static_cast<int>(_config.role);
        } else if (_config.outputProtocol != GPSReceiverConfig::OutputProtocol::Native) {
            qCWarning(GPSDriverLog) << "Unsupported receiver output protocol";
        }
        return false;
    }
    const QString configError = _config.validationError();
    if (!configError.isEmpty()) {
        _configurationResult = {ConfigurationStatus::Unsupported, configError};
        return false;
    }
    const auto* family = gpsDriverFamily(_type);
    _private->driver = family->create(_protocolIO(), &_private->sensorGps, &_private->satelliteInfo, _config);
    unsigned baudrate = _transport.fixedBaudrate();
    const int result = _private->driver->configure(baudrate, _config);

    _updateCapabilities();
    const QString capabilityError = _capabilities.validationError(_config);
    rejectUnsupportedSettings(_configurationReport, _config, _capabilities);
    _private->driver->completeConfigurationReport(_config, result == 0 && capabilityError.isEmpty(),
                                                  _configurationReport);
    if (result != 0 || _transport.fatalError() || _private->driver->ioError() || _private->readFailure ||
        _private->writeFailure || !capabilityError.isEmpty() || _transport.isCancelled()) {
        if (_transport.isCancelled() || _private->driver->ioError() == GPSProtocol::ReadCancelled) {
            _configurationResult = {ConfigurationStatus::Cancelled, {}};
        } else if (_transport.fatalError() || _private->driver->ioError() || _private->readFailure ||
                   _private->writeFailure) {
            QString detail;
            if (_private->readFailure) {
                detail = _private->readFailure->detail;
            } else if (_private->writeFailure) {
                detail = _private->writeFailure->detail;
                if (detail.isEmpty() && _private->writeFailure->status == GPSWriteStatus::TimedOut) {
                    detail = tr("Receiver configuration write timed out");
                }
            }
            _configurationResult = {ConfigurationStatus::TransportError,
                                    detail.isEmpty() ? tr("Receiver transport failed") : detail, _private->readFailure,
                                    _private->writeFailure};
        } else if (!capabilityError.isEmpty()) {
            _configurationResult = {ConfigurationStatus::Unsupported, capabilityError};
        } else {
            const QString detail = _private->driver->configurationError();
            _configurationResult = {ConfigurationStatus::Failed,
                                    detail.isEmpty() ? tr("Receiver configuration failed") : detail};
        }
        _configurationResult.transportRead = _private->readFailure;
        _configurationResult.transportWrite = _private->writeFailure;
        if (_configurationResult.status != ConfigurationStatus::Cancelled) {
            qCWarning(GPSDriverLog) << "Driver configuration failed for type" << static_cast<int>(_type);
        }
        _private->driver.reset();
        return false;
    }

    _baudrate = baudrate;
    _configurationResult.status = ConfigurationStatus::Ready;
    GPSDriverData::initialize(_private->sensorGps);
    return true;
}

void GPSDriver::_updateCapabilities()
{
    if (_private->driver) {
        _private->driver->updateCapabilities(_capabilities);
    }
}

bool GPSDriver::readyForCorrections() const
{
    return !_transport.isCancelled() && !_transport.fatalError() && _private->driver && !_private->driver->ioError() &&
           _configurationResult.status == ConfigurationStatus::Ready &&
           _config.role == GPSReceiverConfig::Role::Position &&
           _capabilities.correctionInput == GPSReceiverCapabilities::Support::Supported &&
           _private->driver->receiverReady();
}

GPSDriver::CorrectionResult GPSDriver::injectCorrections(const QByteArray& data, QDeadlineTimer deadline)
{
    if (_transport.isCancelled()) {
        return {CorrectionStatus::Cancelled};
    }
    if (data.isEmpty() || data.size() > 1029) {
        return {CorrectionStatus::InvalidData};
    }
    if (_config.role != GPSReceiverConfig::Role::Position ||
        _capabilities.correctionInput != GPSReceiverCapabilities::Support::Supported) {
        return {CorrectionStatus::Unsupported};
    }
    if (_transport.fatalError() || (_private->driver && _private->driver->ioError())) {
        return {CorrectionStatus::TransportError};
    }
    if (!readyForCorrections()) {
        return {CorrectionStatus::NotReady};
    }
    const QDeadlineTimer transportDeadline(_transport.correctionWriteTimeout(static_cast<int>(data.size())));
    if (deadline.remainingTime() < 0 || transportDeadline.remainingTime() < deadline.remainingTime()) {
        deadline = transportDeadline;
    }
    const auto result = _transport.writeBounded(reinterpret_cast<const uint8_t*>(data.constData()),
                                                static_cast<int>(data.size()), deadline);
    CorrectionStatus status = CorrectionStatus::TransportError;
    switch (result.status) {
        case GPSTransport::WriteStatus::Completed:
            status =
                result.writtenBytes == data.size() ? CorrectionStatus::Submitted : CorrectionStatus::TransportError;
            break;
        case GPSTransport::WriteStatus::Cancelled:
            status = CorrectionStatus::Cancelled;
            break;
        case GPSTransport::WriteStatus::Unsupported:
            status = CorrectionStatus::Unsupported;
            break;
        case GPSTransport::WriteStatus::InvalidData:
            status = CorrectionStatus::InvalidData;
            break;
        case GPSTransport::WriteStatus::TimedOut:
        case GPSTransport::WriteStatus::Error:
            break;
    }
    return {status, result.writtenBytes, result.acceptedBytes, result.uncertainBytes};
}

GPSDriver::ReceiveResult GPSDriver::receiveResult(unsigned timeoutMs)
{
    if (_transport.isCancelled() || (_private->driver && _private->driver->ioError() == GPSProtocol::ReadCancelled)) {
        return {ReceiveStatus::Cancelled, false, false, _private->readFailure};
    }
    if (!_private->driver) {
        return {};
    }
    const int result = _private->driver->receive(timeoutMs);
    if (_transport.isCancelled() || _private->driver->ioError() == GPSProtocol::ReadCancelled) {
        return {ReceiveStatus::Cancelled, false, false, _private->readFailure};
    }
    if (_transport.fatalError() || _private->driver->ioError() || _private->readFailure) {
        return {ReceiveStatus::DeviceError, false, false, _private->readFailure};
    }
    // A native receive timeout also returns -1; the callback's recorded I/O
    // result distinguishes it from a device error.
    if (result <= 0) {
        return {ReceiveStatus::Idle};
    }
    if ((result & 1) && _sinks.onPosition)
        _sinks.onPosition(GPSDriverData::position(_private->sensorGps));
    if ((result & 2) && _sinks.onSatelliteInfo)
        _sinks.onSatelliteInfo(GPSDriverData::satellites(_private->satelliteInfo));
    return {ReceiveStatus::Data, (result & 1) != 0, (result & 2) != 0};
}

GPSProtocolIO GPSDriver::_protocolIO()
{
    GPSProtocolIO io;
    io.nowUs = _clock.nowUs;
    io.wait = [this](std::chrono::microseconds duration) {
        while (duration.count() > 0 && !_transport.isCancelled()) {
            const auto slice = std::min(duration, std::chrono::microseconds(50000));
            _clock.wait(slice);
            duration -= slice;
        }
        return !_transport.isCancelled();
    };
    io.read = [this](std::span<uint8_t> bytes, GPSProtocolDeadline deadline) -> GPSProtocolReadResult {
        if (_transport.isCancelled())
            return {GPSReadStatus::Cancelled};
        if (_private->readFailure || _private->writeFailure)
            return {GPSReadStatus::Error};
        const auto result = _transport.read(bytes.data(), static_cast<int>(bytes.size()),
                                            deadline.remainingMilliseconds(_clock.nowUs()));
        if (_transport.isCancelled() || result.status == GPSReadStatus::Cancelled) {
            _private->readFailure = GPSReadResult{GPSReadStatus::Cancelled, 0, result.detail};
            return {GPSReadStatus::Cancelled};
        }
        if (result.status != GPSReadStatus::Data && result.status != GPSReadStatus::TimedOut)
            _private->readFailure = result;
        return {result.status, result.bytesRead};
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSProtocolDeadline deadline) -> GPSProtocolWriteResult {
        if (_transport.isCancelled())
            return {GPSWriteStatus::Cancelled};
        if (_private->readFailure || _private->writeFailure)
            return {GPSWriteStatus::Error};
        const auto budget = std::min(std::chrono::milliseconds(deadline.remainingMilliseconds(_clock.nowUs())),
                                     _transport.configurationWriteTimeout());
        const auto result =
            _transport.writeBounded(bytes.data(), static_cast<int>(bytes.size()), QDeadlineTimer(budget));
        if (result.status != GPSWriteStatus::Completed || result.writtenBytes != static_cast<int>(bytes.size()))
            _private->writeFailure = result;
        return {result.status, result.acceptedBytes, result.writtenBytes, result.uncertainBytes};
    };
    io.setBaudrate = [this](unsigned baudrate) {
        if (_transport.isCancelled())
            return GPSBaudStatus::Cancelled;
        if (_private->readFailure || _private->writeFailure || _transport.fatalError())
            return GPSBaudStatus::Error;
        return _transport.setBaudrate(baudrate) ? GPSBaudStatus::Configured
               : _transport.fatalError()        ? GPSBaudStatus::Error
                                                : GPSBaudStatus::Unsupported;
    };
    io.rtcm = [this](std::span<const uint8_t> bytes) {
        if (_config.role == GPSReceiverConfig::Role::RTKBase && _sinks.onRTCM) {
            _sinks.onRTCM(QByteArray(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        }
    };
    io.relativePosition = [this](const GPSRelativeReport& report) {
        if (_sinks.onRelativePosition)
            _sinks.onRelativePosition(GPSDriverData::relativePosition(report));
    };
    io.survey = [this](const GPSSurveyReport& status) {
        if (_config.role != GPSReceiverConfig::Role::RTKBase || !_sinks.onSurveyIn)
            return;
        GPSSurveyInStatus out;
        out.latitude = status.latitude;
        out.longitude = status.longitude;
        out.altitude = status.altitude;
        if (_type == GPSType::u_blox || _type == GPSType::septentrio || status.mean_accuracy != 0)
            out.meanAccuracyMM = status.mean_accuracy;
        if (_type == GPSType::u_blox || _type == GPSType::septentrio)
            out.altitudeDatum = GPSObservation::AltitudeDatum::Ellipsoid;
        out.monotonicTimestampUs = _clock.nowUs();
        out.durationSecs = status.duration;
        out.valid = status.flags & 1;
        out.active = status.flags & 2;
        _sinks.onSurveyIn(out);
    };
    return io;
}
