#include "GPSManager.h"

#include "AppMessages.h"
#include "GPSCorrectionManager.h"
#include "GPSMavlinkOutput.h"
#include "GPSRtk.h"
#include "LinkManager.h"
#include "NMEASourceManager.h"
#include "NTRIPManager.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "RTKAutoConnect.h"
#include "SerialPortManager.h"
#endif
#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject* parent)
    : QObject(parent)
    , _corrections(new GPSCorrectionManager(this))
    , _gpsRtk(new GPSRtk(this))
    , _ntripManager(NTRIPManager::instance())
{
    qCDebug(GPSManagerLog) << this;
    auto* output = new GPSMavlinkOutput(this);
    _corrections->rtcmMavlink()->setOutputProvider([output]() { return output->outputs(); });
    _gpsRtk->setCorrectionManager(_corrections);
    _ntripManager->setCorrectionManager(_corrections);
}

GPSManager::~GPSManager()
{
    shutdown();
    qCDebug(GPSManagerLog) << this;
}

GPSManager* GPSManager::instance()
{
    return _gpsManager();
}

void GPSManager::init()
{
    if (_connectionTimer || _shutdown) {
        return;
    }
    _corrections->init(SettingsManager::instance()->gpsCorrectionSettings());
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    _nmeaSources = new NMEASourceManager(settings, QGCPositionManager::instance(), this);
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
#ifndef QGC_NO_SERIAL_LINK
    _rtkAutoConnect->update();
#endif
}

void GPSManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    if (_connectionTimer) {
        _connectionTimer->stop();
    }
    if (_nmeaSources) {
        _nmeaSources->stop();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_rtkAutoConnect) {
        _rtkAutoConnect->stop();
    }
#endif
    _gpsRtk->disconnectGPS();
    if (_ntripManager) {
        _ntripManager->stopNTRIP();
    }
    _corrections->shutdown();
}
