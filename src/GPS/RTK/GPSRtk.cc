#include "GPSRtk.h"

#include "GPSBaseStationConfig.h"
#include "GPSCorrectionManager.h"
#include "GPSPositionService.h"
#include "GPSProvider.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSSourceHealth.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"
#include "RTKConnectionPolicy.h"
#include "TCPGPSTransport.h"
#include "UDPGPSTransport.h"

#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPorts.h"
#include "SerialGPSTransport.h"
#endif

#include <algorithm>
#include <functional>
#include <utility>

#include <QtCore/QPointer>

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.RTK.GPSRtk")

QDebug operator<<(QDebug debug, const GPSRtk::Configuration& configuration)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "GPSRtk::Configuration(receiverRole=" << configuration.receiverRole
                              << ", baseReceiverManufacturer=" << configuration.baseReceiverManufacturer
                              << ", connectionType=" << configuration.connectionType
                              << ", tcpHost=" << configuration.tcpHost << ", tcpPort=" << configuration.tcpPort
                              << ", udpPort=" << configuration.udpPort
                              << ", serialDevice=" << configuration.serialDevice
                              << ", serialBaudRate=" << configuration.serialBaudRate
                              << ", baseMode=" << configuration.baseMode
                              << ", surveyInAccuracyLimit=" << configuration.surveyInAccuracyLimit
                              << ", surveyInMinObservationDuration=" << configuration.surveyInMinObservationDuration
                              << ", receiverAveragingDuration=" << configuration.receiverAveragingDuration
                              << ", compactRtcmCorrections=" << configuration.compactRtcmCorrections
                              << ", autoConnect=" << configuration.autoConnect << ')';
    return debug;
}

GPSRtk::GPSRtk(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _positionHealth(new GPSSourceHealth(this, scheduler))
{
    qCDebug(GPSRtkLog) << this;
    // A silent receiver stays connected, so its last solution must not remain on display.
    connect(_positionHealth, &GPSSourceHealth::positionChanged, this, [this]() {
        if (_positionHealth->state() == GPSSourceHealth::State::Stale) {
            _clearStaleSolution();
        }
    });
    _connection = new RTKConnectionPolicy(*this, this, scheduler);
    connect(_connection, &RTKConnectionPolicy::reconnectingChanged, this, &GPSRtk::_notifyReceiverChanged);
    connect(_connection, &RTKConnectionPolicy::autoConnectDisabled, this, &GPSRtk::autoConnectDisabled);
    _providerFactory = [](GPSProvider::TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                          QObject* providerParent) {
        return new GPSProvider(std::move(transportFactory), type, config, providerParent);
    };
#ifndef QGC_NO_SERIAL_LINK
    _serialTransportFactory = [](const QString& device, const std::atomic_bool& stop) {
        return std::make_unique<SerialGPSTransport>(device, stop);
    };
#endif
}

void GPSRtk::setConfiguration(const Configuration& configuration)
{
    if (_configuration == configuration) {
        return;
    }
    _configuration = configuration;
    qCDebug(GPSRtkLog) << "RTK configuration applied:" << _configuration;
    _connection->setConfiguration(configuration);
}

void GPSRtk::setProviderFactory(ProviderFactory factory)
{
    if (_destroying) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSRtkLog) << "Inject the provider factory before connecting a receiver";
        return;
    }
    if (factory) {
        _providerFactory = std::move(factory);
        return;
    }
    _providerFactory = [](GPSProvider::TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                          QObject* parent) {
        return new GPSProvider(std::move(transportFactory), type, config, parent);
    };
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

void GPSRtk::_publishStatus()
{
    _notifications.emitSignal(this, &GPSRtk::statusChanged);
}

void GPSRtk::_resetStatus()
{
    _status = {};
    _publishStatus();
}

void GPSRtk::_clearStaleSolution()
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _session.fixType.reset();
    const Status none;
    _status.numSatellites = none.numSatellites;
    _status.numSatellitesUsed = none.numSatellitesUsed;
    _status.fixType = none.fixType;
    _status.jammingState = none.jammingState;
    _status.spoofingState = none.spoofingState;
    _publishStatus();
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

void GPSRtk::_onGPSConnect(const QString& identity)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _session.ready = true;
    if (std::exchange(_session.identity, identity) != identity) {
        qCDebug(GPSRtkLog) << "Receiver identity:" << identity;
    }
    _setError(GPSConnectionError::None);
    _connection->receiverReady();
    _status.connected = true;
    _publishStatus();
    _notifyReceiverChanged();
    if (_positionService && !_session.position) {
        const quint64 session = _session.id;
        auto registration = _positionService->registerPositionSource(GPSPositionService::SelectedSource::Receiver,
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
    _resetStatus();
    _notifyReceiverChanged();
    const RTKSessionOutcome outcome = _connection->sessionEnded(portRemoved);
    if (portRemoved) {
        qCDebug(GPSRtkLog) << "Receiver serial device removed";
    }
    switch (outcome) {
        case RTKSessionOutcome::Unplugged:
            _setError(error, tr("Receiver unplugged."));
            return;
        case RTKSessionOutcome::WaitingForPort:
            _setError(error, tr("Receiver unplugged. Reconnecting when it is plugged back in."));
            return;
        case RTKSessionOutcome::Retrying:
            qCWarning(GPSRtkLog) << "GPS receiver session ended:" << error << detail;
            _setError(error, error == GPSConnectionError::ConfigFailed && !detail.isEmpty()
                                 ? tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail)
                                 : tr("Receiver connection lost. Reconnecting automatically."));
            return;
        case RTKSessionOutcome::None:
            break;
    }
    if (portRemoved) {
        _setError(error, tr("Receiver unplugged. Select a device and reconnect."));
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
    if (_session.role != ConfiguredBase) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    if (_session.baseMode != 1) {
        const bool located = qIsFinite(status.position.latitudeDegrees) && qIsFinite(status.position.longitudeDegrees);
        const double accuracy =
            status.meanAccuracyMeters.value_or(_session.surveyAccuracyLimitMeters.value_or(qQNaN()));
        if (status.valid && located && qIsFinite(accuracy)) {
            _session.basePosition = std::pair(status.position, accuracy);
        } else {
            _session.basePosition.reset();
        }
    }
    _status.currentDuration = status.duration;
    _status.currentAccuracy = status.meanAccuracyMeters.value_or(qQNaN());
    _status.currentLatitude = status.position.latitudeDegrees;
    _status.currentLongitude = status.position.longitudeDegrees;
    _status.currentAltitude = status.position.altitudeMeters;
    _status.valid = status.valid;
    _status.active = status.active;
    _publishStatus();
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtk::setSerialPorts(GPSSerialPorts* serialPorts)
{
    if (_destroying || hasReceiver()) {
        return;
    }
    QObject::disconnect(_portEnumerationConnection);
    _serialPorts = serialPorts;
    if (_serialPorts) {
        _portEnumerationConnection =
            connect(_serialPorts, &GPSSerialPorts::portsEnumerated, this, [this](const QStringList& availablePorts) {
                if (!_session.serialDevice.isEmpty() && !availablePorts.contains(_session.serialDevice)) {
                    _endSession(GPSConnectionError::DeviceError, {}, true);
                }
            });
    }
}

GPSSerialPorts* GPSRtk::serialPorts() const
{
    return _serialPorts.data();
}

bool GPSRtk::connectDiscovered(const QString& device, QStringView boardName)
{
    for (const auto& entry : gpsReceiverDescriptors()) {
        // Discovery identifies receivers QGroundControl configures; a passive role is always chosen explicitly.
        if (entry.capabilities.passive) {
            continue;
        }
        if (boardName.contains(QLatin1StringView(entry.detectionKey.data(), entry.detectionKey.size()),
                               Qt::CaseInsensitive)) {
            return connectSerial(device, entry.type, 0, false);
        }
    }
    _setError(GPSConnectionError::ConfigFailed, tr("Select a specific receiver type before connecting."));
    return false;
}

bool GPSRtk::connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges)
{
    const QString endpoint = device.trimmed();
    if (endpoint.isEmpty() || !_serialPorts) {
        _setError(GPSConnectionError::OpenFailed, tr("Select an available serial device."));
        return false;
    }
    auto reservation = _serialPorts->reserve(endpoint);
    if (!reservation) {
        _setError(GPSConnectionError::OpenFailed, tr("The selected serial device is already in use."));
        return false;
    }
    return _connectReceiver(
        type, _roleFor(type),
        [endpoint, reservation, factory = _serialTransportFactory](const std::atomic_bool& requestStop) {
            return factory(endpoint, requestStop);
        },
        QStringLiteral("serial:%1").arg(endpoint), baudRate, allowPersistentChanges, endpoint, endpoint);
}
#endif

bool GPSRtk::connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges)
{
    const QString endpoint = QStringLiteral("%1:%2").arg(host).arg(port);
    return _connectReceiver(
        type, _roleFor(type),
        [host, port](const std::atomic_bool& requestStop) {
            return std::make_unique<TCPGPSTransport>(host, port, requestStop);
        },
        QStringLiteral("tcp:%1").arg(endpoint), GPSTransport::BRIDGE_BAUDRATE, allowPersistentChanges, {}, endpoint);
}

bool GPSRtk::connectUdp(quint16 port, GPSType type)
{
    const QString endpoint = QStringLiteral("UDP port %1").arg(port);
    return _connectReceiver(
        type, _roleFor(type),
        [port](const std::atomic_bool& requestStop) { return std::make_unique<UDPGPSTransport>(port, requestStop); },
        QStringLiteral("udp:%1").arg(port), GPSTransport::BRIDGE_BAUDRATE, false, {}, endpoint);
}

GPSRtk::ReceiverRole GPSRtk::_roleFor(GPSType type) const
{
    if (type != GPSType::passive) {
        return ConfiguredBase;
    }
    return _configuration.receiverRole == PositionOnly ? PositionOnly : Passive;
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

GPSReceiverPresentation GPSRtk::capabilitiesFor(int role, int manufacturer) const
{
    return gpsReceiverPresentation(role == ConfiguredBase ? manufacturer : manufacturerForType(GPSType::passive));
}

GPSReceiverPresentation GPSRtk::activePresentation() const
{
    return gpsReceiverPresentation(_session.manufacturer);
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

QString GPSRtk::_receiverConfig(GPSType type, const Configuration& configuration, uint32_t baudRate,
                                GPSReceiverConfig& config, bool allowPersistentChanges)
{
    config = GPSReceiverConfig{.baudRate = baudRate, .allowPersistentChanges = allowPersistentChanges};
    if (type == GPSType::passive) {
        config.role = GPSReceiverConfig::Role::Passive;
        return gpsReceiverConfigError(type, config);
    }
    switch (configuration.baseMode) {
        case 1:
            config.base.mode = GPSBaseStationConfig::Fixed{
                .position = {.latitudeDegrees = configuration.fixedBasePositionLatitude,
                             .longitudeDegrees = configuration.fixedBasePositionLongitude,
                             .altitudeMeters = configuration.fixedBasePositionAltitude},
                .accuracyMeters = configuration.fixedBasePositionAccuracy,
            };
            break;
        case 0:
            config.base.mode = GPSBaseStationConfig::SurveyIn{
                .accuracyMeters = configuration.surveyInAccuracyLimit,
                .durationSecs = configuration.surveyInMinObservationDuration,
            };
            break;
        case 2:
            config.base.mode =
                GPSBaseStationConfig::ReceiverAveraging{.maximumDurationSecs = configuration.receiverAveragingDuration};
            break;
        default:
            return tr("Select a supported base mode.");
    }
    // The option is hidden for receivers that cannot send MSM4, so it never blocks their connection.
    config.base.compactObservations =
        configuration.compactRtcmCorrections && gpsReceiverCapabilities(type, config.role).compactObservations;
    return gpsReceiverConfigError(type, config);
}

bool GPSRtk::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                             const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                             std::optional<ReceiverRole> role)
{
    if (_destroying) {
        return false;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->reset();
    // Only passive types can take a passive role, and they never configure a base.
    const ReceiverRole sessionRole = type != GPSType::passive          ? ConfiguredBase
                                     : role && *role != ConfiguredBase ? *role
                                                                       : _roleFor(type);
    return _connectReceiver(type, sessionRole, std::move(transportFactory), sourceInstance, baudRate,
                            allowPersistentChanges);
}

bool GPSRtk::_connectReceiver(GPSType type, ReceiverRole role, GPSProvider::TransportFactory transportFactory,
                              const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                              const QString& serialDevice, const QString& endpoint)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    GPSReceiverConfig config;
    const QString configError = _receiverConfig(type, _configuration, baudRate, config, allowPersistentChanges);
    if (!configError.isEmpty()) {
        _setError(GPSConnectionError::ConfigFailed, configError);
        return false;
    }
    _retireSession();
    _resetStatus();
    if (role == ConfiguredBase) {
        // Discovery records the family it detected.
        _notifications.emitSignal(this, &GPSRtk::baseManufacturerDetected, manufacturerForType(type));
    }
    _setError(GPSConnectionError::None);
    const bool forwardsCorrections = role != PositionOnly;
    const QPointer<GPSCorrectionManager> correctionManager = forwardsCorrections ? _correctionManager : nullptr;
    GPSCorrectionSourceRegistration registration;
    if (correctionManager) {
        registration = correctionManager->registerSource(GPSCorrectionSource::LocalReceiver, sourceInstance);
    }
    // A registration observer that connected another receiver is superseded by this request.
    _retireSession();
    _session.corrections = std::move(registration);
    _session.manufacturer = manufacturerForType(type);
    _session.role = role;
    _session.baseMode = config.role == GPSReceiverConfig::Role::Passive                                     ? -1
                        : std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode)             ? 1
                        : std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode) ? 2
                                                                                                            : 0;
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode)) {
        _session.basePosition = std::pair(fixed->position, static_cast<double>(fixed->accuracyMeters));
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode);
               survey && config.role != GPSReceiverConfig::Role::Passive) {
        _session.surveyAccuracyLimitMeters = survey->accuracyMeters;
    }
    _session.serialDevice = serialDevice;
    _session.endpoint = endpoint;
    _session.id = ++_sessionCount;
    _session.provider = _providerFactory(std::move(transportFactory), type, config, this);
    if (!_session.provider) {
        _retireSession();
        _setError(GPSConnectionError::OpenFailed, tr("Failed to create the receiver session."));
        return false;
    }
    _session.provider->setEndsWhenIdle(role == ConfiguredBase);
    const QPointer<GPSProvider> provider = _session.provider;
    (void) connect(
        provider, &GPSProvider::finished, this,
        [this, provider]() {
            if (provider && _session.provider == provider) {
                _endSession(GPSConnectionError::DeviceError, {}, false);
            }
        },
        Qt::QueuedConnection);
    (void) connect(provider, &GPSProvider::finished, provider, &QObject::deleteLater);
    const auto corrections = _session.corrections.weak();
    const auto current = [this, provider, corrections, registered = !correctionManager.isNull()]() {
        return provider && _session.provider == provider && (!registered || corrections.valid());
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
    // Queued callbacks capture a weak view of the producing session's registration.
    (void) connect(
        provider, &GPSProvider::RTCMDataUpdate, this,
        [correctionManager, corrections, forwardsCorrections](const QByteArray& data, qint64 receivedAtMs) {
            if (!correctionManager) {
                if (forwardsCorrections) {
                    qCWarning(GPSRtkLog) << "Correction manager not ready; dropping" << data.size() << "bytes";
                }
                return;
            }
            const bool valid = RTCMFramer::isValidFrame(data);
            correctionManager->acceptIngress(
                corrections.event(data, receivedAtMs, RTCMFramer::frameMessageId(data), valid, false,
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
    // Started workers own their transport reservation until the session exits; finished() schedules deletion.
}

void GPSRtk::disconnectGPS()
{
    if (_destroying) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->reset();
    disconnectReceiver(false);
}

void GPSRtk::disconnectReceiver(bool clearError)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _retireSession();
    _resetStatus();
    _notifyReceiverChanged();
    if (clearError) {
        _setError(GPSConnectionError::None);
    }
}

void GPSRtk::_satelliteInfoUpdate(const GPSSatelliteReport& msg)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const int inView = msg.inView.value_or(-1);
    const int used = msg.used.value_or(-1);
    qCDebug(GPSRtkLog) << QStringLiteral("%1 in view, %2 used")
                              .arg(inView)
                              .arg(msg.used ? QString::number(used) : QStringLiteral("unknown"));
    _status.numSatellites = inView;
    _status.numSatellitesUsed = used;
    _publishStatus();
}

void GPSRtk::_positionUpdate(const GPSPositionReport& report)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const auto fixType = static_cast<int>(report.navigation.fixType);
    if (std::exchange(_session.fixType, fixType) != fixType) {
        qCDebug(GPSRtkLog) << "Receiver fix changed:" << fixType;
    }
    _status.fixType = report.navigation.fixType;
    // Drivers project integrity to Unknown once it is older than its freshness window.
    _status.jammingState = report.integrity.jamming.state;
    _status.spoofingState = report.integrity.spoofing.state;
    _publishStatus();
    const quint64 receivedAtUs = MonotonicClock::nowUs();
    auto observation = _session.basePosition
                           ? GPSObservation::fromSurveyedPosition(_session.basePosition->first,
                                                                  _session.basePosition->second, receivedAtUs)
                           : GPSObservation::fromNavigation(report.navigation, receivedAtUs);
    observation.sessionId = _session.id;
    _positionHealth->updateObservation(observation);
}
