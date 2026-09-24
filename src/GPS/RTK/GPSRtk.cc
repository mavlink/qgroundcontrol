#include "GPSRtk.h"

#include "AutoConnectSettings.h"
#include "GPSBaseStationConfig.h"
#include "GPSCorrectionManager.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiverConfig.h"
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

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.RTK.GPSRtk")

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
    ++_generation;
    _retireSession();

    qCDebug(GPSRtkLog) << this;
}

void GPSRtk::_onGPSConnect(const QString& identity)
{
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = ++_generation;
    _setError(GPSConnectionError::None);
    if (!guard || _generation != generation) {
        return;
    }
    if (std::exchange(_session.identity, identity) != identity) {
        qCDebug(GPSRtkLog) << "Receiver identity:" << identity;
        emit receiverChanged();
        if (!guard || _generation != generation) {
            return;
        }
    }
    _publishFacts({{_gpsRtkFactGroup->connected(), true}}, generation);
}

bool GPSRtk::_publishFacts(std::initializer_list<std::pair<Fact*, QVariant>> updates, quint64 generation)
{
    const QPointer<GPSRtk> guard(this);
    const auto facts = _gpsRtkFactGroup;
    for (const auto& [fact, value] : updates) {
        fact->setRawValue(value);
        if (!guard || _generation != generation) {
            return false;
        }
    }
    return true;
}

bool GPSRtk::_publishDisconnected(quint64 generation)
{
    const QPointer<GPSRtk> guard(this);
    const auto reset = [](Fact* fact) { return std::pair<Fact*, QVariant>(fact, fact->rawDefaultValue()); };
    auto& facts = *_gpsRtkFactGroup;
    if (!_publishFacts({reset(facts.connected()), reset(facts.valid()), reset(facts.active()),
                        reset(facts.currentDuration()), reset(facts.currentAccuracy()), reset(facts.currentLatitude()),
                        reset(facts.currentLongitude()), reset(facts.currentAltitude()), reset(facts.numSatellites()),
                        reset(facts.numSatellitesUsed()), reset(facts.fixType())},
                       generation)) {
        return false;
    }
    emit receiverChanged();
    return guard && _generation == generation;
}

void GPSRtk::_onGPSConnectionError(GPSConnectionError error, const QString& detail)
{
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSRtkLog) << "Failed to open GPS receiver transport";
            _setError(error, tr("Failed to open the receiver. Check the device, permissions, and other connections."));
            break;
        case GPSConnectionError::ConfigFailed:
            qCWarning(GPSRtkLog) << "GPS receiver did not accept configuration";
            _setError(error,
                      detail.isEmpty()
                          ? tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode.")
                          : tr("Receiver configuration failed: %1").arg(detail));
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
    _connectionError = error;
    if (std::exchange(_errorMessage, message) != message) {
        emit errorMessageChanged();
    }
}

void GPSRtk::_onGPSSurveyReport(const GPSSurveyReport& status)
{
    if (_session.manufacturer == manufacturerForType(GPSType::passive)) {
        return;
    }
    const quint64 generation = ++_generation;
    _publishFacts({{_gpsRtkFactGroup->currentDuration(), static_cast<qint64>(status.duration.count())},
                   {_gpsRtkFactGroup->currentAccuracy(), status.meanAccuracyMeters.value_or(qQNaN())},
                   {_gpsRtkFactGroup->currentLatitude(), status.position.latitudeDegrees},
                   {_gpsRtkFactGroup->currentLongitude(), status.position.longitudeDegrees},
                   {_gpsRtkFactGroup->currentAltitude(), status.position.altitudeMeters},
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
    ++_generation;
    QObject::disconnect(_portEnumerationConnection);
    _serialPorts = serialPorts;
    if (_serialPorts) {
        _portEnumerationConnection =
            connect(_serialPorts, &SerialPortManager::portsEnumerated, this, [this](const QStringList& availablePorts) {
                if (!_session.serialDevice.isEmpty() && !availablePorts.contains(_session.serialDevice)) {
                    const QPointer<GPSRtk> guard(this);
                    const quint64 generation = _generation + 1;
                    disconnectGPS();
                    if (guard && _generation == generation) {
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
    ++_generation;
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
    ++_generation;
#ifndef QGC_NO_SERIAL_LINK
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = _generation;
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
    if (!guard || _generation != generation) {
        return false;
    }
    const auto port = std::find_if(ports.cbegin(), ports.cend(),
                                   [&device](const auto& candidate) { return candidate.systemLocation == device; });
    if (device.isEmpty() || port == ports.cend() || port->bootloader) {
        _setError(GPSConnectionError::OpenFailed, tr("Select an available serial device that is not a bootloader."));
        return false;
    }
    // Zero asks configurable receivers to detect the rate.
    const auto baud = settings->serialBaudRate()->rawValue().toULongLong();
    if ((baud != 0 && (baud < 1200 || baud > 4000000)) || (baud == 0 && *type == GPSType::passive)) {
        _setError(GPSConnectionError::ConfigFailed,
                  gpsReceiverConfigErrorText(GPSReceiverConfigError::InvalidBaudRate));
        return false;
    }
    emit manualConnectionRequested();
    if (!guard || _generation != generation) {
        return false;
    }
    SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->setRawValue(false);
    if (!guard || _generation != generation) {
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
    const quint64 generation = ++_generation;
    emit manualConnectionRequested();
    if (!guard || _generation != generation) {
        return;
    }
    SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->setRawValue(false);
    if (!guard || _generation != generation) {
        return;
    }
    _retireSession();
    if (guard && _generation == generation && _publishDisconnected(generation)) {
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
    ++_generation;
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
            config.base.mode = GPSBaseStationConfig::Fixed{
                .position = {.latitudeDegrees = settings->fixedBasePositionLatitude()->rawValue().toDouble(),
                             .longitudeDegrees = settings->fixedBasePositionLongitude()->rawValue().toDouble(),
                             .altitudeMeters = settings->fixedBasePositionAltitude()->rawValue().toFloat()},
                .accuracyMeters = settings->fixedBasePositionAccuracy()->rawValue().toFloat(),
            };
            break;
        case BaseModeDefinition::Mode::BaseSurveyIn:
            config.base.mode = GPSBaseStationConfig::SurveyIn{
                .accuracyMeters = settings->surveyInAccuracyLimit()->rawValue().toDouble(),
                .durationSecs = settings->surveyInMinObservationDuration()->rawValue().toLongLong(),
            };
            break;
        case BaseModeDefinition::Mode::BaseReceiverAveraging:
            config.base.mode = GPSBaseStationConfig::ReceiverAveraging{
                .maximumDurationSecs = settings->receiverAveragingDuration()->rawValue().toUInt()};
            break;
        default:
            return tr("Select a supported base mode.");
    }
    // The option is hidden for receivers that cannot send MSM4, so it never blocks their connection.
    config.base.compactObservations = settings->compactRtcmCorrections()->rawValue().toBool() &&
                                      gpsReceiverCapabilities(type, config.role).compactObservations;
    return gpsReceiverConfigError(type, config);
}

bool GPSRtk::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                             const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges)
{
    if (_destroying) {
        return false;
    }
    ++_generation;
    return _connectReceiver(type, std::move(transportFactory), sourceInstance, baudRate, allowPersistentChanges);
}

bool GPSRtk::_connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                              const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                              const QString& serialDevice)
{
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = _generation;
    const auto currentOperation = [guard, generation]() {
        return guard && !guard->_destroying && guard->_generation == generation;
    };
    RTKSettings* const settings = SettingsManager::instance()->rtkSettings();
    GPSReceiverConfig config;
    const QString configError = _receiverConfig(type, settings, baudRate, config, allowPersistentChanges);
    if (!configError.isEmpty()) {
        _setError(GPSConnectionError::ConfigFailed, configError);
        return false;
    }
    _retireSession();
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
    _session.manufacturer = manufacturerForType(type);
    _session.baseMode = config.role == GPSReceiverConfig::Role::Passive ? -1
                        : std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode)
                            ? static_cast<int>(BaseModeDefinition::Mode::BaseFixed)
                        : std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode)
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
            correctionManager->acceptIngress(
                token.event(data, receivedAtMs, RTCMFramer::frameMessageId(data), valid, false,
                            valid ? GPSCorrectionReason::None : GPSCorrectionReason::InvalidFrame));
        },
        Qt::QueuedConnection);
    (void) connectCurrent(&GPSProvider::satelliteInfoUpdate, std::bind_front(&GPSRtk::_satelliteInfoUpdate, this));
    (void) connectCurrent(&GPSProvider::fixTypeChanged, std::bind_front(&GPSRtk::_fixTypeChanged, this));
    (void) connectCurrent(&GPSProvider::surveyInStatus, std::bind_front(&GPSRtk::_onGPSSurveyReport, this));
    (void) connectCurrent(
        &GPSProvider::connectionError, [this, guard](GPSConnectionError error, const QString& detail) {
            const quint64 retiredGeneration = ++_generation;
            _retireSession();
            if (guard && _generation == retiredGeneration && _publishDisconnected(retiredGeneration)) {
                _onGPSConnectionError(error, detail);
            }
        });
    (void) connectCurrent(&GPSProvider::receiverReady, std::bind_front(&GPSRtk::_onGPSConnect, this));
    _session.started = true;
    provider->start();
    emit receiverChanged();
    return currentOperation() && provider && _session.provider == provider;
}

void GPSRtk::_retireSession()
{
    auto retired = std::move(_session);
    _session = {};
    const auto provider = retired.provider;
    if (provider) {
        // Retirement callbacks may delete this owner; the worker must already be independent.
        provider->setParent(nullptr);
        provider->stop();
    }
    retired.corrections.reset();
    if (provider && !retired.started) {
        delete provider.data();
    }
    // Started workers own their transport reservation until run() exits; finished() schedules deletion.
}

void GPSRtk::disconnectGPS()
{
    if (_destroying) {
        return;
    }
    const QPointer<GPSRtk> guard(this);
    const quint64 generation = ++_generation;
    _retireSession();
    if (guard && _generation == generation) {
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

void GPSRtk::_satelliteInfoUpdate(const GPSSatelliteReport& msg)
{
    const quint64 generation = ++_generation;
    const int inView = msg.inView.value_or(-1);
    const int used = msg.used.value_or(-1);
    qCDebug(GPSRtkLog) << QStringLiteral("%1 in view, %2 used")
                              .arg(inView)
                              .arg(msg.used ? QString::number(used) : QStringLiteral("unknown"));
    _publishFacts({{_gpsRtkFactGroup->numSatellites(), inView}, {_gpsRtkFactGroup->numSatellitesUsed(), used}},
                  generation);
}

void GPSRtk::_fixTypeChanged(GPSPositionReport::FixType fixType)
{
    qCDebug(GPSRtkLog) << "Receiver fix changed:" << static_cast<int>(fixType);
    const quint64 generation = ++_generation;
    _publishFacts({{_gpsRtkFactGroup->fixType(), static_cast<int>(fixType)}}, generation);
}
