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
    bool positionUpdated = false;
    bool satellitesUpdated = false;
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
                     GPSExecutionContext clock)
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
    if ((_transport.isCancelled() || _clock.cancelled())) {
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
        _private->writeFailure || !capabilityError.isEmpty() || (_transport.isCancelled() || _clock.cancelled())) {
        if ((_transport.isCancelled() || _clock.cancelled()) ||
            _private->driver->ioError() == GPSProtocol::ReadCancelled) {
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
    _private->sensorGps = {};
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
    return !(_transport.isCancelled() || _clock.cancelled()) && !_transport.fatalError() && _private->driver &&
           !_private->driver->ioError() && _configurationResult.status == ConfigurationStatus::Ready &&
           _config.role == GPSReceiverConfig::Role::Position &&
           _capabilities.correctionInput == GPSReceiverCapabilities::Support::Supported &&
           _private->driver->receiverReady();
}

GPSDriver::CorrectionResult GPSDriver::injectCorrections(const QByteArray& data, QDeadlineTimer deadline)
{
    return injectCorrections(
        data, GPSDeadline{deadline.isForever()
                              ? UINT64_MAX
                              : _clock.nowUs() + uint64_t(std::max<qint64>(0, deadline.remainingTime())) * 1000});
}

GPSDriver::CorrectionResult GPSDriver::injectCorrections(const QByteArray& data, GPSDeadline deadline)
{
    if ((_transport.isCancelled() || _clock.cancelled())) {
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
    const auto budget = _transport.correctionWriteTimeout(static_cast<int>(data.size()));
    deadline.untilUs = std::min(deadline.untilUs, _clock.nowUs() + uint64_t(budget.count()) * 1000);
    const auto result = _transport.writeUntil(reinterpret_cast<const uint8_t*>(data.constData()),
                                              static_cast<int>(data.size()), deadline, _clock);
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
    if ((_transport.isCancelled() || _clock.cancelled()) ||
        (_private->driver && _private->driver->ioError() == GPSProtocol::ReadCancelled)) {
        return {ReceiveStatus::Cancelled, false, false, _private->readFailure};
    }
    if (!_private->driver) {
        return {};
    }
    _private->positionUpdated = false;
    _private->satellitesUpdated = false;
    const int result = _private->driver->receive(timeoutMs);
    if ((_transport.isCancelled() || _clock.cancelled()) || _private->driver->ioError() == GPSProtocol::ReadCancelled) {
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
    return {ReceiveStatus::Data, _private->positionUpdated, _private->satellitesUpdated};
}

GPSProtocolIO GPSDriver::_protocolIO()
{
    GPSProtocolIO io;
    io.log = [](GPSProtocolLogLevel level, std::string_view message) {
        const auto text = QString::fromUtf8(message.data(), message.size());
        switch (level) {
            case GPSProtocolLogLevel::Debug:
                qCDebug(GPSDriversLog).noquote() << text;
                break;
            case GPSProtocolLogLevel::Warning:
                qCWarning(GPSDriversLog).noquote() << text;
                break;
            case GPSProtocolLogLevel::Error:
                qCCritical(GPSDriversLog).noquote() << text;
                break;
        }
    };
    io.nowUs = _clock.nowUs;
    io.wait = [this](std::chrono::microseconds duration) {
        while (duration.count() > 0 && !(_transport.isCancelled() || _clock.cancelled())) {
            const auto slice = std::min(duration, std::chrono::microseconds(50000));
            _clock.wait(slice);
            duration -= slice;
        }
        return !(_transport.isCancelled() || _clock.cancelled());
    };
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSProtocolReadResult {
        if ((_transport.isCancelled() || _clock.cancelled()))
            return {GPSReadStatus::Cancelled};
        if (_private->readFailure || _private->writeFailure)
            return {GPSReadStatus::Error};
        const auto result = _transport.readUntil(bytes.data(), static_cast<int>(bytes.size()), deadline, _clock);
        if ((_transport.isCancelled() || _clock.cancelled()) || result.status == GPSReadStatus::Cancelled) {
            _private->readFailure = GPSReadResult{GPSReadStatus::Cancelled, 0, result.detail};
            return {GPSReadStatus::Cancelled};
        }
        if (result.status != GPSReadStatus::Data && result.status != GPSReadStatus::TimedOut)
            _private->readFailure = result;
        return {result.status, result.bytesRead};
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) -> GPSProtocolWriteResult {
        if ((_transport.isCancelled() || _clock.cancelled()))
            return {GPSWriteStatus::Cancelled};
        if (_private->readFailure || _private->writeFailure)
            return {GPSWriteStatus::Error};
        const auto budget = std::min(std::chrono::milliseconds(deadline.remainingMilliseconds(_clock.nowUs())),
                                     _transport.configurationWriteTimeout());
        deadline.untilUs = std::min(deadline.untilUs, _clock.nowUs() + uint64_t(budget.count()) * 1000);
        const auto result = _transport.writeUntil(bytes.data(), static_cast<int>(bytes.size()), deadline, _clock);
        if (result.status != GPSWriteStatus::Completed || result.writtenBytes != static_cast<int>(bytes.size()))
            _private->writeFailure = result;
        return {result.status, result.acceptedBytes, result.writtenBytes, result.uncertainBytes};
    };
    io.setBaudrate = [this](unsigned baudrate) {
        if ((_transport.isCancelled() || _clock.cancelled()))
            return GPSBaudStatus::Cancelled;
        if (_private->readFailure || _private->writeFailure || _transport.fatalError())
            return GPSBaudStatus::Error;
        return _transport.setBaudrate(baudrate) ? GPSBaudStatus::Configured
               : _transport.fatalError()        ? GPSBaudStatus::Error
                                                : GPSBaudStatus::Unsupported;
    };
    const auto rtcm = [this](std::span<const uint8_t> bytes) {
        if (_config.role == GPSReceiverConfig::Role::RTKBase && _sinks.onRTCM) {
            _sinks.onRTCM(QByteArray(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        }
    };
    const auto relative = [this](const GPSRelativeReport& report) {
        if (_sinks.onRelativePosition)
            _sinks.onRelativePosition(GPSDriverData::relativePosition(report, _clock));
    };
    const auto survey = [this](const GPSSurveyReport& status) {
        if (_config.role != GPSReceiverConfig::Role::RTKBase || !_sinks.onSurveyIn)
            return;
        _sinks.onSurveyIn(GPSDriverData::survey(status, _clock));
    };
    io.decoded = [this, rtcm, relative, survey](GPSDecodedBatch batch) {
        for (const auto& event : batch.events) {
            std::visit(
                [&](const auto& report) {
                    using Report = std::decay_t<decltype(report)>;
                    if constexpr (std::is_same_v<Report, GPSPositionReport>) {
                        _private->positionUpdated = true;
                        if (_sinks.onPosition)
                            _sinks.onPosition(GPSDriverData::position(report, _clock));
                    } else if constexpr (std::is_same_v<Report, GPSSatelliteReport> ||
                                         std::is_same_v<Report, GPSSatelliteUsageReport>) {
                        _private->satellitesUpdated = true;
                        if (_sinks.onSatelliteInfo)
                            _sinks.onSatelliteInfo(GPSDriverData::satellites(report, _clock));
                    } else if constexpr (std::is_same_v<Report, GPSRTCMReport>) {
                        rtcm({report.bytes.data(), report.size});
                    } else if constexpr (std::is_same_v<Report, GPSRelativeReport>) {
                        relative(report);
                    } else if constexpr (std::is_same_v<Report, GPSSurveyReport>) {
                        survey(report);
                    }
                },
                event);
        }
    };
    return io;
}
