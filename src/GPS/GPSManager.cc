#include "GPSManager.h"

#include "AppMessages.h"
#include "AutoConnectSettings.h"
#include "GPSBaseStationState.h"
#include "GPSConnectionSettings.h"
#include "GPSCorrectionSettings.h"
#include "GPSPositionSettings.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSReceiver.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverPositionSource.h"
#include "GPSReceiverSettingsPresentation.h"
#include "LinkManager.h"
#include "NMEASourceManager.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif
#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

#include <utility>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject* parent)
    : GPSManager(
          *SettingsManager::instance(), QGCPositionManager::instance(),
          []() { return LinkManager::instance()->connectionsSuspended(); }, parent)
{
    qCDebug(GPSManagerLog) << this;
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
    connect(&_receiverSession, &GPSReceiverSession::configurationStarted, &_corrections,
            [this]() { _corrections.beginSourceSession(GPSCorrectionSource::LocalReceiver); });
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, &_corrections, [this]() {
        if (!_receiverSession.hasReceiver()) {
            _corrections.endSourceSession(GPSCorrectionSource::LocalReceiver);
        }
    });
    connect(&_receiverSession, &GPSReceiverSession::connectionError, &_corrections,
            [this]() { _corrections.endSourceSession(GPSCorrectionSource::LocalReceiver); });
    connect(&_receiverSession, &GPSReceiverSession::rtcmFrameReceived, &_corrections,
            [this](const QByteArray& data, qint64 receivedAtMs, quint64 sessionId) {
                if (_shutdown || !sessionId || !_receiverSession.hasReceiver() ||
                    sessionId != _receiverSession.sessionId()) {
                    return;
                }
                _corrections.acceptFrame({GPSCorrectionSource::LocalReceiver,
                                          _corrections.sourceSession(GPSCorrectionSource::LocalReceiver), receivedAtMs,
                                          data, 0, true, false});
            });
    connect(&_receiverSession, &GPSReceiverSession::receiverTypeChanged, this, [settings](GPSType type) {
        const auto capabilities = GPSReceiverCapabilities::forType(type);
        if (capabilities.recognized()) {
            settings->rtkSettings()->baseReceiverManufacturers()->setRawValue(capabilities.manufacturerId);
        }
    });
    _nmeaSources = new NMEASourceManager(settings->autoConnectSettings(), this);
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
    if (_connectionTimer || _shutdown) {
        return;
    }
    auto* correctionSettings = _settings.gpsCorrectionSettings();
    _corrections.init(correctionSettings);
    for (Fact* fact : {correctionSettings->correctionSource(), correctionSettings->correctionSourceInstance(),
                       correctionSettings->injectLocalReceiver()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSManager::_updateCorrectionSettings);
    }
    connect(&_corrections, &GPSCorrectionManager::selectedSourceChanged, &_receiverSession,
            &GPSReceiverSession::clearPendingCorrections);
    _updateCorrectionSettings();
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
        auto* ntripSettings = _settings.ntripSettings();
        for (Fact* fact : {ntripSettings->ntripServerConnectEnabled(), ntripSettings->ntripUdpForwardEnabled(),
                           ntripSettings->ntripUdpTargetAddress(), ntripSettings->ntripUdpTargetPort()}) {
            connect(fact, &Fact::rawValueChanged, this, &GPSManager::_updateNtripUdpOutput);
        }
        _updateNtripUdpOutput();
        connect(_ntrip, &NTRIPManager::correctionSessionStarted, this,
                [this](quint64 attemptId, const QString& sourceId) {
                    if (!_shutdown && _ntrip && attemptId != 0 && attemptId == _ntrip->correctionAttemptId()) {
                        _ntripAttemptId = attemptId;
                        _corrections.beginSourceSession(GPSCorrectionSource::Ntrip, sourceId);
                    }
                });
        connect(_ntrip, &NTRIPManager::correctionSessionEnded, this, [this](quint64 attemptId) {
            if (!_shutdown && attemptId == _ntripAttemptId) {
                _ntripAttemptId = 0;
                _corrections.endSourceSession(GPSCorrectionSource::Ntrip);
            }
        });
        connect(_ntrip, &NTRIPManager::correctionRejectedAt, this,
                [this](const QByteArray& data, int messageId, qint64 receivedAtMs, quint64 attemptId) {
                    if (!_shutdown && _ntrip && attemptId != 0 && attemptId == _ntripAttemptId &&
                        attemptId == _ntrip->correctionAttemptId()) {
                        _corrections.recordRejectedFrame(
                            {GPSCorrectionSource::Ntrip, _corrections.sourceSession(GPSCorrectionSource::Ntrip),
                             receivedAtMs, data, messageId, false, true, _ntrip->correctionSourceId()},
                            GPSCorrectionReason::InvalidFrame);
                    }
                });
        connect(_ntrip, &NTRIPManager::correctionReceivedAt, this,
                [this](const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs, quint64 attemptId) {
                    if (!_shutdown && _ntrip && attemptId != 0 && attemptId == _ntripAttemptId &&
                        attemptId == _ntrip->correctionAttemptId()) {
                        _corrections.acceptFrame({GPSCorrectionSource::Ntrip,
                                                  _corrections.sourceSession(GPSCorrectionSource::Ntrip), receivedAtMs,
                                                  data, messageId, true, filtered});
                    }
                });
    }
#ifndef QGC_NO_SERIAL_LINK
    auto* serialDiscovery = SerialPortManager::instance();
    _receiverAutoConnect->setSerialDiscovery(serialDiscovery);
    _nmeaSources->setSerialDiscovery(serialDiscovery);
#endif
    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(1000);
    connect(_connectionTimer, &QTimer::timeout, this, &GPSManager::_updateConnections);
    if (!QGC::runningUnitTests()) {
        _connectionTimer->start();
    }
    if (_ntrip) {
        _ntrip->init();
    }
}

void GPSManager::_updateConnections()
{
    if (_shutdown || (_connectionsSuspended && _connectionsSuspended())) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    _nmeaSources->update();
    if (guard && !_shutdown) {
        _receiverAutoConnect->update();
    }
}

void GPSManager::_updatePositionSource()
{
    const QPointer<QGeoPositionInfoSource> source =
        !_shutdown && _positionManager && _receiver->connected() &&
                _settings.rtkSettings()->useReceiverPosition()->rawValue().toBool()
            ? _receiver->positionSource()
            : nullptr;
    if (_registeredReceiverSource == source) {
        return;
    }
    const quint64 revision = ++_receiverRegistrationRevision;
    const QPointer<GPSManager> guard(this);
    _registeredReceiverSource = source;
    _receiverRegistration.reset();
    if (!guard || revision != _receiverRegistrationRevision || !source || !_positionManager || _shutdown) {
        return;
    }
    auto registration = _positionManager->registerPositionSource(QGCPositionManager::SelectedSource::Receiver, source,
                                                                 _receiver->health());
    if (guard && revision == _receiverRegistrationRevision && !_shutdown) {
        if (!registration) {
            _registeredReceiverSource = nullptr;
        }
        _receiverRegistration = std::move(registration);
    }
}

void GPSManager::_updateNmeaPositionSource()
{
    const QPointer<QGeoPositionInfoSource> source =
        !_shutdown && _positionManager ? _nmeaSources->positionSource() : nullptr;
    if (_registeredNmeaSource == source) {
        return;
    }
    const quint64 revision = ++_nmeaRegistrationRevision;
    const QPointer<GPSManager> guard(this);
    _registeredNmeaSource = source;
    _nmeaRegistration.reset();
    if (!guard || revision != _nmeaRegistrationRevision || !source || !_positionManager || _shutdown) {
        return;
    }
    auto registration = _positionManager->registerPositionSource(QGCPositionManager::SelectedSource::Nmea, source,
                                                                 _nmeaSources->health());
    if (guard && revision == _nmeaRegistrationRevision && !_shutdown) {
        if (!registration) {
            _registeredNmeaSource = nullptr;
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
    _ntripAttemptId = 0;
    const QPointer<GPSManager> guard(this);
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
    if (_connectionTimer) {
        _connectionTimer->stop();
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
    _receiverAutoConnect->setAutoConnect(automatic);
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
    _receiverSession.clearPendingCorrections();
    auto* settings = _settings.gpsCorrectionSettings();
    const int source = settings->correctionSource()->rawValue().toInt();
    if (source >= GPSCorrectionSettings::LocalReceiver && source <= GPSCorrectionSettings::Udp) {
        const auto category = source == GPSCorrectionSettings::LocalReceiver ? GPSCorrectionSource::LocalReceiver
                              : source == GPSCorrectionSettings::Ntrip       ? GPSCorrectionSource::Ntrip
                                                                             : GPSCorrectionSource::Udp;
        _corrections.setSelectedSource(category);
        _corrections.setSelectedInstance(settings->correctionSourceInstance()->rawValue().toString());
        _corrections.setRoutingPolicy(GPSCorrectionManager::RoutingPolicy::Manual);
    } else {
        _corrections.setRoutingPolicy(source == GPSCorrectionSettings::All
                                          ? GPSCorrectionManager::RoutingPolicy::All
                                          : GPSCorrectionManager::RoutingPolicy::Automatic);
    }
    if (settings->injectLocalReceiver()->rawValue().toBool()) {
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
