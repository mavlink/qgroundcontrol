#include "GPSRtk.h"

#include "AutoConnectSettings.h"
#include "GPSBaseStationConfig.h"
#include "GPSCorrectionManager.h"
#include "GPSDriver.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiverConfigValidation.h"
#include "GPSReceiverDescriptor.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#include "SerialPortManager.h"
#endif

#include <algorithm>
#include <functional>
#include <utility>

#include <QtCore/QPointer>

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.GPSRtk")

GPSRtk::GPSRtk(QObject* parent)
    : QObject(parent)
    , _gpsRtkFactGroup(std::make_shared<GPSRTKFactGroup>())
{
    qCDebug(GPSRtkLog) << this;
#ifndef QGC_NO_SERIAL_LINK
    _serialTransportFactory = [](const QString& device, const std::atomic_bool& stop) {
        return std::make_unique<SerialGPSTransport>(device, stop);
    };
    setSerialPortManager(SerialPortManager::instance());
#endif
}

GPSRtk::~GPSRtk()
{
    _destroying = true;
    _retireSession(++_session.generation);

    qCDebug(GPSRtkLog) << this;
}

void GPSRtk::_onGPSConnect()
{
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = ++_session.generation;
    _setError(GPSConnectionError::None);
    if (guard && _session.generation == generation) {
        _publishFacts({{_gpsRtkFactGroup->connected(), true}}, generation);
    }
}

void GPSRtk::_onGPSDisconnect()
{
    disconnectGPS();
}

bool GPSRtk::_publishFacts(std::initializer_list<std::pair<Fact*, QVariant>> updates, quint64 generation)
{
    const QPointer<GPSRtk> guard(this);
    const auto facts = _gpsRtkFactGroup;
    for (const auto& [fact, value] : updates) {
        fact->setRawValue(value);
        if (!guard || _session.generation != generation) {
            return false;
        }
    }
    return true;
}

bool GPSRtk::_publishDisconnected(quint64 generation)
{
    const QPointer<GPSRtk> guard(this);
    if (!_publishFacts({{_gpsRtkFactGroup->connected(), false},
                        {_gpsRtkFactGroup->valid(), false},
                        {_gpsRtkFactGroup->active(), false},
                        {_gpsRtkFactGroup->currentDuration(), 0},
                        {_gpsRtkFactGroup->currentAccuracy(), qQNaN()},
                        {_gpsRtkFactGroup->currentLatitude(), qQNaN()},
                        {_gpsRtkFactGroup->currentLongitude(), qQNaN()},
                        {_gpsRtkFactGroup->currentAltitude(), qQNaN()},
                        {_gpsRtkFactGroup->numSatellites(), -1},
                        {_gpsRtkFactGroup->numSatellitesUsed(), -1}},
                       generation)) {
        return false;
    }
    emit receiverChanged();
    return guard && _session.generation == generation;
}

void GPSRtk::_onGPSConnectionError(GPSConnectionError error)
{
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSRtkLog) << "Failed to open GPS receiver transport";
            _setError(error, tr("Failed to open the receiver. Check the device, permissions, and other connections."));
            break;
        case GPSConnectionError::ConfigFailed:
            qCWarning(GPSRtkLog) << "GPS receiver did not accept configuration";
            if (_gpsRtkFactGroup->lastError()->rawValue().toInt() != static_cast<int>(error) ||
                _errorMessage.isEmpty()) {
                _setError(error,
                          tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode."));
            }
            break;
        case GPSConnectionError::DeviceError:
            qCWarning(GPSRtkLog) << "GPS device error, connection lost";
            _setError(error, tr("Receiver connection lost. Check the device and reconnect."));
            break;
        case GPSConnectionError::None:
            _setError(error);
            break;
    }
}

void GPSRtk::_setError(GPSConnectionError error, const QString& message)
{
    const quint64 generation = _session.generation;
    const bool changed = _errorMessage != message;
    _errorMessage = message;
    if (_publishFacts({{_gpsRtkFactGroup->lastError(), static_cast<int>(error)}}, generation) && changed) {
        emit errorMessageChanged();
    }
}

void GPSRtk::_onGPSSurveyInStatus(const GPSSurveyInStatus& status)
{
    if (_session.manufacturer == manufacturerForType(GPSType::passive)) {
        return;
    }
    const quint64 generation = ++_session.generation;
    _publishFacts({{_gpsRtkFactGroup->currentDuration(), static_cast<qint64>(status.duration.count())},
                   {_gpsRtkFactGroup->currentAccuracy(), status.meanAccuracyMeters.value_or(qQNaN())},
                   {_gpsRtkFactGroup->currentLatitude(), status.coordinate.latitude()},
                   {_gpsRtkFactGroup->currentLongitude(), status.coordinate.longitude()},
                   {_gpsRtkFactGroup->currentAltitude(), status.altitudeEllipsoidMeters},
                   {_gpsRtkFactGroup->valid(), status.valid},
                   {_gpsRtkFactGroup->active(), status.active}},
                  generation);
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtk::setSerialPortManager(SerialPortManager* serialPorts)
{
    if (_destroying || hasReceiver()) {
        return;
    }
    ++_session.generation;
    QObject::disconnect(_portEnumerationConnection);
    _serialPorts = serialPorts;
    if (_serialPorts) {
        _portEnumerationConnection =
            connect(_serialPorts, &SerialPortManager::portsEnumerated, this, [this](const QStringList& availablePorts) {
                if (!_session.serialDevice.isEmpty() && !availablePorts.contains(_session.serialDevice)) {
                    const QPointer<GPSRtk> guard(this);
                    const quint64 generation = _session.generation + 1;
                    disconnectGPS();
                    if (guard && _session.generation == generation) {
                        _setError(GPSConnectionError::DeviceError,
                                  tr("Receiver unplugged. Select a device and reconnect."));
                    }
                }
            });
    }
}

bool GPSRtk::connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate, bool allowPersistentChanges)
{
    if (_destroying) {
        return false;
    }
    ++_session.generation;
    for (const auto& entry : gpsReceiverDescriptors()) {
        if (gps_type.contains(QLatin1StringView(entry.detectionKey.data(), entry.detectionKey.size()),
                              Qt::CaseInsensitive)) {
            return _connectSerialGPS(device, entry.type, baudRate, allowPersistentChanges);
        }
    }
    _setError(GPSConnectionError::ConfigFailed, tr("Select a specific receiver type before connecting."));
    return false;
}

bool GPSRtk::_connectSerialGPS(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges)
{
    if (!GPSDriver::supportsType(type)) {
        _setError(GPSConnectionError::ConfigFailed, tr("The selected receiver type is unavailable in this build."));
        return false;
    }
    const QString endpoint = device.trimmed();
    if (endpoint.isEmpty() || !_serialPorts) {
        _setError(GPSConnectionError::OpenFailed, tr("Select an available serial device."));
        return false;
    }
    auto reservation = _serialPorts->reservePort(endpoint);
    if (!reservation) {
        _setError(GPSConnectionError::OpenFailed, tr("The selected serial device is already in use."));
        return false;
    }
    return _connectReceiver(
        type,
        [endpoint, reservation, factory = _serialTransportFactory](const std::atomic_bool& requestStop) {
            return factory(endpoint, requestStop);
        },
        QStringLiteral("serial:%1").arg(endpoint), baudRate, allowPersistentChanges, endpoint);
}
#endif

bool GPSRtk::serialSupported() const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    return true;
#endif
}

std::optional<GPSType> GPSRtk::typeForManufacturer(int manufacturer)
{
    if (const auto* descriptor = gpsReceiverDescriptorForManufacturer(manufacturer)) {
        return descriptor->type;
    }
    return std::nullopt;
}

int GPSRtk::manufacturerForType(GPSType type)
{
    const auto* descriptor = gpsReceiverDescriptor(type);
    return descriptor ? descriptor->manufacturerId : 0;
}

QVariantMap GPSRtk::capabilitiesForManufacturer(int manufacturer) const
{
    return gpsReceiverPresentation(manufacturer);
}

bool GPSRtk::connectConfiguredGPS(bool allowPersistentChanges)
{
    if (_destroying) {
        return false;
    }
    ++_session.generation;
#ifndef QGC_NO_SERIAL_LINK
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = _session.generation;
    auto* settings = SettingsManager::instance()->rtkSettings();
    const auto type = typeForManufacturer(settings->baseReceiverManufacturers()->rawValue().toInt());
    if (!type) {
        _setError(GPSConnectionError::ConfigFailed, tr("Select a specific receiver type before connecting."));
        return false;
    }
    if (hasReceiver()) {
        _setError(GPSConnectionError::OpenFailed, tr("Disconnect the current receiver before connecting another."));
        return false;
    }
    const QString device = settings->serialDevice()->rawValue().toString().trimmed();
    const auto ports = _serialPorts ? _serialPorts->availablePorts() : QList<SerialPortManager::Port>{};
    if (!guard || _session.generation != generation) {
        return false;
    }
    const auto port = std::find_if(ports.cbegin(), ports.cend(),
                                   [&device](const auto& candidate) { return candidate.systemLocation == device; });
    if (device.isEmpty() || port == ports.cend() || port->bootloader) {
        _setError(GPSConnectionError::OpenFailed, tr("Select an available serial device that is not a bootloader."));
        return false;
    }
    const auto baud = settings->serialBaudRate()->rawValue().toULongLong();
    if (baud < 1200 || baud > 4000000) {
        _setError(GPSConnectionError::ConfigFailed, tr("Select a baud rate between 1200 and 4000000."));
        return false;
    }
    emit manualConnectionRequested();
    if (!guard || _session.generation != generation) {
        return false;
    }
    SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->setRawValue(false);
    if (!guard || _session.generation != generation) {
        return false;
    }
    return _connectSerialGPS(device, *type, static_cast<uint32_t>(baud), allowPersistentChanges);
#else
    Q_UNUSED(allowPersistentChanges);
    _setError(GPSConnectionError::OpenFailed, tr("Serial receiver connections are unavailable in this build."));
    return false;
#endif
}

void GPSRtk::disconnectConfiguredGPS()
{
    if (_destroying) {
        return;
    }
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = ++_session.generation;
    emit manualConnectionRequested();
    if (!guard || _session.generation != generation) {
        return;
    }
    SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->setRawValue(false);
    if (!guard || _session.generation != generation) {
        return;
    }
    _retireSession(generation);
    if (guard && _session.generation == generation && _publishDisconnected(generation)) {
        _setError(GPSConnectionError::None);
    }
}

void GPSRtk::setCorrectionManager(GPSCorrectionManager* manager)
{
    if (_destroying || _correctionManager == manager) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSRtkLog) << "Inject the correction manager before connecting a receiver";
        return;
    }
    ++_session.generation;
    _correctionManager = manager;
}

QString GPSRtk::_receiverConfig(GPSType type, RTKSettings* settings, uint32_t baudRate, GPSReceiverConfig& config,
                                bool allowPersistentChanges)
{
    config = GPSReceiverConfig{.baudRate = baudRate, .allowPersistentChanges = allowPersistentChanges};
    if (type == GPSType::passive) {
        config.role = GPSReceiverConfig::Role::Passive;
        return gpsReceiverConfigError(type, config);
    }
    switch (static_cast<BaseModeDefinition::Mode>(settings->useFixedBasePosition()->rawValue().toInt())) {
        case BaseModeDefinition::Mode::BaseFixed:
            config.base = GPSBaseStationConfig{
                .useFixedBase = true,
                .fixedPosition = {.latitudeDegrees = settings->fixedBasePositionLatitude()->rawValue().toDouble(),
                                  .longitudeDegrees = settings->fixedBasePositionLongitude()->rawValue().toDouble(),
                                  .altitudeMeters = settings->fixedBasePositionAltitude()->rawValue().toFloat()},
                .fixedBaseAccuracyMeters = settings->fixedBasePositionAccuracy()->rawValue().toFloat(),
            };
            break;
        case BaseModeDefinition::Mode::BaseSurveyIn:
            config.base = GPSBaseStationConfig{
                .surveyInAccMeters = settings->surveyInAccuracyLimit()->rawValue().toDouble(),
                .surveyInDurationSecs = settings->surveyInMinObservationDuration()->rawValue().toLongLong(),
            };
            break;
        case BaseModeDefinition::Mode::BaseReceiverAveraging:
            config.base.surveyMode = GPSBaseStationConfig::SurveyMode::ReceiverManaged;
            config.base.receiverAveragingDurationSecs = settings->receiverAveragingDuration()->rawValue().toUInt();
            break;
        default:
            return tr("Select a supported base mode.");
    }
    return gpsReceiverConfigError(type, config);
}

bool GPSRtk::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                             const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges)
{
    if (_destroying) {
        return false;
    }
    ++_session.generation;
    return _connectReceiver(type, std::move(transportFactory), sourceInstance, baudRate, allowPersistentChanges);
}

bool GPSRtk::_connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                              const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                              const QString& serialDevice)
{
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = _session.generation;
    const auto currentOperation = [guard, generation]() {
        return guard && !guard->_destroying && guard->_session.generation == generation;
    };
    if (!GPSDriver::supportsType(type)) {
        _setError(GPSConnectionError::ConfigFailed, tr("The selected receiver type is unavailable in this build."));
        return false;
    }
    RTKSettings* const settings = SettingsManager::instance()->rtkSettings();
    GPSReceiverConfig config;
    const QString configError = _receiverConfig(type, settings, baudRate, config, allowPersistentChanges);
    if (!configError.isEmpty()) {
        _setError(GPSConnectionError::ConfigFailed, configError);
        return false;
    }
    _retireSession(generation);
    if (!currentOperation() || !_publishDisconnected(generation)) {
        return false;
    }
    settings->baseReceiverManufacturers()->setRawValue(manufacturerForType(type));
    if (!currentOperation()) {
        return false;
    }
    _setError(GPSConnectionError::None);
    if (!currentOperation()) {
        return false;
    }
    const QPointer<GPSCorrectionManager> correctionManager = _correctionManager;
    GPSCorrectionSourceRegistration registration;
    if (correctionManager) {
        registration = correctionManager->registerSource(GPSCorrectionSource::LocalReceiver, sourceInstance);
        if (!currentOperation()) {
            return false;
        }
    }
    _session.corrections = std::move(registration);
    _session.configuration = config;
    _session.manufacturer = manufacturerForType(type);
    _session.baseMode = config.role == GPSReceiverConfig::Role::Passive ? -1
                        : config.base.useFixedBase ? static_cast<int>(BaseModeDefinition::Mode::BaseFixed)
                        : config.base.surveyMode == GPSBaseStationConfig::SurveyMode::ReceiverManaged
                            ? static_cast<int>(BaseModeDefinition::Mode::BaseReceiverAveraging)
                            : static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn);
    _session.serialDevice = serialDevice;
    _session.provider = new GPSProvider(std::move(transportFactory), type, config, this);
    const QPointer<GPSProvider> provider = _session.provider;
    (void) connect(
        provider, &QThread::finished, this,
        [this, provider]() {
            if (provider && _session.provider == provider) {
                disconnectGPS();
            }
        },
        Qt::QueuedConnection);
    (void) connect(provider, &QThread::finished, provider, &QObject::deleteLater);
    const auto token = _session.corrections.token();
    const auto current = [guard, provider, token, registered = !correctionManager.isNull()]() {
        return guard && provider && guard->_session.provider == provider && guard->_session.manufacturer != 0 &&
               (!registered || token.valid());
    };
    const auto connectCurrent = [this, provider, current]<typename... Args>(void (GPSProvider::*signal)(Args...),
                                                                            auto handler) {
        return connect(
            provider, signal, this,
            [current, handler](Args... args) {
                if (current()) {
                    handler(args...);
                }
            },
            Qt::QueuedConnection);
    };
    // Queued callbacks retain the producing session's token.
    (void) connect(
        provider, &GPSProvider::RTCMDataUpdate, this,
        [correctionManager, token](const QByteArray& data, qint64 receivedAtMs) {
            if (!correctionManager) {
                qCWarning(GPSRtkLog) << "Correction manager not ready; dropping" << data.size() << "bytes";
                return;
            }
            const bool valid = RTCMFramer::isValidFrame(data);
            const int messageId =
                data.size() >= 5 ? (static_cast<quint8>(data[3]) << 4) | (static_cast<quint8>(data[4]) >> 4) : 0;
            correctionManager->acceptIngress(
                token.event(data, receivedAtMs, messageId, valid, false,
                            valid ? GPSCorrectionReason::None : GPSCorrectionReason::InvalidFrame));
        },
        Qt::QueuedConnection);
    (void) connectCurrent(&GPSProvider::satelliteInfoUpdate, std::bind_front(&GPSRtk::_satelliteInfoUpdate, this));
    (void) connectCurrent(&GPSProvider::satelliteUsageUpdate, std::bind_front(&GPSRtk::_satelliteUsageUpdate, this));
    (void) connectCurrent(&GPSProvider::fixTypeChanged, std::bind_front(&GPSRtk::_fixTypeChanged, this));
    (void) connectCurrent(&GPSProvider::surveyInStatus, std::bind_front(&GPSRtk::_onGPSSurveyInStatus, this));
    (void) connectCurrent(&GPSProvider::configurationError, [this](const QString& detail) {
        if (!detail.isEmpty()) {
            _setError(GPSConnectionError::ConfigFailed, tr("Receiver configuration failed: %1").arg(detail));
        }
    });
    (void) connectCurrent(&GPSProvider::connectionError, [this, guard](GPSConnectionError error) {
        const quint64 retiredGeneration = ++_session.generation;
        _retireSession(retiredGeneration);
        if (guard && _session.generation == retiredGeneration && _publishDisconnected(retiredGeneration)) {
            _onGPSConnectionError(error);
        }
    });
    (void) connectCurrent(&GPSProvider::receiverReady, std::bind_front(&GPSRtk::_onGPSConnect, this));
    _session.started = true;
    provider->start();
    emit receiverChanged();
    return currentOperation() && provider && _session.provider == provider;
}

void GPSRtk::_retireSession(quint64 generation)
{
    auto retired = std::move(_session);
    _session = {};
    _session.generation = generation;
    const auto provider = retired.provider;
    const auto timeoutMs = _disconnectTimeoutMs;
    if (provider) {
        // Retirement callbacks may delete this owner; the worker must already be independent.
        provider->setParent(nullptr);
        provider->stop();
    }
    retired.corrections.reset();
    if (provider) {
        if (!retired.started) {
            delete provider.data();
        } else if (!provider->wait(timeoutMs)) {
            qCWarning(GPSRtkLog) << "GPS thread did not exit in time; deferring cleanup to finished()";
        } else {
            delete provider.data();
        }
    }
}

void GPSRtk::disconnectGPS()
{
    if (_destroying) {
        return;
    }
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = ++_session.generation;
    _retireSession(generation);
    if (guard && _session.generation == generation) {
        _publishDisconnected(generation);
    }
}

bool GPSRtk::connected() const
{
    return _gpsRtkFactGroup->connected()->rawValue().toBool();
}

GPSRTKFactGroup* GPSRtk::gpsRtkFactGroup()
{
    return _gpsRtkFactGroup.get();
}

GPSRtk::SatelliteCounts GPSRtk::countSatellites(const GPSSatelliteReport& msg)
{
    SatelliteCounts counts;
    if (msg.timestampUs == 0) {
        return counts;
    }
    counts.inView = (std::min) (msg.count, GPSSatelliteReport::MAX_SATELLITES);
    if (msg.count > GPSSatelliteReport::MAX_SATELLITES) {
        return counts;
    }
    counts.used = 0;
    for (int i = 0; i < counts.inView; ++i) {
        if (!msg.satellites[i].used) {
            counts.used.reset();
            break;
        }
        if (*msg.satellites[i].used) {
            ++*counts.used;
        }
    }
    return counts;
}

void GPSRtk::_satelliteInfoUpdate(const GPSSatelliteReport& msg)
{
    const quint64 generation = ++_session.generation;
    const SatelliteCounts counts = countSatellites(msg);
    qCDebug(GPSRtkLog) << QStringLiteral("%1 in view, %2 used")
                              .arg(counts.inView)
                              .arg(counts.used ? QString::number(*counts.used) : QStringLiteral("unknown"));
    const int used = counts.used.value_or(_session.countOnlySatelliteUsage.value_or(-1));
    _publishFacts({{_gpsRtkFactGroup->numSatellites(), counts.inView}, {_gpsRtkFactGroup->numSatellitesUsed(), used}},
                  generation);
}

void GPSRtk::_fixTypeChanged(GPSPositionReport::FixType fixType)
{
    qCDebug(GPSRtkLog) << "Receiver fix changed:" << static_cast<int>(fixType);
}

void GPSRtk::_satelliteUsageUpdate(const GPSSatelliteUsageReport& msg)
{
    // A count-only observation cannot change the independently reported satellites in view.
    _session.countOnlySatelliteUsage = msg.usedCount;
    _publishFacts({{_gpsRtkFactGroup->numSatellitesUsed(), msg.usedCount.value_or(-1)}}, ++_session.generation);
}
