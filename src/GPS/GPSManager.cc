#include "GPSManager.h"

#include "AppMessages.h"
#include "GPSRtk.h"
#include "LinkManager.h"
#include "NmeaSourceManager.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RTKAutoConnect.h"
#include "RTKPositionSource.h"
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
    , _gpsRtk(new GPSRtk(this))
{
    qCDebug(GPSManagerLog) << this;

    auto* settings = SettingsManager::instance();
    _nmeaSources = new NmeaSourceManager(settings->autoConnectSettings(), _positionManager, this);
    _rtkAutoConnect = new RTKAutoConnect(_gpsRtk, settings->autoConnectSettings(), settings->rtkSettings(), this);
    connect(settings->rtkSettings()->useReceiverPosition(), &Fact::rawValueChanged, this,
            &GPSManager::_updatePositionSource);
    connect(_gpsRtk, &GPSRtk::connectedChanged, this, &GPSManager::_updatePositionSource);
    connect(_rtkAutoConnect, &RTKAutoConnect::networkActiveChanged, this, &GPSManager::networkRtkActiveChanged);
    connect(_rtkAutoConnect, &RTKAutoConnect::networkAutoConnectPausedChanged, this,
            &GPSManager::networkRtkAutoConnectPausedChanged);
}

GPSManager::~GPSManager()
{
    qCDebug(GPSManagerLog) << this;

    shutdown();
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
#ifndef QGC_NO_SERIAL_LINK
    _rtkAutoConnect->setSerialDiscovery(SerialPortManager::instance());
    connect(_rtkAutoConnect, &RTKAutoConnect::connectRequested, this,
            [this](const QString& device, const QString& name) { _gpsRtk->connectGPS(device, name); });
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
    _rtkAutoConnect->update();
}

void GPSManager::_updatePositionSource()
{
    if (!_positionManager) {
        _positionSourceInstalled = false;
        return;
    }
    const bool useReceiver = SettingsManager::instance()->rtkSettings()->useReceiverPosition()->rawValue().toBool();
    qCDebug(GPSManagerLog) << "Ground-station receiver position selection"
                           << "enabled:" << useReceiver
                           << "connected:" << _gpsRtk->connected();
    if (useReceiver && _gpsRtk->connected()) {
        _positionManager->setReceiverPositionSource(_gpsRtk->positionSource());
        _positionSourceInstalled = true;
    } else if (_positionSourceInstalled) {
        _positionManager->clearReceiverPositionSource(_gpsRtk->positionSource());
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
    return !LinkManager::instance()->connectionsSuspended() && _rtkAutoConnect->connectSelected();
}

void GPSManager::disconnectRtk()
{
    _rtkAutoConnect->disconnectSelected();
}

bool GPSManager::networkRtkActive() const
{
    return _rtkAutoConnect->networkActive();
}

bool GPSManager::networkRtkAutoConnectPaused() const
{
    return _rtkAutoConnect->networkAutoConnectPaused();
}

bool GPSManager::connectNetworkRtk()
{
    if (networkRtkActive() || LinkManager::instance()->connectionsSuspended()) {
        return false;
    }
    return _rtkAutoConnect->connectNetwork();
}

void GPSManager::disconnectNetworkRtk()
{
    _rtkAutoConnect->disconnectNetwork();
}

void GPSManager::shutdown()
{
    if (_connectionTimer) {
        _connectionTimer->stop();
    }
    if (_nmeaSources) {
        _nmeaSources->stop();
    }
    _rtkAutoConnect->stop();
    _gpsRtk->disconnectGPS();
}
