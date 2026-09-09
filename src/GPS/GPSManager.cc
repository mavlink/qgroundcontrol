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
    : QObject(parent)
    , _positionManager(QGCPositionManager::instance())
    , _corrections(this)
    , _receiverSession(this)
    , _receiver(new GPSReceiver(_receiverSession, this))
    , _baseStationState(new GPSBaseStationState(_receiverSession, *_receiver->facts()->rtk(), this))
{
    qCDebug(GPSManagerLog) << this;

    auto* settings = SettingsManager::instance();
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

void GPSManager::init()
{
    if (_connectionTimer) {
        return;
    }
    _corrections.init(SettingsManager::instance()->ntripSettings());
#ifndef QGC_NO_SERIAL_LINK
    _receiverAutoConnect->setSerialDiscovery(SerialPortManager::instance());
#endif
    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(1000);
    connect(_connectionTimer, &QTimer::timeout, this, &GPSManager::_updateConnections);
    if (!QGC::runningUnitTests()) {
        _connectionTimer->start();
    }
}

void GPSManager::_updateConnections()
{
    if (LinkManager::instance()->connectionsSuspended()) {
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
    const bool useReceiver = SettingsManager::instance()->rtkSettings()->useReceiverPosition()->rawValue().toBool();
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
    return !LinkManager::instance()->connectionsSuspended() && _nmeaSources->connectSource();
}

void GPSManager::disconnectNmea()
{
    _nmeaSources->disconnectSource();
}

bool GPSManager::connectRtk()
{
    return !LinkManager::instance()->connectionsSuspended() && _receiverAutoConnect->connectSelected();
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
    if (networkRtkActive() || LinkManager::instance()->connectionsSuspended()) {
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
    auto* settings = SettingsManager::instance();
    const auto config = GPSConnectionSettings::fromSettings(*settings->rtkSettings());
    _receiverAutoConnect->setConfig(config, restart);
    const bool automatic = config.transport == GPSConnectionConfig::Serial
                               ? settings->autoConnectSettings()->autoConnectRTKGPS()->rawValue().toBool()
                               : settings->autoConnectSettings()->autoConnectNetworkRTKGPS()->rawValue().toBool();
    _receiverAutoConnect->setAutoConnect(automatic);
}
