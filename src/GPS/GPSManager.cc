#include "GPSManager.h"

#include "AppMessages.h"
#include "AutoConnectSettings.h"
#include "GPSBaseStationState.h"
#include "GPSConnectionSettings.h"
#include "GPSReceiver.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverPositionSource.h"
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
            [this](const QByteArray& data, qint64 receivedAtMs) {
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
    _nmeaSources = new NMEASourceManager(settings->autoConnectSettings(), _positionManager, this);
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
          receiverSettings->fixedBasePositionAccuracy(), settings->autoConnectSettings()->autoConnectRTKGPS(),
          settings->autoConnectSettings()->autoConnectNetworkRTKGPS()}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() { _updateReceiverSettings(); });
    }
    _updateReceiverSettings();
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
    auto* correctionSettings = _settings.ntripSettings();
    _corrections.init(correctionSettings);
    for (Fact* fact : {correctionSettings->correctionSource(), correctionSettings->correctionSourceInstance(),
                       correctionSettings->injectLocalReceiver()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSManager::_updateCorrectionSettings);
    }
    connect(&_corrections, &GPSCorrectionManager::selectedSourceChanged, &_receiverSession,
            &GPSReceiverSession::clearPendingCorrections);
    _updateCorrectionSettings();
    _corrections.addSink(QStringLiteral("localReceiver"), [this](const GPSCorrectionFrame& frame) -> quint64 {
        if (_shutdown || frame.source == GPSCorrectionSource::LocalReceiver || !frame.validated ||
            !_settings.ntripSettings()->injectLocalReceiver()->rawValue().toBool() ||
            !_receiverSession.readyForCorrections()) {
            return 0;
        }
        return _receiverSession.submitCorrections(frame.data, frame.receivedAtMs, _receiverSession.sessionId())
                   ? static_cast<quint64>(frame.data.size())
                   : 0;
    });
    _ntrip = ntrip;
    if (_ntrip) {
        connect(_ntrip, &NTRIPManager::correctionSessionStarted, this, [this]() {
            if (!_shutdown && _ntrip) {
                _corrections.beginSourceSession(GPSCorrectionSource::Ntrip, _ntrip->correctionSourceId());
            }
        });
        connect(_ntrip, &NTRIPManager::correctionSessionEnded, this,
                [this]() { _corrections.endSourceSession(GPSCorrectionSource::Ntrip); });
        connect(_ntrip, &NTRIPManager::correctionReceivedAt, this,
                [this](const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs) {
                    if (!_shutdown) {
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
    _nmeaSources->update();
    _receiverAutoConnect->update();
}

void GPSManager::_updatePositionSource()
{
    if (!_positionManager) {
        _positionSourceInstalled = false;
        return;
    }
    const bool useReceiver = _settings.rtkSettings()->useReceiverPosition()->rawValue().toBool();
    qCDebug(GPSManagerLog) << "Ground-station receiver position selection"
                           << "enabled:" << useReceiver << "connected:" << _receiver->connected();
    if (useReceiver && _receiver->connected()) {
        _positionManager->setReceiverPositionSource(_receiver->positionSource(), _receiver->health());
        _positionSourceInstalled = true;
    } else if (_positionSourceInstalled) {
        _positionManager->clearReceiverPositionSource(_receiver->positionSource());
        _positionSourceInstalled = false;
    }
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
    if (_ntrip) {
        _ntrip->stopNTRIP();
        _ntrip->disconnect(this);
        _ntrip = nullptr;
    }
    _corrections.removeSink(QStringLiteral("localReceiver"));
    if (_connectionTimer) {
        _connectionTimer->stop();
    }
    if (_nmeaSources) {
        _nmeaSources->shutdown();
    }
    _receiverAutoConnect->stop();
    _receiverSession.shutdown();
    _corrections.shutdown();
}

void GPSManager::_updateReceiverSettings(bool restart)
{
    if (_shutdown) {
        return;
    }
    auto* settings = &_settings;
    const auto config = GPSConnectionSettings::fromSettings(*settings->rtkSettings());
    _receiverAutoConnect->setConfig(config, restart);
    const bool automatic = config.transport == GPSConnectionConfig::Serial
                               ? settings->autoConnectSettings()->autoConnectRTKGPS()->rawValue().toBool()
                               : settings->autoConnectSettings()->autoConnectNetworkRTKGPS()->rawValue().toBool();
    _receiverAutoConnect->setAutoConnect(automatic);
}

void GPSManager::_updateCorrectionSettings()
{
    if (_shutdown) {
        return;
    }
    _receiverSession.clearPendingCorrections();
    auto* settings = _settings.ntripSettings();
    const int source = settings->correctionSource()->rawValue().toInt();
    if (source >= NTRIPSettings::LocalReceiver && source <= NTRIPSettings::Udp) {
        const auto category = source == NTRIPSettings::LocalReceiver ? GPSCorrectionSource::LocalReceiver
                              : source == NTRIPSettings::Ntrip       ? GPSCorrectionSource::Ntrip
                                                                     : GPSCorrectionSource::Udp;
        _corrections.setSelectedSource(category);
        _corrections.setSelectedInstance(settings->correctionSourceInstance()->rawValue().toString());
        _corrections.setRoutingPolicy(GPSCorrectionManager::RoutingPolicy::Manual);
    } else {
        _corrections.setRoutingPolicy(source == NTRIPSettings::All ? GPSCorrectionManager::RoutingPolicy::All
                                                                   : GPSCorrectionManager::RoutingPolicy::Automatic);
    }
}
