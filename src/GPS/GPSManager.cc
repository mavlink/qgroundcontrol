#include "GPSManager.h"

#include "AppMessages.h"
#include "GPSRtk.h"
#include "LinkManager.h"
#include "NmeaSourceManager.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "TcpGPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "RTKAutoConnect.h"
#include "SerialPortManager.h"
#endif
#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include <algorithm>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject *parent)
    : QObject(parent)
    , _gpsRtk(new GPSRtk(this))
{
    qCDebug(GPSManagerLog) << this;
}

GPSManager::~GPSManager()
{
    shutdown();
    qCDebug(GPSManagerLog) << this;
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
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    _nmeaSources = new NmeaSourceManager(settings, QGCPositionManager::instance(), this);
#ifndef QGC_NO_SERIAL_LINK
    _rtkAutoConnect = new RTKAutoConnect(settings, _gpsRtk, SerialPortManager::instance(), this);
    connect(_rtkAutoConnect, &RTKAutoConnect::connectRequested, this,
            [this](const QString& device, const QString& name) { _gpsRtk->connectGPS(device, name); });
    connect(_rtkAutoConnect, &RTKAutoConnect::disconnectRequested, _gpsRtk, &GPSRtk::disconnectGPS);
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
    if (_networkRtkActive) {
        _updateNetworkRtk();
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    _rtkAutoConnect->update();
#endif
}

bool GPSManager::connectNetworkRtk()
{
    if (_networkRtkActive || LinkManager::instance()->connectionsSuspended()) {
        return false;
    }
    auto* settings = SettingsManager::instance()->rtkSettings();
    const QString host = settings->networkBaseHost()->rawValue().toString().trimmed();
    const int port = settings->networkBasePort()->rawValue().toInt();
    const int type = settings->networkReceiverType()->rawValue().toInt();
    QUrl endpoint;
    endpoint.setScheme(QStringLiteral("tcp"));
    endpoint.setHost(host);
    if (host.isEmpty() || !endpoint.isValid() || endpoint.host().isEmpty() || port < 1 || port > 65535 || type < 0 ||
        type > 3) {
        return false;
    }

#ifndef QGC_NO_SERIAL_LINK
    if (_rtkAutoConnect) {
        _rtkAutoConnect->stop();
    }
#endif
    _networkHost = endpoint.host();
    _networkPort = static_cast<quint16>(port);
    switch (type) {
        case 0:
            _networkType = GPSType::u_blox;
            break;
        case 1:
            _networkType = GPSType::trimble;
            break;
        case 2:
            _networkType = GPSType::septentrio;
            break;
        case 3:
            _networkType = GPSType::femto;
            break;
    }
    _networkRetryDelayMs = 1000;
    _networkRetryDeadline = QDeadlineTimer::Forever;
    _networkRtkActive = true;
    _startNetworkRtk();
    emit networkRtkActiveChanged();
    return true;
}

void GPSManager::_startNetworkRtk()
{
    _gpsRtk->connectReceiver(_networkType, [host = _networkHost, port = _networkPort](const std::atomic_bool& stop) {
        return std::make_unique<TcpGPSTransport>(host, port, stop);
    });
}

void GPSManager::_updateNetworkRtk()
{
    if (!_networkRtkActive) {
        return;
    }
    if (_gpsRtk->hasReceiver()) {
        _networkRetryDeadline = QDeadlineTimer::Forever;
        if (_gpsRtk->connected()) {
            _networkRetryDelayMs = 1000;
        }
        return;
    }
    if (_networkRetryDeadline.isForever()) {
        _networkRetryDeadline.setRemainingTime(_networkRetryDelayMs);
    }
    if (_networkRetryDeadline.hasExpired()) {
        _networkRetryDeadline = QDeadlineTimer::Forever;
        _networkRetryDelayMs = (std::min) (_networkRetryDelayMs * 2, kMaxNetworkRetryDelayMs);
        _startNetworkRtk();
    }
}

void GPSManager::disconnectNetworkRtk()
{
    if (!_networkRtkActive) {
        return;
    }
    _networkRtkActive = false;
    _networkRetryDeadline = QDeadlineTimer::Forever;
    _networkRetryDelayMs = 1000;
    _gpsRtk->disconnectGPS();
    emit networkRtkActiveChanged();
}

void GPSManager::shutdown()
{
    if (_connectionTimer) {
        _connectionTimer->stop();
    }
    if (_nmeaSources) {
        _nmeaSources->stop();
    }
    disconnectNetworkRtk();
#ifndef QGC_NO_SERIAL_LINK
    if (_rtkAutoConnect) {
        _rtkAutoConnect->stop();
    }
#endif
    _gpsRtk->disconnectGPS();
}
