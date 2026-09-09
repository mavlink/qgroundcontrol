#include "GPSManager.h"

#include "AppMessages.h"
#include "AutoConnectSettings.h"
#include "GPSBaseStationState.h"
#include "GPSConnectionSettings.h"
#include "GPSCorrectionSettings.h"
#include "GPSMavlinkOutput.h"
#include "GPSPositionSettings.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSReceiver.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverSettingsPresentation.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "NMEAConnectionConfig.h"
#include "NMEASourceManager.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "VehicleGPSPositionProvider.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif
#include <QtCore/QApplicationStatic>

#include <utility>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject* parent)
    : GPSManager(
          *SettingsManager::instance(), QGCPositionManager::instance(),
          []() { return LinkManager::instance()->connectionsSuspended(); }, parent)
{
    qCDebug(GPSManagerLog) << this;
    connect(LinkManager::instance(), &LinkManager::connectionsSuspendedChanged, this, &GPSManager::_updateConnections);
    auto* output = new GPSMavlinkOutput(this);
    _corrections.rtcmMavlink()->setOutputProvider([output]() { return output->outputs(); });
    _vehiclePositionProvider = new VehicleGPSPositionProvider(this);
    auto* vehicles = MultiVehicleManager::instance();
    _vehiclePositionProvider->setVehicle(vehicles->activeVehicle());
    connect(vehicles, &MultiVehicleManager::activeVehicleChanged, _vehiclePositionProvider,
            &VehicleGPSPositionProvider::setVehicle);
}

GPSManager::GPSManager(SettingsManager& settingsManager, QGCPositionManager* positionManager,
                       std::function<bool()> connectionsSuspended, QObject* parent)
    : QObject(parent)
    , _settings(settingsManager)
    , _connectionsSuspended(std::move(connectionsSuspended))
    , _positionManager(positionManager)
    , _corrections(this)
    , _recording(this)
    , _satellites(this)
    , _nmeaSatellites(this)
    , _relativePosition(this)
    , _receiverSession(this)
    , _receiver(new GPSReceiver(_receiverSession, this))
    , _baseStationState(new GPSBaseStationState(_receiverSession, *_receiver->facts()->rtk(), this))
{
    qCDebug(GPSManagerLog) << this;

    auto* settings = &_settings;
    connect(&_receiverSession, &GPSReceiverSession::configurationStarted, this, &GPSManager::_startReceiverCorrections);
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, this, [this]() {
        if (!_receiverSession.hasReceiver()) {
            _stopReceiverCorrections();
        }
    });
    connect(&_receiverSession, &GPSReceiverSession::connectionError, this, &GPSManager::_stopReceiverCorrections);
    connect(&_receiverSession, &GPSReceiverSession::rtcmFrameReceived, this,
            [this](const QByteArray& data, qint64 receivedAtMs, quint64 sessionId) {
                if (_shutdown || !sessionId || sessionId != _receiverCorrectionAttemptId ||
                    sessionId != _receiverSession.sessionId() || !_receiverSession.hasReceiver()) {
                    return;
                }
                const auto token = _receiverCorrectionSource.token();
                _corrections.acceptIngress(token.event(data, receivedAtMs, 0, true));
            });
    connect(&_receiverSession, &GPSReceiverSession::receiverTypeChanged, this, [settings](GPSType type) {
        const auto capabilities = GPSReceiverCapabilities::forType(type);
        if (capabilities.recognized()) {
            settings->rtkSettings()->baseReceiverManufacturers()->setRawValue(capabilities.manufacturerId);
        }
    });
    _nmeaSources = new NMEASourceManager(this);
    _nmeaSources->setSuspended(_connectionsSuspended && _connectionsSuspended());
    auto* nmeaSettings = settings->autoConnectSettings();
    for (Fact* fact :
         {nmeaSettings->nmeaSource(), nmeaSettings->autoConnectNmeaPort(), nmeaSettings->autoConnectNmeaBaud(),
          nmeaSettings->nmeaUdpPort(), nmeaSettings->nmeaTcpHost(), nmeaSettings->nmeaTcpPort(),
          nmeaSettings->nmeaReceiverMode(), nmeaSettings->nmeaAutoConnect()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSManager::_updateNmeaSettings);
    }
    _updateNmeaSettings();
    connect(_nmeaSources, &NMEASourceManager::positionSourceChanged, this, &GPSManager::_updateNmeaPositionSource);
    _nmeaSources->setRecordingBuffer(_recording.buffer());
    _receiverSession.setRecordingBuffer(_recording.buffer());
    connect(&_receiverSession, &GPSReceiverSession::configurationStarted, this, [this]() {
        if (_shutdown) {
            return;
        }
        const QPointer<GPSManager> guard(this);
        const quint64 session = _receiverSession.sessionId();
        _satellites.beginSession(QStringLiteral("nativeReceiver"), session);
        if (guard && !_shutdown && session == _receiverSession.sessionId()) {
            _relativePosition.beginSession(QStringLiteral("nativeReceiver"), session);
        }
    });
    connect(_receiver, &GPSReceiver::satellitesReceived, &_satellites, &GPSSatelliteModel::updateObservation);
    connect(&_receiverSession, &GPSReceiverSession::relativePositionReceived, &_relativePosition,
            &GPSRelativePositionModel::updateObservation);
    connect(&_receiverSession, &GPSReceiverSession::disconnected, this, [this]() {
        const QPointer<GPSManager> guard(this);
        const quint64 session = _receiverSession.sessionId();
        _satellites.reset();
        if (guard && session == _receiverSession.sessionId()) {
            _relativePosition.reset();
        }
    });
    connect(_nmeaSources, &NMEASourceManager::satellitesReceived, this, &GPSManager::_updateNmeaSatellites);
    connect(_nmeaSources, &NMEASourceManager::stateChanged, this, &GPSManager::_updateNmeaSatellites);
    connect(settings->gpsPositionSettings()->sourceMode(), &Fact::rawValueChanged, this,
            &GPSManager::_updatePositionSourceMode);
    _updatePositionSourceMode();
    _receiverAutoConnect = new GPSReceiverAutoConnect(&_receiverSession, _receiver->health(), this);
    _receiverAutoConnect->setSuspended(_connectionsSuspended && _connectionsSuspended());
    auto* receiverSettings = settings->rtkSettings();
    for (Fact* fact : {receiverSettings->connectionType(), receiverSettings->serialDevice(),
                       receiverSettings->networkReceiverType(), receiverSettings->receiverRole()}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() { _updateReceiverSettings(true); });
    }
    for (Fact* fact :
         {receiverSettings->networkBaseHost(), receiverSettings->networkBasePort(), receiverSettings->udpLocalPort(),
          receiverSettings->useFixedBasePosition(), receiverSettings->surveyInAccuracyLimit(),
          receiverSettings->surveyInMinObservationDuration(), receiverSettings->fixedBasePositionLatitude(),
          receiverSettings->fixedBasePositionLongitude(), receiverSettings->fixedBasePositionAltitude(),
          receiverSettings->fixedBasePositionAccuracy(), receiverSettings->constellationMask(),
          receiverSettings->dynamicModel(), receiverSettings->outputRateHz(), receiverSettings->headingOffsetDeg(),
          settings->autoConnectSettings()->autoConnectRTKGPS(),
          settings->autoConnectSettings()->autoConnectNetworkRTKGPS()}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() { _updateReceiverSettings(); });
    }
    _updateReceiverSettings();
    connect(&_receiverSession, &GPSReceiverSession::capabilitiesUpdated, this, &GPSManager::receiverSettingsChanged);
    connect(&_receiverSession, &GPSReceiverSession::configurationReported, this,
            &GPSManager::configurationReportChanged);
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, this, &GPSManager::receiverSettingsChanged);
    connect(settings->rtkSettings()->useReceiverPosition(), &Fact::rawValueChanged, this,
            &GPSManager::_updatePositionSource);
    connect(_receiver, &GPSReceiver::connectedChanged, this, &GPSManager::_updatePositionSource);
    connect(_receiverAutoConnect, &GPSReceiverAutoConnect::networkActiveChanged, this,
            &GPSManager::networkRtkActiveChanged);
    connect(_receiverAutoConnect, &GPSReceiverAutoConnect::networkAutoConnectPausedChanged, this,
            &GPSManager::networkRtkAutoConnectPausedChanged);
}

GPSManager::~GPSManager()
{
    qCDebug(GPSManagerLog) << this;

    shutdown();
    delete _receiverAutoConnect;
    delete _baseStationState;
    delete _receiver;
}

GPSManager *GPSManager::instance()
{
    return _gpsManager();
}

void GPSManager::init(NTRIPManager* ntrip)
{
    if (_initialized || _shutdown) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    _initialized = true;
    auto* correctionSettings = _settings.gpsCorrectionSettings();
    _corrections.init(correctionSettings);
    if (!guard || _shutdown) {
        return;
    }
    for (Fact* fact : {correctionSettings->correctionSource(), correctionSettings->correctionSourceInstance(),
                       correctionSettings->injectLocalReceiver()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSManager::_updateCorrectionSettings);
    }
    connect(&_corrections, &GPSCorrectionManager::selectedSourceChanged, &_receiverSession,
            &GPSReceiverSession::clearPendingCorrections);
    _updateCorrectionSettings();
    if (!guard || _shutdown) {
        return;
    }
    connect(&_receiverSession, &GPSReceiverSession::correctionDeliveriesReady, &_corrections,
            &GPSCorrectionManager::recordDeliveries);
    const auto invalidateDestination = [this]() {
        if (_correctionDestinationSession != 0) {
            const quint64 retiredSession = std::exchange(_correctionDestinationSession, 0);
            _corrections.invalidateDestination(QStringLiteral("localReceiver"), retiredSession);
        }
    };
    connect(&_receiverSession, &GPSReceiverSession::disconnected, this, invalidateDestination);
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, this, [this, invalidateDestination]() {
        if (!_receiverSession.hasReceiver() || _correctionDestinationSession != _receiverSession.sessionId()) {
            invalidateDestination();
        }
    });
    _ntrip = ntrip;
    if (_ntrip) {
        const QPointer<GPSManager> owner(this);
        using Source = NTRIPGgaProvider::PositionSource;
        _ntrip->setPositionProvider(Source::VehicleGPS, [owner]() {
            return owner && owner->_vehiclePositionProvider
                       ? PositionResult{owner->_vehiclePositionProvider->gpsPosition(), QStringLiteral("Vehicle GPS")}
                       : PositionResult{};
        });
        _ntrip->setPositionProvider(Source::VehicleEKF, [owner]() {
            return owner && owner->_vehiclePositionProvider
                       ? PositionResult{owner->_vehiclePositionProvider->ekfPosition(), QStringLiteral("Vehicle EKF")}
                       : PositionResult{};
        });
        _ntrip->setPositionProvider(Source::RTKBase, [owner]() {
            if (!owner || owner->_shutdown) {
                return PositionResult{};
            }
            const auto reference = owner->_baseStationState->reference();
            const auto coordinate = reference.observation.position.coordinate();
            const bool accepted = reference.isValid() && (coordinate.latitude() != 0 || coordinate.longitude() != 0);
            return accepted ? PositionResult{reference.observation, QStringLiteral("RTK Base"), true}
                            : PositionResult{};
        });
        _ntrip->setPositionProvider(Source::GCSPosition, [owner]() {
            if (!owner || owner->_shutdown || !owner->_positionManager) {
                return PositionResult{};
            }
            const auto observation = owner->_positionManager->acceptedObservation(GPSObservation::PositionUse::NTRIP);
            if (!observation) {
                return PositionResult{};
            }
            const auto coordinate = observation->position.coordinate();
            return coordinate.latitude() != 0 || coordinate.longitude() != 0
                       ? PositionResult{*observation, QStringLiteral("GCS Position")}
                       : PositionResult{};
        });
        auto* ntripSettings = _settings.ntripSettings();
        for (Fact* fact : {ntripSettings->ntripServerConnectEnabled(), ntripSettings->ntripUdpForwardEnabled(),
                           ntripSettings->ntripUdpTargetAddress(), ntripSettings->ntripUdpTargetPort()}) {
            connect(fact, &Fact::rawValueChanged, this, &GPSManager::_updateNtripUdpOutput);
        }
        _updateNtripUdpOutput();
        if (!guard || _shutdown) {
            return;
        }
        connect(_ntrip, &NTRIPManager::correctionSessionStarted, this, &GPSManager::_startNtripCorrections);
        connect(_ntrip, &NTRIPManager::correctionSessionEnded, this, &GPSManager::_stopNtripCorrections);
        connect(_ntrip, &NTRIPManager::correctionRejectedAt, this,
                [this](const QByteArray& data, int messageId, qint64 receivedAtMs, quint64 attemptId) {
                    if (!_shutdown && _ntrip && attemptId != 0 && attemptId == _ntripAttemptId &&
                        attemptId == _ntrip->correctionAttemptId()) {
                        const auto token = _ntripCorrectionSource.token();
                        _corrections.acceptIngress(
                            token.event(data, receivedAtMs, messageId, false, true, GPSCorrectionReason::InvalidFrame));
                    }
                });
        connect(_ntrip, &NTRIPManager::correctionReceivedAt, this,
                [this](const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs, quint64 attemptId) {
                    if (!_shutdown && _ntrip && attemptId != 0 && attemptId == _ntripAttemptId &&
                        attemptId == _ntrip->correctionAttemptId()) {
                        const auto token = _ntripCorrectionSource.token();
                        _corrections.acceptIngress(token.event(data, receivedAtMs, messageId, true, filtered));
                    }
                });
    }
#ifndef QGC_NO_SERIAL_LINK
    auto* serialDiscovery = SerialPortManager::instance();
    _receiverAutoConnect->setSerialDiscovery(serialDiscovery);
    _nmeaSources->setSerialDiscovery(serialDiscovery);
#endif
    if (_ntrip) {
        _ntrip->init();
    }
    if (!guard || _shutdown) {
        return;
    }
    _updateNmeaSettings();
    if (!guard || _shutdown) {
        return;
    }
    _updateReceiverSettings();
    if (guard && !_shutdown) {
        _updateConnections();
    }
}

void GPSManager::_startReceiverCorrections()
{
    if (_shutdown || !_receiverSession.hasReceiver()) {
        return;
    }
    const quint64 attempt = _receiverSession.sessionId();
    const quint64 revision = ++_receiverCorrectionRevision;
    const QPointer<GPSManager> guard(this);
    _receiverCorrectionAttemptId = attempt;
    _receiverCorrectionSource.reset();
    if (!guard || _shutdown || revision != _receiverCorrectionRevision || attempt != _receiverSession.sessionId() ||
        !_receiverSession.hasReceiver()) {
        return;
    }
    auto registration = _corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    if (guard && !_shutdown && revision == _receiverCorrectionRevision && attempt == _receiverSession.sessionId()) {
        _receiverCorrectionSource = std::move(registration);
    }
}

void GPSManager::_stopReceiverCorrections()
{
    ++_receiverCorrectionRevision;
    _receiverCorrectionAttemptId = 0;
    _receiverCorrectionSource.reset();
}

void GPSManager::_startNtripCorrections(quint64 attemptId, const QString& sourceId)
{
    if (_shutdown || !_ntrip || !attemptId || attemptId != _ntrip->correctionAttemptId()) {
        return;
    }
    const quint64 revision = ++_ntripCorrectionRevision;
    const QPointer<GPSManager> guard(this);
    _ntripAttemptId = attemptId;
    _ntripCorrectionSource.reset();
    if (!guard || _shutdown || revision != _ntripCorrectionRevision || !_ntrip ||
        attemptId != _ntrip->correctionAttemptId()) {
        return;
    }
    auto registration = _corrections.registerSource(GPSCorrectionSource::Ntrip, sourceId);
    if (guard && !_shutdown && revision == _ntripCorrectionRevision && _ntrip &&
        attemptId == _ntrip->correctionAttemptId()) {
        _ntripCorrectionSource = std::move(registration);
    }
}

void GPSManager::_stopNtripCorrections(quint64 attemptId)
{
    if (attemptId == _ntripAttemptId) {
        ++_ntripCorrectionRevision;
        _ntripAttemptId = 0;
        _ntripCorrectionSource.reset();
    }
}

void GPSManager::_updateConnections()
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    const bool suspended = _connectionsSuspended && _connectionsSuspended();
    _nmeaSources->setSuspended(suspended);
    if (!guard || _shutdown) {
        return;
    }
    _receiverAutoConnect->setSuspended(suspended);
    if (!guard || _shutdown || suspended) {
        return;
    }
    _nmeaSources->update();
    if (guard && !_shutdown) {
        _receiverAutoConnect->update();
    }
}

void GPSManager::_updatePositionSource()
{
    const QPointer<QObject> source = !_shutdown && _positionManager && _receiver->connected() &&
                                             _settings.rtkSettings()->useReceiverPosition()->rawValue().toBool()
                                         ? _receiver
                                         : nullptr;
    const quint64 session = source ? _receiverSession.sessionId() : 0;
    if (_registeredReceiverSource == source && _registeredReceiverSession == session) {
        return;
    }
    const quint64 revision = ++_receiverRegistrationRevision;
    const QPointer<GPSManager> guard(this);
    _registeredReceiverSource = source;
    _registeredReceiverSession = session;
    _receiverRegistration.reset();
    if (!guard || revision != _receiverRegistrationRevision || !source || !_positionManager || _shutdown) {
        return;
    }
    auto registration = _positionManager->registerPositionSource(QGCPositionManager::SelectedSource::Receiver, source,
                                                                 _receiver->health(), session);
    if (guard && revision == _receiverRegistrationRevision && !_shutdown) {
        if (!registration) {
            _registeredReceiverSource = nullptr;
            _registeredReceiverSession = 0;
        }
        _receiverRegistration = std::move(registration);
    }
}

void GPSManager::_updateNmeaPositionSource()
{
    const QPointer<QObject> source =
        !_shutdown && _positionManager && _nmeaSources->positionSource() ? _nmeaSources : nullptr;
    const quint64 session = source ? _nmeaSources->sessionId() : 0;
    if (_registeredNmeaSource == source && _registeredNmeaSession == session) {
        return;
    }
    const quint64 revision = ++_nmeaRegistrationRevision;
    const QPointer<GPSManager> guard(this);
    _registeredNmeaSource = source;
    _registeredNmeaSession = session;
    _nmeaRegistration.reset();
    if (!guard || revision != _nmeaRegistrationRevision || !source || !_positionManager || _shutdown) {
        return;
    }
    auto registration = _positionManager->registerPositionSource(QGCPositionManager::SelectedSource::Nmea, source,
                                                                 _nmeaSources->health(), session);
    if (guard && revision == _nmeaRegistrationRevision && !_shutdown) {
        if (!registration) {
            _registeredNmeaSource = nullptr;
            _registeredNmeaSession = 0;
        }
        _nmeaRegistration = std::move(registration);
    }
}

void GPSManager::_updatePositionSourceMode()
{
    if (!_shutdown && _positionManager) {
        _positionManager->setSourceMode(static_cast<QGCPositionManager::SourceMode>(
            _settings.gpsPositionSettings()->sourceMode()->rawValue().toInt()));
    }
}

void GPSManager::_updateNmeaSatellites()
{
    if (_shutdown || !_nmeaSources->positionSource()) {
        _nmeaSatellites.reset();
        return;
    }
    const quint64 session = _nmeaSources->sessionId();
    const QPointer<GPSManager> guard(this);
    if (_nmeaSatellites.sessionId() != session) {
        _nmeaSatellites.beginSession(QStringLiteral("nmeaReceiver"), session);
    }
    if (!guard || _shutdown || session != _nmeaSources->sessionId() || !_nmeaSources->positionSource()) {
        return;
    }
    _nmeaSatellites.updateObservation(_nmeaSources->satelliteObservation());
}

bool GPSManager::connectNmea()
{
    return !_shutdown && (!_connectionsSuspended || !_connectionsSuspended()) && _nmeaSources->connectSource();
}

void GPSManager::disconnectNmea()
{
    _nmeaSources->disconnectSource();
}

bool GPSManager::connectRtk()
{
    return !_shutdown && (!_connectionsSuspended || !_connectionsSuspended()) &&
           _receiverAutoConnect->connectSelected();
}

void GPSManager::disconnectRtk()
{
    _receiverAutoConnect->disconnectSelected();
}

bool GPSManager::networkRtkActive() const
{
    return _receiverAutoConnect->networkActive();
}

bool GPSManager::networkRtkAutoConnectPaused() const
{
    return _receiverAutoConnect->networkAutoConnectPaused();
}

bool GPSManager::connectNetworkRtk()
{
    if (_shutdown || networkRtkActive() || (_connectionsSuspended && _connectionsSuspended())) {
        return false;
    }
    return _receiverAutoConnect->connectNetwork();
}

void GPSManager::disconnectNetworkRtk()
{
    _receiverAutoConnect->disconnectNetwork();
}

void GPSManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    const QPointer<GPSManager> guard(this);
    _stopReceiverCorrections();
    if (!guard) {
        return;
    }
    _stopNtripCorrections(_ntripAttemptId);
    if (!guard) {
        return;
    }
    ++_receiverRegistrationRevision;
    ++_nmeaRegistrationRevision;
    _registeredReceiverSource = nullptr;
    _registeredNmeaSource = nullptr;
    _receiverRegistration.reset();
    if (!guard) {
        return;
    }
    _nmeaRegistration.reset();
    if (!guard) {
        return;
    }
    const QPointer<NTRIPManager> ntrip = std::exchange(_ntrip, nullptr);
    if (ntrip) {
        ntrip->disconnect(this);
        ntrip->stopNTRIP();
        if (!guard) {
            return;
        }
    }
    if (_nmeaSources) {
        _nmeaSources->shutdown();
        if (!guard) {
            return;
        }
    }
    _receiverAutoConnect->stop();
    if (!guard) {
        return;
    }
    _receiverSession.shutdown();
    if (!guard) {
        return;
    }
    _recording.stop();
    if (!guard) {
        return;
    }
    _satellites.reset();
    if (!guard) {
        return;
    }
    _nmeaSatellites.reset();
    if (!guard) {
        return;
    }
    _relativePosition.reset();
    if (!guard) {
        return;
    }
    _corrections.removeSink(QStringLiteral("localReceiver"));
    if (!guard) {
        return;
    }
    _corrections.shutdown();
}

void GPSManager::_updateNmeaSettings()
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    const quint64 revision = ++_nmeaSettingsRevision;
    auto* settings = _settings.autoConnectSettings();
    const auto profile = NMEAConnectionConfig::fromSettings(*settings).profile();
    const bool automatic = settings->nmeaAutoConnect()->rawValue().toBool();
    _nmeaSources->setProfile(profile);
    if (guard && !_shutdown && revision == _nmeaSettingsRevision) {
        _nmeaSources->setAutoConnect(automatic && _initialized);
    }
}

void GPSManager::_updateReceiverSettings(bool restart)
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    const quint64 revision = ++_receiverSettingsRevision;
    auto* settings = &_settings;
    const auto config = GPSConnectionSettings::fromSettings(*settings->rtkSettings());
    const bool automatic = config.transport == GPSConnectionConfig::Serial
                               ? settings->autoConnectSettings()->autoConnectRTKGPS()->rawValue().toBool()
                               : settings->autoConnectSettings()->autoConnectNetworkRTKGPS()->rawValue().toBool();
    _receiverAutoConnect->setProfile(config.profile(), restart);
    if (!guard || _shutdown || revision != _receiverSettingsRevision) {
        return;
    }
    _receiverAutoConnect->setAutoConnect(automatic && _initialized);
    if (guard && !_shutdown && revision == _receiverSettingsRevision && restart) {
        emit receiverSettingsChanged();
    }
}

QVariantList GPSManager::receiverSettings() const
{
    auto* settings = _settings.rtkSettings();
    const auto capabilities = _receiverSession.hasReceiver()
                                  ? _receiverSession.capabilities()
                                  : GPSReceiverCapabilities::forType(
                                        static_cast<GPSType>(settings->networkReceiverType()->rawValue().toInt()));
    const bool baseStation = settings->receiverRole()->rawValue().toInt() == RTKSettings::RTKBase;
    return GPSReceiverSettingsPresentation::settingDescriptors(capabilities, baseStation);
}

QVariantList GPSManager::configurationReport() const
{
    return GPSReceiverSettingsPresentation::configurationReport(_receiverSession.configurationReport());
}

void GPSManager::_updateCorrectionSettings()
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    const quint64 revision = ++_correctionSettingsRevision;
    auto* settings = _settings.gpsCorrectionSettings();
    const bool injectReceiver = settings->injectLocalReceiver()->rawValue().toBool();
    const int source = settings->correctionSource()->rawValue().toInt();
    GPSCorrectionManager::RoutingConfiguration configuration;
    configuration.instance = settings->correctionSourceInstance()->rawValue().toString();
    if (source >= GPSCorrectionSettings::LocalReceiver && source <= GPSCorrectionSettings::Udp) {
        configuration.source = source == GPSCorrectionSettings::LocalReceiver ? GPSCorrectionSource::LocalReceiver
                               : source == GPSCorrectionSettings::Ntrip       ? GPSCorrectionSource::Ntrip
                                                                              : GPSCorrectionSource::Udp;
        configuration.policy = GPSCorrectionManager::RoutingPolicy::Manual;
    } else {
        configuration.policy = source == GPSCorrectionSettings::All ? GPSCorrectionManager::RoutingPolicy::All
                                                                    : GPSCorrectionManager::RoutingPolicy::Automatic;
    }
    _receiverSession.clearPendingCorrections();
    if (!guard || _shutdown || revision != _correctionSettingsRevision) {
        return;
    }
    _corrections.applyRoutingConfiguration(configuration);
    if (!guard || _shutdown || revision != _correctionSettingsRevision) {
        return;
    }
    if (injectReceiver) {
        _corrections.addDetailedSink(QStringLiteral("localReceiver"), [this](const GPSCorrectionFrame& frame) {
            const quint64 session = _receiverSession.sessionId();
            if (_shutdown || frame.source == GPSCorrectionSource::LocalReceiver || !frame.validated ||
                !_settings.gpsCorrectionSettings()->injectLocalReceiver()->rawValue().toBool() ||
                !_receiverSession.readyForCorrections()) {
                return GPSCorrectionRouter::Submission{0, session, GPSCorrectionReason::DestinationUnavailable};
            }
            _correctionDestinationSession = session;
            const auto result = _receiverSession.submitCorrections(frame, session);
            return GPSCorrectionRouter::Submission{
                result.accepted ? static_cast<quint64>(frame.data.size()) : 0, session,
                result.accepted ? GPSCorrectionReason::None : gpsCorrectionReason(result.outcome)};
        });
    } else {
        _corrections.removeSink(QStringLiteral("localReceiver"));
    }
}

void GPSManager::_updateNtripUdpOutput()
{
    if (_shutdown) {
        return;
    }
    auto* settings = _settings.ntripSettings();
    const bool enabled = _ntrip && settings->ntripServerConnectEnabled()->rawValue().toBool() &&
                         settings->ntripUdpForwardEnabled()->rawValue().toBool();
    _corrections.configureNtripUdpOutput(enabled, settings->ntripUdpTargetAddress()->rawValue().toString(),
                                         settings->ntripUdpTargetPort()->rawValue().toUInt());
}
