#include "GPSReceiver.h"

#include "GPSBaseStationSettings.h"
#include "GPSCorrectionManager.h"
#include "GPSPositionService.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverSession.h"
#include "GPSSourceHealth.h"
#include "QGCLoggingCategory.h"
#include "TCPGPSTransport.h"
#include "UDPGPSTransport.h"

#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPorts.h"
#include "SerialGPSTransport.h"
#endif

#include <utility>

QGC_LOGGING_CATEGORY(GPSReceiverLog, "GPS.Receiver.GPSReceiver")

QDebug operator<<(QDebug debug, const GPSReceiver::Configuration& configuration)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "GPSReceiver::Configuration(receiverRole=" << configuration.receiverRole
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

GPSBaseStationSettings GPSReceiver::Configuration::baseStationSettings() const
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

GPSReceiver::GPSReceiver(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _positionHealth(new GPSSourceHealth(this, scheduler))
{
    qCDebug(GPSReceiverLog) << this;
    // A silent receiver stays connected, so its last solution must not remain on display.
    connect(_positionHealth, &GPSSourceHealth::positionChanged, this, [this]() {
        if (_positionHealth->state() == GPSSourceHealth::State::Stale) {
            _clearStaleSolution();
        }
    });
    _connection = new GPSReceiverConnectionPolicy(*this, this, scheduler);
    connect(_connection, &GPSReceiverConnectionPolicy::reconnectingChanged, this, &GPSReceiver::_notifyReceiverChanged);
    connect(_connection, &GPSReceiverConnectionPolicy::autoConnectDisabled, this, &GPSReceiver::autoConnectDisabled);
#ifndef QGC_NO_SERIAL_LINK
    _serialTransportFactory = [](const QString& device, std::stop_token stopToken) {
        return std::make_unique<SerialGPSTransport>(device, std::move(stopToken));
    };
#endif
}

namespace {
/// The settings a persistent-change permission was granted for.
bool consentScopeChanged(const GPSReceiver::Configuration& before, const GPSReceiver::Configuration& after)
{
    return before.receiverRole != after.receiverRole ||
           before.baseReceiverManufacturer != after.baseReceiverManufacturer || before.baseMode != after.baseMode ||
           before.connectionType != after.connectionType || before.serialDevice != after.serialDevice ||
           before.serialBaudRate != after.serialBaudRate || before.tcpHost != after.tcpHost ||
           before.tcpPort != after.tcpPort || before.udpPort != after.udpPort;
}
}  // namespace

void GPSReceiver::setConfiguration(const Configuration& configuration)
{
    if (_configuration == configuration) {
        return;
    }
    const NotificationQueue::Scope publish(_notifications);
    if (consentScopeChanged(_configuration, configuration)) {
        setPersistentChangesAllowed(false);
    }
    _configuration = configuration;
    qCDebug(GPSReceiverLog) << "Receiver configuration applied:" << _configuration;
    _connection->setConfiguration(configuration);
    _notifications.emitSignal(this, &GPSReceiver::configurationChanged);
}

void GPSReceiver::setPersistentChangesAllowed(bool allowed)
{
    if (_destroying || _persistentChangesAllowed == allowed) {
        return;
    }
    const NotificationQueue::Scope publish(_notifications);
    _persistentChangesAllowed = allowed;
    _notifications.emitSignal(this, &GPSReceiver::persistentChangesAllowedChanged);
}

GPSReceiver::ConnectionType GPSReceiver::connectionTypeFor(const Configuration& configuration)
{
    const auto saved = configuration.connectionType;
#ifdef QGC_NO_SERIAL_LINK
    return saved == Udp ? Udp : Tcp;
#else
    return saved == Tcp || saved == Udp ? saved : Serial;
#endif
}

bool GPSReceiver::connectionSupported() const
{
    return _configuration.receiverRole != ConfiguredBase || effectiveConnectionType() != Udp;
}

void GPSReceiver::setProviderFactory(ProviderFactory factory)
{
    if (_destroying) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSReceiverLog) << "Inject the provider factory before connecting a receiver";
        return;
    }
    _providerFactory = std::move(factory);
}

GPSReceiver::~GPSReceiver()
{
    _destroying = true;
    _notifications.close();
    // The policy observes settings and must not run against a partially destroyed receiver.
    delete std::exchange(_connection, nullptr);
    _retireSession();

    qCDebug(GPSReceiverLog) << this;
}

void GPSReceiver::_publishStatus()
{
    _notifications.emitSignal(this, &GPSReceiver::statusChanged);
}

void GPSReceiver::_resetStatus()
{
    _status = {};
    _publishStatus();
}

void GPSReceiver::_clearStaleSolution()
{
    const NotificationQueue::Scope publish(_notifications);
    _reportedFixType.reset();
    const Status none;
    _status.numSatellites = none.numSatellites;
    _status.numSatellitesUsed = none.numSatellitesUsed;
    _status.fixType = none.fixType;
    _status.jammingState = none.jammingState;
    _status.spoofingState = none.spoofingState;
    _publishStatus();
}

void GPSReceiver::_notifyReceiverChanged()
{
    setPersistentChangesAllowed(false);
    _notifications.emitSignal(this, &GPSReceiver::receiverChanged);
}

void GPSReceiver::_setError(GPSConnectionError error, const QString& message)
{
    _connectionError = error;
    if (std::exchange(_errorMessage, message) != message) {
        _notifications.emitSignal(this, &GPSReceiver::errorMessageChanged);
    }
}

void GPSReceiver::_onGPSConnect()
{
    const NotificationQueue::Scope publish(_notifications);
    _setError(GPSConnectionError::None);
    _connection->receiverReady();
    _status.connected = true;
    _publishStatus();
    _notifyReceiverChanged();
    if (_session) {
        _session->registerPosition(_positionService, _positionHealth);
    }
}

void GPSReceiver::_onReceiverDetected(GPSType type)
{
    const NotificationQueue::Scope publish(_notifications);
    _status.detectedType = type;
    _publishStatus();
    _notifyReceiverChanged();
}

void GPSReceiver::_endSession(GPSConnectionError error, const QString& detail, bool portRemoved)
{
    const NotificationQueue::Scope publish(_notifications);
    // The family an Automatic session detected, else the selected one.
    const auto* family = _session ? gpsReceiverDescriptorForManufacturer(_session->manufacturer()) : nullptr;
    _retireSession();
    _resetStatus();
    _notifyReceiverChanged();
    const GPSReceiverSessionOutcome outcome = _connection->sessionEnded(portRemoved);
    const bool configurationFailed =
        error == GPSConnectionError::ConfigFailed || error == GPSConnectionError::ConsentRequired;
    if (portRemoved) {
        qCDebug(GPSReceiverLog) << "Receiver serial device removed";
    }
    switch (outcome) {
        case GPSReceiverSessionOutcome::Unplugged:
            _setError(error, tr("Receiver unplugged."));
            return;
        case GPSReceiverSessionOutcome::AutoRetrying:
            // Auto-connect never passes the flash-save consent this receiver asked for.
            if (error == GPSConnectionError::ConsentRequired && !detail.isEmpty() && family) {
                qCWarning(GPSReceiverLog) << "GPS receiver did not accept configuration";
                //: %1 explains the failure; %2 is a receiver manufacturer, such as Quectel
                _setError(error, tr("Receiver configuration failed: %1. Auto-connect never saves settings to the "
                                    "receiver's flash. To let QGroundControl save the %2 receiver's settings, connect "
                                    "manually and allow flash save and restart.")
                                     .arg(detail, QString::fromLatin1(family->name.data(),
                                                                      static_cast<qsizetype>(family->name.size()))));
                return;
            }
            break;
        case GPSReceiverSessionOutcome::WaitingForPort:
            _setError(error, tr("Receiver unplugged. Reconnecting when it is plugged back in."));
            return;
        case GPSReceiverSessionOutcome::Retrying:
            qCWarning(GPSReceiverLog) << "GPS receiver session ended:" << error << detail;
            _setError(error, configurationFailed && !detail.isEmpty()
                                 ? tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail)
                                 : tr("Receiver connection lost. Reconnecting automatically."));
            return;
        case GPSReceiverSessionOutcome::None:
            break;
    }
    if (portRemoved) {
        _setError(error, tr("Receiver unplugged. Select a device and reconnect."));
        return;
    }
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSReceiverLog) << "Failed to open GPS receiver transport";
            _setError(error, tr("Failed to open the receiver. Check the device or network address, permissions, and "
                                "other connections."));
            break;
        case GPSConnectionError::ConfigFailed:
        case GPSConnectionError::ConsentRequired:
            qCWarning(GPSReceiverLog) << "GPS receiver did not accept configuration";
            _setError(error,
                      detail.isEmpty()
                          ? tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode.")
                          : tr("Receiver configuration failed: %1").arg(detail));
            break;
        case GPSConnectionError::DeviceError:
            qCWarning(GPSReceiverLog) << "GPS device error, connection lost";
            _setError(error, tr("Receiver connection lost. Check the device and reconnect."));
            break;
        case GPSConnectionError::None:
            _setError(error);
            break;
    }
}

void GPSReceiver::_onGPSSurveyReport(const GPSSurveyReport& status)
{
    const NotificationQueue::Scope publish(_notifications);
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
void GPSReceiver::setSerialPorts(GPSSerialPorts* serialPorts)
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

GPSSerialPorts* GPSReceiver::serialPorts() const
{
    return _serialPorts.data();
}

bool GPSReceiver::connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges)
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

bool GPSReceiver::connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges)
{
    const QString endpoint = QStringLiteral("%1:%2").arg(host).arg(port);
    return _connectReceiver(
        type, _roleFor(type),
        [host, port](std::stop_token stopToken) {
            return std::make_unique<TCPGPSTransport>(host, port, std::move(stopToken));
        },
        QStringLiteral("tcp:%1").arg(endpoint), GPSTransport::BRIDGE_BAUDRATE, allowPersistentChanges, {}, endpoint);
}

bool GPSReceiver::connectUdp(quint16 port, GPSType type)
{
    const QString endpoint = QStringLiteral("UDP port %1").arg(port);
    return _connectReceiver(
        type, _roleFor(type),
        [port](std::stop_token stopToken) { return std::make_unique<UDPGPSTransport>(port, std::move(stopToken)); },
        QStringLiteral("udp:%1").arg(port), GPSTransport::BRIDGE_BAUDRATE, false, {}, endpoint);
}

GPSReceiver::ReceiverRole GPSReceiver::_roleFor(GPSType type) const
{
    if (type != GPSType::passive) {
        return ConfiguredBase;
    }
    return _configuration.receiverRole == PositionOnly ? PositionOnly : Passive;
}

bool GPSReceiver::serialSupported() const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    return true;
#endif
}

std::optional<GPSType> GPSReceiver::typeForManufacturer(int manufacturer)
{
    if (manufacturer == GPS_AUTOMATIC_MANUFACTURER) {
        return GPSType::automatic;
    }
    if (const auto* descriptor = gpsReceiverDescriptorForManufacturer(manufacturer)) {
        return descriptor->type;
    }
    return std::nullopt;
}

int GPSReceiver::manufacturerForType(GPSType type)
{
    const auto* descriptor = gpsReceiverDescriptor(type);
    return descriptor ? descriptor->manufacturerId : GPS_AUTOMATIC_MANUFACTURER;
}

GPSReceiverPresentation GPSReceiver::capabilitiesFor(int role, int manufacturer) const
{
    return gpsReceiverPresentation(role == ConfiguredBase ? manufacturer : manufacturerForType(GPSType::passive));
}

bool GPSReceiver::hasReceiver() const
{
    return _session && _session->hasProvider();
}

int GPSReceiver::activeManufacturer() const
{
    return _session ? _session->manufacturer() : 0;
}

int GPSReceiver::activeBaseMode() const
{
    return _session ? _session->baseMode() : -1;
}

QString GPSReceiver::activeEndpoint() const
{
    return _session ? _session->endpoint() : QString();
}

QString GPSReceiver::receiverIdentity() const
{
    return _session ? _session->identity() : QString();
}

QString GPSReceiver::detectedReceiver() const
{
    const auto* descriptor = _status.detectedType ? gpsReceiverDescriptor(*_status.detectedType) : nullptr;
    return descriptor ? QString::fromLatin1(descriptor->name.data(), static_cast<qsizetype>(descriptor->name.size()))
                      : QString();
}

GPSReceiver::ReceiverRole GPSReceiver::activeRole() const
{
    return _session ? _session->role() : ConfiguredBase;
}

GPSReceiverPresentation GPSReceiver::activePresentation() const
{
    return gpsReceiverPresentation(activeManufacturer());
}

bool GPSReceiver::connectConfiguredGPS(bool allowPersistentChanges)
{
    if (_destroying) {
        return false;
    }
    const NotificationQueue::Scope publish(_notifications);
    return _connection->connectConfigured(allowPersistentChanges);
}

bool GPSReceiver::connectSelectedReceiver()
{
    if (_destroying) {
        return false;
    }
    const NotificationQueue::Scope publish(_notifications);
    const bool allowed =
        _persistentChangesAllowed &&
        capabilitiesFor(_configuration.receiverRole, _configuration.baseReceiverManufacturer).persistentConfiguration;
    setPersistentChangesAllowed(false);
    return connectConfiguredGPS(allowed);
}

void GPSReceiver::disconnectConfiguredGPS()
{
    if (_destroying) {
        return;
    }
    const NotificationQueue::Scope publish(_notifications);
    setPersistentChangesAllowed(false);
    _connection->disconnectConfigured();
}

bool GPSReceiver::reconnecting() const
{
    return _connection && _connection->reconnecting();
}

void GPSReceiver::setPositionService(GPSPositionService* service)
{
    if (_destroying || _positionService == service) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSReceiverLog) << "Inject the position service before connecting a receiver";
        return;
    }
    _positionService = service;
}

std::optional<GPSObservation> GPSReceiver::acceptedPositionObservation(GPSObservation::PositionUse use) const
{
    return _positionHealth->acceptedObservation(use);
}

void GPSReceiver::setCorrectionManager(GPSCorrectionManager* manager)
{
    if (_destroying || _correctionManager == manager) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSReceiverLog) << "Inject the correction manager before connecting a receiver";
        return;
    }
    _correctionManager = manager;
}

bool GPSReceiver::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                                  const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                                  std::optional<ReceiverRole> role)
{
    if (_destroying) {
        return false;
    }
    const NotificationQueue::Scope publish(_notifications);
    _connection->reset();
    // Only passive types can take a passive role, and they never configure a base.
    const ReceiverRole sessionRole = type != GPSType::passive          ? ConfiguredBase
                                     : role && *role != ConfiguredBase ? *role
                                                                       : _roleFor(type);
    return _connectReceiver(type, sessionRole, std::move(transportFactory), sourceInstance, baudRate,
                            allowPersistentChanges);
}

bool GPSReceiver::_connectReceiver(GPSType type, ReceiverRole role, GPSProvider::TransportFactory transportFactory,
                                   const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                                   const QString& serialDevice, const QString& endpoint)
{
    const NotificationQueue::Scope publish(_notifications);
    QString configError;
    GPSReceiverConfig config = gpsReceiverConfigFor(_configuration.baseStationSettings(), type, baudRate,
                                                    allowPersistentChanges, &configError);
    if (!configError.isEmpty()) {
        _setError(GPSConnectionError::ConfigFailed, configError);
        return false;
    }
    _retireSession();
    _resetStatus();
    _setError(GPSConnectionError::None);
    auto* const session = new GPSReceiverSession(
        ++_sessionCount,
        {.type = type, .role = role, .config = std::move(config), .serialDevice = serialDevice, .endpoint = endpoint},
        this);
    session->registerCorrections(_correctionManager, sourceInstance);
    // A registration observer that connected another receiver is superseded by this request.
    _retireSession();
    _session = session;
    connect(session, &GPSReceiverSession::satelliteInfoUpdate, this, &GPSReceiver::_satelliteInfoUpdate);
    connect(session, &GPSReceiverSession::positionUpdate, this, &GPSReceiver::_positionUpdate);
    connect(session, &GPSReceiverSession::surveyInStatus, this, &GPSReceiver::_onGPSSurveyReport);
    connect(session, &GPSReceiverSession::receiverReady, this, &GPSReceiver::_onGPSConnect);
    connect(session, &GPSReceiverSession::receiverDetected, this, &GPSReceiver::_onReceiverDetected);
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

void GPSReceiver::_retireSession()
{
    _reportedFixType.reset();
    if (auto* const retired = std::exchange(_session, nullptr)) {
        retired->deleteLater();
        retired->retire();
    }
    _positionHealth->reset();
}

void GPSReceiver::disconnectGPS()
{
    if (_destroying) {
        return;
    }
    const NotificationQueue::Scope publish(_notifications);
    _connection->reset();
    disconnectReceiver(false);
}

void GPSReceiver::disconnectReceiver(bool clearError)
{
    const NotificationQueue::Scope publish(_notifications);
    _retireSession();
    _resetStatus();
    _notifyReceiverChanged();
    if (clearError) {
        _setError(GPSConnectionError::None);
    }
}

void GPSReceiver::_satelliteInfoUpdate(const GPSSatelliteReport& msg)
{
    const NotificationQueue::Scope publish(_notifications);
    const int inView = msg.inView.value_or(-1);
    const int used = msg.used.value_or(-1);
    qCDebug(GPSReceiverLog) << QStringLiteral("%1 in view, %2 used")
                                   .arg(inView)
                                   .arg(msg.used ? QString::number(used) : QStringLiteral("unknown"));
    _status.numSatellites = inView;
    _status.numSatellitesUsed = used;
    _publishStatus();
}

void GPSReceiver::_positionUpdate(const GPSPositionReport& report)
{
    const NotificationQueue::Scope publish(_notifications);
    const auto fixType = static_cast<int>(report.navigation.fixType);
    if (std::exchange(_reportedFixType, fixType) != fixType) {
        qCDebug(GPSReceiverLog) << "Receiver fix changed:" << fixType;
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
