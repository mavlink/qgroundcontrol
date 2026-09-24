#include "GPSRtk.h"

#include "AutoConnectSettings.h"
#include "GPSBaseStationConfig.h"
#include "GPSCorrectionManager.h"
#include "GPSPositionService.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSSourceHealth.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"
#include "RTKConnectionPolicy.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "TCPGPSTransport.h"

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
    , _positionHealth(new GPSSourceHealth(this))
{
    qCDebug(GPSRtkLog) << this;
    _connection = new RTKConnectionPolicy(this);
    connect(_connection, &RTKConnectionPolicy::reconnectingChanged, this, &GPSRtk::_notifyReceiverChanged);
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
    _notifications.close();
    // The policy observes settings and must not run against a partially destroyed receiver.
    delete std::exchange(_connection, nullptr);
    _retireSession();

    qCDebug(GPSRtkLog) << this;
}

void GPSRtk::_stageFact(Fact* fact, const QVariant& value)
{
    // The group stays alive while a setter unwinds after an observer deletes this receiver.
    _notifications.post(reinterpret_cast<quintptr>(fact),
                        [facts = _gpsRtkFactGroup, fact, value]() { fact->setRawValue(value); });
}

void GPSRtk::_stageDisconnectedFacts()
{
    auto& facts = *_gpsRtkFactGroup;
    for (Fact* fact :
         {facts.connected(), facts.valid(), facts.active(), facts.currentDuration(), facts.currentAccuracy(),
          facts.currentLatitude(), facts.currentLongitude(), facts.currentAltitude(), facts.numSatellites(),
          facts.numSatellitesUsed(), facts.fixType(), facts.jammingState(), facts.spoofingState()}) {
        _stageFact(fact, fact->rawDefaultValue());
    }
}

void GPSRtk::_notifyReceiverChanged()
{
    _notifications.emitSignal(this, &GPSRtk::receiverChanged);
}

void GPSRtk::_setError(GPSConnectionError error, const QString& message)
{
    _connectionError = error;
    if (std::exchange(_errorMessage, message) != message) {
        _notifications.emitSignal(this, &GPSRtk::errorMessageChanged);
    }
}

QGeoCoordinate GPSRtk::basePosition() const
{
    const auto position = _session.basePosition ? std::optional(_session.basePosition->first) : _session.surveyPosition;
    return position ? QGeoCoordinate(position->latitudeDegrees, position->longitudeDegrees) : QGeoCoordinate();
}

void GPSRtk::_onGPSConnect(const QString& identity)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _session.ready = true;
    if (std::exchange(_session.identity, identity) != identity) {
        qCDebug(GPSRtkLog) << "Receiver identity:" << identity;
    }
    _setError(GPSConnectionError::None);
    _connection->receiverReady();
    _stageFact(_gpsRtkFactGroup->connected(), true);
    _notifyReceiverChanged();
    if (_positionService && !_session.position) {
        const quint64 session = _session.id;
        auto registration = _positionService->registerPositionSource(GPSPositionService::SelectedSource::Receiver, this,
                                                                     _positionHealth, session);
        if (_session.id == session) {
            _session.position = std::move(registration);
        }
    }
}

void GPSRtk::_endSession(GPSConnectionError error, const QString& detail, bool portRemoved)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _retireSession();
    _stageDisconnectedFacts();
    _notifyReceiverChanged();
    const QString retryMessage = _connection->sessionEnded(error, detail, portRemoved);
    if (portRemoved) {
        qCDebug(GPSRtkLog) << "Receiver serial device removed";
        _setError(error,
                  retryMessage.isEmpty() ? tr("Receiver unplugged. Select a device and reconnect.") : retryMessage);
        return;
    }
    if (!retryMessage.isEmpty()) {
        qCWarning(GPSRtkLog) << "GPS receiver session ended:" << static_cast<int>(error) << detail;
        _setError(error, retryMessage);
        return;
    }
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSRtkLog) << "Failed to open GPS receiver transport";
            _setError(error, tr("Failed to open the receiver. Check the device or network address, permissions, and "
                                "other connections."));
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

void GPSRtk::_onGPSSurveyReport(const GPSSurveyReport& status)
{
    if (_session.manufacturer == manufacturerForType(GPSType::passive)) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    if (_session.baseMode != static_cast<int>(BaseModeDefinition::Mode::BaseFixed)) {
        const auto previous = basePosition();
        const bool wasFinal = basePositionFinal();
        const bool located = qIsFinite(status.position.latitudeDegrees) && qIsFinite(status.position.longitudeDegrees);
        const double accuracy =
            status.meanAccuracyMeters.value_or(_session.surveyAccuracyLimitMeters.value_or(qQNaN()));
        _session.surveyPosition = located ? std::optional(status.position) : std::nullopt;
        if (status.valid && located && qIsFinite(accuracy)) {
            _session.basePosition = std::pair(status.position, accuracy);
        } else {
            _session.basePosition.reset();
        }
        if (basePosition() != previous || basePositionFinal() != wasFinal) {
            _notifications.emitSignal(this, &GPSRtk::basePositionChanged);
        }
    }
    auto& facts = *_gpsRtkFactGroup;
    _stageFact(facts.currentDuration(), static_cast<qint64>(status.duration.count()));
    _stageFact(facts.currentAccuracy(), status.meanAccuracyMeters.value_or(qQNaN()));
    _stageFact(facts.currentLatitude(), status.position.latitudeDegrees);
    _stageFact(facts.currentLongitude(), status.position.longitudeDegrees);
    _stageFact(facts.currentAltitude(), status.position.altitudeMeters);
    _stageFact(facts.valid(), status.valid);
    _stageFact(facts.active(), status.active);
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtk::setSerialPortManager(SerialPortManager* serialPorts)
{
    if (_destroying || hasReceiver()) {
        return;
    }
    QObject::disconnect(_portEnumerationConnection);
    _serialPorts = serialPorts;
    if (_serialPorts) {
        _portEnumerationConnection =
            connect(_serialPorts, &SerialPortManager::portsEnumerated, this, [this](const QStringList& availablePorts) {
                if (!_session.serialDevice.isEmpty() && !availablePorts.contains(_session.serialDevice)) {
                    _endSession(GPSConnectionError::DeviceError, {}, true);
                }
            });
    }
}

bool GPSRtk::connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate, bool allowPersistentChanges)
{
    if (_destroying) {
        return false;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->reset();
    return _connectGPS(device, gps_type, baudRate, allowPersistentChanges);
}

bool GPSRtk::_connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate, bool allowPersistentChanges)
{
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
        QStringLiteral("serial:%1").arg(endpoint), baudRate, allowPersistentChanges, endpoint, endpoint);
}
#endif

bool GPSRtk::_connectTcpGPS(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges)
{
    const QString endpoint = QStringLiteral("%1:%2").arg(host).arg(port);
    // Bridges keep their own serial rate, so drivers use the transport's fixed rate.
    return _connectReceiver(
        type,
        [host, port](const std::atomic_bool& requestStop) {
            return std::make_unique<TCPGPSTransport>(host, port, requestStop);
        },
        QStringLiteral("tcp:%1").arg(endpoint), TCPGPSTransport::FIXED_BAUDRATE, allowPersistentChanges, {}, endpoint);
}

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
    const GPSNotificationQueue::Scope publish(_notifications);
    return _connection->connectConfigured(allowPersistentChanges);
}

void GPSRtk::disconnectConfiguredGPS()
{
    if (_destroying) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->disconnectConfigured();
}

bool GPSRtk::reconnecting() const
{
    return _connection && _connection->reconnecting();
}

void GPSRtk::setPositionService(GPSPositionService* service)
{
    if (_destroying || _positionService == service) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSRtkLog) << "Inject the position service before connecting a receiver";
        return;
    }
    _positionService = service;
}

std::optional<GPSObservation> GPSRtk::acceptedPositionObservation(GPSObservation::PositionUse use) const
{
    return _positionHealth->acceptedObservation(use);
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
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->reset();
    return _connectReceiver(type, std::move(transportFactory), sourceInstance, baudRate, allowPersistentChanges);
}

bool GPSRtk::_connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                              const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                              const QString& serialDevice, const QString& endpoint)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    RTKSettings* const settings = SettingsManager::instance()->rtkSettings();
    GPSReceiverConfig config;
    const QString configError = _receiverConfig(type, settings, baudRate, config, allowPersistentChanges);
    if (!configError.isEmpty()) {
        _setError(GPSConnectionError::ConfigFailed, configError);
        return false;
    }
    _retireSession();
    _stageDisconnectedFacts();
    _stageFact(settings->baseReceiverManufacturers(), manufacturerForType(type));
    _setError(GPSConnectionError::None);
    const QPointer<GPSCorrectionManager> correctionManager = _correctionManager;
    GPSCorrectionSourceRegistration registration;
    if (correctionManager) {
        registration = correctionManager->registerSource(GPSCorrectionSource::LocalReceiver, sourceInstance);
    }
    // A registration observer that connected another receiver is superseded by this request.
    _retireSession();
    _session.corrections = std::move(registration);
    _session.manufacturer = manufacturerForType(type);
    _session.baseMode = config.role == GPSReceiverConfig::Role::Passive ? -1
                        : std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode)
                            ? static_cast<int>(BaseModeDefinition::Mode::BaseFixed)
                        : std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode)
                            ? static_cast<int>(BaseModeDefinition::Mode::BaseReceiverAveraging)
                            : static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn);
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode)) {
        _session.basePosition = std::pair(fixed->position, static_cast<double>(fixed->accuracyMeters));
        _notifications.emitSignal(this, &GPSRtk::basePositionChanged);
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode);
               survey && config.role != GPSReceiverConfig::Role::Passive) {
        _session.surveyAccuracyLimitMeters = survey->accuracyMeters;
    }
    _session.serialDevice = serialDevice;
    _session.endpoint = endpoint;
    _session.id = ++_sessionCount;
    _session.provider = new GPSProvider(std::move(transportFactory), type, config, this);
    const QPointer<GPSProvider> provider = _session.provider;
    (void) connect(
        provider, &QThread::finished, this,
        [this, provider]() {
            if (provider && _session.provider == provider) {
                _endSession(GPSConnectionError::DeviceError, {}, false);
            }
        },
        Qt::QueuedConnection);
    (void) connect(provider, &QThread::finished, provider, &QObject::deleteLater);
    const auto token = _session.corrections.token();
    const auto current = [this, provider, token, registered = !correctionManager.isNull()]() {
        return provider && _session.provider == provider && (!registered || token.valid());
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
    (void) connectCurrent(&GPSProvider::positionUpdate, std::bind_front(&GPSRtk::_positionUpdate, this));
    (void) connectCurrent(&GPSProvider::surveyInStatus, std::bind_front(&GPSRtk::_onGPSSurveyReport, this));
    (void) connectCurrent(&GPSProvider::connectionError, [this](GPSConnectionError error, const QString& detail) {
        _endSession(error, detail, false);
    });
    (void) connectCurrent(&GPSProvider::receiverReady, std::bind_front(&GPSRtk::_onGPSConnect, this));
    _session.started = true;
    provider->start();
    _notifyReceiverChanged();
    return true;
}

void GPSRtk::_retireSession()
{
    auto retired = std::move(_session);
    _session = {};
    if (retired.basePosition || retired.surveyPosition) {
        _notifications.emitSignal(this, &GPSRtk::basePositionChanged);
    }
    const auto provider = retired.provider;
    if (provider) {
        // Retirement callbacks may delete this owner; the worker must already be independent.
        provider->setParent(nullptr);
        provider->stop();
    }
    retired.position.reset();
    retired.corrections.reset();
    _positionHealth->reset();
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
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->reset();
    _disconnect(false);
}

void GPSRtk::_disconnect(bool clearError)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _retireSession();
    _stageDisconnectedFacts();
    _notifyReceiverChanged();
    if (clearError) {
        _setError(GPSConnectionError::None);
    }
}

GPSRTKFactGroup* GPSRtk::gpsRtkFactGroup()
{
    return _gpsRtkFactGroup.get();
}

void GPSRtk::_satelliteInfoUpdate(const GPSSatelliteReport& msg)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const int inView = msg.inView.value_or(-1);
    const int used = msg.used.value_or(-1);
    qCDebug(GPSRtkLog) << QStringLiteral("%1 in view, %2 used")
                              .arg(inView)
                              .arg(msg.used ? QString::number(used) : QStringLiteral("unknown"));
    _stageFact(_gpsRtkFactGroup->numSatellites(), inView);
    _stageFact(_gpsRtkFactGroup->numSatellitesUsed(), used);
}

void GPSRtk::_positionUpdate(const GPSPositionReport& report)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const auto fixType = static_cast<int>(report.navigation.fixType);
    if (std::exchange(_session.fixType, fixType) != fixType) {
        qCDebug(GPSRtkLog) << "Receiver fix changed:" << fixType;
        _stageFact(_gpsRtkFactGroup->fixType(), fixType);
    }
    // Drivers project integrity to Unknown once it is older than its freshness window.
    _stageFact(_gpsRtkFactGroup->jammingState(), static_cast<int>(report.integrity.jamming.state));
    _stageFact(_gpsRtkFactGroup->spoofingState(), static_cast<int>(report.integrity.spoofing.state));
    const quint64 receivedAtUs = MonotonicClock::nowUs();
    auto observation = _session.basePosition
                           ? GPSObservation::fromSurveyedPosition(_session.basePosition->first,
                                                                  _session.basePosition->second, receivedAtUs)
                           : GPSObservation::fromNavigation(report.navigation, receivedAtUs);
    observation.sessionId = _session.id;
    _positionHealth->updateObservation(observation);
}
