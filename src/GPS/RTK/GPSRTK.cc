#include "GPSRTK.h"

#include "GPSBaseStationSettings.h"
#include "GPSCorrectionManager.h"
#include "GPSPositionService.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverSession.h"
#include "GPSSourceHealth.h"
#include "QGCLoggingCategory.h"
#include "RTKConnectionPolicy.h"
#include "TCPGPSTransport.h"
#include "UDPGPSTransport.h"

#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPorts.h"
#include "SerialGPSTransport.h"
#endif

#include <utility>

QGC_LOGGING_CATEGORY(GPSRTKLog, "GPS.RTK.GPSRTK")

QDebug operator<<(QDebug debug, const GPSRTK::Configuration& configuration)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "GPSRTK::Configuration(receiverRole=" << configuration.receiverRole
                              << ", baseReceiverManufacturer=" << configuration.baseReceiverManufacturer
                              << ", connectionType=" << configuration.connectionType
                              << ", tcpHost=" << configuration.tcpHost << ", tcpPort=" << configuration.tcpPort
                              << ", udpPort=" << configuration.udpPort
                              << ", serialDevice=" << configuration.serialDevice
                              << ", serialBaudRate=" << configuration.serialBaudRate
                              << ", baseMode=" << configuration.baseMode
                              << ", surveyInAccuracyLimit=" << configuration.surveyInAccuracyLimit
                              << ", surveyInMinObservationDuration="
                              << configuration.surveyInMinObservationDuration.count()
                              << ", receiverAveragingDuration=" << configuration.receiverAveragingDuration.count()
                              << ", compactRtcmCorrections=" << configuration.compactRtcmCorrections
                              << ", autoConnect=" << configuration.autoConnect << ')';
    return debug;
}

GPSBaseStationSettings GPSRTK::Configuration::baseStationSettings() const
{
    return {
        .mode = static_cast<GPSBaseStationSettings::Mode>(baseMode),
        .fixedPosition = {.latitudeDegrees = fixedBasePositionLatitude,
                          .longitudeDegrees = fixedBasePositionLongitude,
                          .altitudeMeters = fixedBasePositionAltitude},
        .fixedAccuracyMeters = fixedBasePositionAccuracy,
        .surveyInAccuracyMeters = surveyInAccuracyLimit,
        .surveyInMinimumDuration = surveyInMinObservationDuration,
        .averagingMaximumDuration = receiverAveragingDuration,
        .compactObservations = compactRtcmCorrections,
    };
}

GPSRTK::GPSRTK(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _positionHealth(new GPSSourceHealth(this, scheduler))
{
    qCDebug(GPSRTKLog) << this;
    // A silent receiver stays connected, so its last solution must not remain on display.
    connect(_positionHealth, &GPSSourceHealth::positionChanged, this, [this]() {
        if (_positionHealth->state() == GPSSourceHealth::State::Stale) {
            _clearStaleSolution();
        }
    });
    _connection = new RTKConnectionPolicy(*this, this, scheduler);
    connect(_connection, &RTKConnectionPolicy::reconnectingChanged, this, &GPSRTK::_notifyReceiverChanged);
    connect(_connection, &RTKConnectionPolicy::autoConnectDisabled, this, &GPSRTK::autoConnectDisabled);
#ifndef QGC_NO_SERIAL_LINK
    _serialTransportFactory = [](const QString& device, std::stop_token stopToken) {
        return std::make_unique<SerialGPSTransport>(device, std::move(stopToken));
    };
#endif
}

void GPSRTK::setConfiguration(const Configuration& configuration)
{
    if (_configuration == configuration) {
        return;
    }
    _configuration = configuration;
    qCDebug(GPSRTKLog) << "RTK configuration applied:" << _configuration;
    _connection->setConfiguration(configuration);
}

void GPSRTK::setProviderFactory(ProviderFactory factory)
{
    if (_destroying) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSRTKLog) << "Inject the provider factory before connecting a receiver";
        return;
    }
    _providerFactory = std::move(factory);
}

GPSRTK::~GPSRTK()
{
    _destroying = true;
    _notifications.close();
    // The policy observes settings and must not run against a partially destroyed receiver.
    delete std::exchange(_connection, nullptr);
    _retireSession();

    qCDebug(GPSRTKLog) << this;
}

void GPSRTK::_publishStatus()
{
    _notifications.emitSignal(this, &GPSRTK::statusChanged);
}

void GPSRTK::_resetStatus()
{
    _status = {};
    _publishStatus();
}

void GPSRTK::_clearStaleSolution()
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _reportedFixType.reset();
    const Status none;
    _status.numSatellites = none.numSatellites;
    _status.numSatellitesUsed = none.numSatellitesUsed;
    _status.fixType = none.fixType;
    _status.jammingState = none.jammingState;
    _status.spoofingState = none.spoofingState;
    _publishStatus();
}

void GPSRTK::_notifyReceiverChanged()
{
    _notifications.emitSignal(this, &GPSRTK::receiverChanged);
}

void GPSRTK::_setError(GPSConnectionError error, const QString& message)
{
    _connectionError = error;
    if (std::exchange(_errorMessage, message) != message) {
        _notifications.emitSignal(this, &GPSRTK::errorMessageChanged);
    }
}

void GPSRTK::_onGPSConnect()
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _setError(GPSConnectionError::None);
    _connection->receiverReady();
    _status.connected = true;
    _publishStatus();
    _notifyReceiverChanged();
    if (_session) {
        _session->registerPosition(_positionService, _positionHealth);
    }
}

void GPSRTK::_endSession(GPSConnectionError error, const QString& detail, bool portRemoved)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _retireSession();
    _resetStatus();
    _notifyReceiverChanged();
    const RTKSessionOutcome outcome = _connection->sessionEnded(portRemoved);
    if (portRemoved) {
        qCDebug(GPSRTKLog) << "Receiver serial device removed";
    }
    switch (outcome) {
        case RTKSessionOutcome::Unplugged:
            _setError(error, tr("Receiver unplugged."));
            return;
        case RTKSessionOutcome::WaitingForPort:
            _setError(error, tr("Receiver unplugged. Reconnecting when it is plugged back in."));
            return;
        case RTKSessionOutcome::Retrying:
            qCWarning(GPSRTKLog) << "GPS receiver session ended:" << error << detail;
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
            qCWarning(GPSRTKLog) << "Failed to open GPS receiver transport";
            _setError(error, tr("Failed to open the receiver. Check the device or network address, permissions, and "
                                "other connections."));
            break;
        case GPSConnectionError::ConfigFailed:
            qCWarning(GPSRTKLog) << "GPS receiver did not accept configuration";
            _setError(error,
                      detail.isEmpty()
                          ? tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode.")
                          : tr("Receiver configuration failed: %1").arg(detail));
            break;
        case GPSConnectionError::DeviceError:
            qCWarning(GPSRTKLog) << "GPS device error, connection lost";
            _setError(error, tr("Receiver connection lost. Check the device and reconnect."));
            break;
        case GPSConnectionError::None:
            _setError(error);
            break;
    }
}

void GPSRTK::_onGPSSurveyReport(const GPSSurveyReport& status)
{
    const GPSNotificationQueue::Scope publish(_notifications);
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
void GPSRTK::setSerialPorts(GPSSerialPorts* serialPorts)
{
    if (_destroying || hasReceiver()) {
        return;
    }
    QObject::disconnect(_portEnumerationConnection);
    _serialPorts = serialPorts;
    if (_serialPorts) {
        _portEnumerationConnection =
            connect(_serialPorts, &GPSSerialPorts::portsEnumerated, this, [this](const QStringList& availablePorts) {
                if (_session && !_session->serialDevice().isEmpty() &&
                    !availablePorts.contains(_session->serialDevice())) {
                    _endSession(GPSConnectionError::DeviceError, {}, true);
                }
            });
    }
}

GPSSerialPorts* GPSRTK::serialPorts() const
{
    return _serialPorts.data();
}

bool GPSRTK::connectDiscovered(const QString& device, QStringView boardName)
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

bool GPSRTK::connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges)
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
        [endpoint, reservation, factory = _serialTransportFactory](std::stop_token stopToken) {
            return factory(endpoint, std::move(stopToken));
        },
        QStringLiteral("serial:%1").arg(endpoint), baudRate, allowPersistentChanges, endpoint, endpoint);
}
#endif

bool GPSRTK::connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges)
{
    const QString endpoint = QStringLiteral("%1:%2").arg(host).arg(port);
    return _connectReceiver(
        type, _roleFor(type),
        [host, port](std::stop_token stopToken) {
            return std::make_unique<TCPGPSTransport>(host, port, std::move(stopToken));
        },
        QStringLiteral("tcp:%1").arg(endpoint), GPSTransport::BRIDGE_BAUDRATE, allowPersistentChanges, {}, endpoint);
}

bool GPSRTK::connectUdp(quint16 port, GPSType type)
{
    const QString endpoint = QStringLiteral("UDP port %1").arg(port);
    return _connectReceiver(
        type, _roleFor(type),
        [port](std::stop_token stopToken) { return std::make_unique<UDPGPSTransport>(port, std::move(stopToken)); },
        QStringLiteral("udp:%1").arg(port), GPSTransport::BRIDGE_BAUDRATE, false, {}, endpoint);
}

GPSRTK::ReceiverRole GPSRTK::_roleFor(GPSType type) const
{
    if (type != GPSType::passive) {
        return ConfiguredBase;
    }
    return _configuration.receiverRole == PositionOnly ? PositionOnly : Passive;
}

bool GPSRTK::serialSupported() const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    return true;
#endif
}

std::optional<GPSType> GPSRTK::typeForManufacturer(int manufacturer)
{
    if (const auto* descriptor = gpsReceiverDescriptorForManufacturer(manufacturer)) {
        return descriptor->type;
    }
    return std::nullopt;
}

int GPSRTK::manufacturerForType(GPSType type)
{
    const auto* descriptor = gpsReceiverDescriptor(type);
    return descriptor ? descriptor->manufacturerId : 0;
}

GPSReceiverPresentation GPSRTK::capabilitiesFor(int role, int manufacturer) const
{
    return gpsReceiverPresentation(role == ConfiguredBase ? manufacturer : manufacturerForType(GPSType::passive));
}

bool GPSRTK::hasReceiver() const
{
    return _session && _session->hasProvider();
}

int GPSRTK::activeManufacturer() const
{
    return _session ? _session->manufacturer() : 0;
}

int GPSRTK::activeBaseMode() const
{
    return _session ? _session->baseMode() : -1;
}

QString GPSRTK::activeEndpoint() const
{
    return _session ? _session->endpoint() : QString();
}

QString GPSRTK::receiverIdentity() const
{
    return _session ? _session->identity() : QString();
}

GPSRTK::ReceiverRole GPSRTK::activeRole() const
{
    return _session ? _session->role() : ConfiguredBase;
}

GPSReceiverPresentation GPSRTK::activePresentation() const
{
    return gpsReceiverPresentation(activeManufacturer());
}

bool GPSRTK::connectConfiguredGPS(bool allowPersistentChanges)
{
    if (_destroying) {
        return false;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    return _connection->connectConfigured(allowPersistentChanges);
}

void GPSRTK::disconnectConfiguredGPS()
{
    if (_destroying) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->disconnectConfigured();
}

bool GPSRTK::reconnecting() const
{
    return _connection && _connection->reconnecting();
}

void GPSRTK::setPositionService(GPSPositionService* service)
{
    if (_destroying || _positionService == service) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSRTKLog) << "Inject the position service before connecting a receiver";
        return;
    }
    _positionService = service;
}

std::optional<GPSObservation> GPSRTK::acceptedPositionObservation(GPSObservation::PositionUse use) const
{
    return _positionHealth->acceptedObservation(use);
}

void GPSRTK::setCorrectionManager(GPSCorrectionManager* manager)
{
    if (_destroying || _correctionManager == manager) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSRTKLog) << "Inject the correction manager before connecting a receiver";
        return;
    }
    _correctionManager = manager;
}

bool GPSRTK::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
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

bool GPSRTK::_connectReceiver(GPSType type, ReceiverRole role, GPSProvider::TransportFactory transportFactory,
                              const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                              const QString& serialDevice, const QString& endpoint)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    QString configError;
    GPSReceiverConfig config = gpsReceiverConfigFor(_configuration.baseStationSettings(), type, baudRate,
                                                    allowPersistentChanges, &configError);
    if (!configError.isEmpty()) {
        _setError(GPSConnectionError::ConfigFailed, configError);
        return false;
    }
    _retireSession();
    _resetStatus();
    if (role == ConfiguredBase) {
        // Discovery records the family it detected.
        _notifications.emitSignal(this, &GPSRTK::baseManufacturerDetected, manufacturerForType(type));
    }
    _setError(GPSConnectionError::None);
    auto* const session = new GPSReceiverSession(
        ++_sessionCount,
        {.type = type, .role = role, .config = std::move(config), .serialDevice = serialDevice, .endpoint = endpoint},
        this);
    session->registerCorrections(_correctionManager, sourceInstance);
    // A registration observer that connected another receiver is superseded by this request.
    _retireSession();
    _session = session;
    connect(session, &GPSReceiverSession::satelliteInfoUpdate, this, &GPSRTK::_satelliteInfoUpdate);
    connect(session, &GPSReceiverSession::positionUpdate, this, &GPSRTK::_positionUpdate);
    connect(session, &GPSReceiverSession::surveyInStatus, this, &GPSRTK::_onGPSSurveyReport);
    connect(session, &GPSReceiverSession::receiverReady, this, &GPSRTK::_onGPSConnect);
    connect(session, &GPSReceiverSession::ended, this,
            [this](GPSConnectionError error, const QString& detail) { _endSession(error, detail, false); });
    if (!session->start(_providerFactory, std::move(transportFactory))) {
        _retireSession();
        _setError(GPSConnectionError::OpenFailed, tr("Failed to create the receiver session."));
        return false;
    }
    _notifyReceiverChanged();
    return true;
}

void GPSRTK::_retireSession()
{
    _reportedFixType.reset();
    if (auto* const retired = std::exchange(_session, nullptr)) {
        retired->deleteLater();
        retired->retire();
    }
    _positionHealth->reset();
}

void GPSRTK::disconnectGPS()
{
    if (_destroying) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _connection->reset();
    disconnectReceiver(false);
}

void GPSRTK::disconnectReceiver(bool clearError)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _retireSession();
    _resetStatus();
    _notifyReceiverChanged();
    if (clearError) {
        _setError(GPSConnectionError::None);
    }
}

void GPSRTK::_satelliteInfoUpdate(const GPSSatelliteReport& msg)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const int inView = msg.inView.value_or(-1);
    const int used = msg.used.value_or(-1);
    qCDebug(GPSRTKLog) << QStringLiteral("%1 in view, %2 used")
                              .arg(inView)
                              .arg(msg.used ? QString::number(used) : QStringLiteral("unknown"));
    _status.numSatellites = inView;
    _status.numSatellitesUsed = used;
    _publishStatus();
}

void GPSRTK::_positionUpdate(const GPSPositionReport& report)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const auto fixType = static_cast<int>(report.navigation.fixType);
    if (std::exchange(_reportedFixType, fixType) != fixType) {
        qCDebug(GPSRTKLog) << "Receiver fix changed:" << fixType;
    }
    _status.fixType = report.navigation.fixType;
    // Drivers project integrity to Unknown once it is older than its freshness window.
    _status.jammingState = report.integrity.jamming.state;
    _status.spoofingState = report.integrity.spoofing.state;
    _publishStatus();
    if (_session) {
        _positionHealth->updateObservation(_session->observation(report));
    }
}
