#include "GPSManager.h"

#include <utility>

#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

#include "AppMessages.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSCorrectionStatus.h"
#include "GPSGgaSources.h"
#include "GPSMavlinkOutput.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSSettingsBindings.h"
#include "LinkManager.h"
#include "NTRIPManager.h"
#include "NTRIPNetworkMonitor.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RTKConnectionPolicy.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPortManagerAdapter.h"
#include "SerialPortManager.h"
#endif

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject* parent)
    : QObject(parent)
    , _corrections(new GPSCorrectionManager(this))
    , _gpsRtk(new GPSRtk(this))
    , _gpsRtkFacts(new GPSRTKFactGroup(_gpsRtk, this))
    , _ntripManager(new NTRIPManager(this))
    , _ntripNetworkMonitor(new QtNTRIPNetworkMonitor(this))
    , _positionManager(new QGCPositionManager(this))
    , _correctionStatus(
          new GPSCorrectionStatus(_corrections, _ntripManager, _gpsRtk,
                                  SettingsManager::instance()->gpsCorrectionSettings()->rtcmUdpInputEnabled(), this))
    , _ggaSources(new GPSGgaSources(_ntripManager, _gpsRtk, _positionManager, this))
{
    qCDebug(GPSManagerLog) << this;
    _corrections->rtcmMavlink()->setOutputProvider(createGpsMavlinkOutputProvider());
    _gpsRtk->setCorrectionManager(_corrections);
#ifndef QGC_NO_SERIAL_LINK
    _gpsRtk->setSerialPorts(new GPSSerialPortManagerAdapter(SerialPortManager::instance(), this));
#endif
    _ntripManager->setCorrectionManager(_corrections);
    _ntripManager->setNetworkMonitor(_ntripNetworkMonitor);
    connect(_correctionStatus, &GPSCorrectionStatus::stateChanged, this, &GPSManager::correctionStateChanged);
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

GPSManager::CorrectionState GPSManager::correctionState() const
{
    return _correctionStatus->state();
}

void GPSManager::init()
{
    if (_connectionTimer || _shutdown) {
        return;
    }
    _positionManager->init();
    _ggaSources->init();
    _gpsRtk->setPositionService(_positionManager);
    GPSSettingsBindings::bindRtk(SettingsManager::instance()->rtkSettings(), _gpsRtk);
    GPSSettingsBindings::bindCorrections(SettingsManager::instance()->gpsCorrectionSettings(), _corrections);
    GPSSettingsBindings::bindNtrip(SettingsManager::instance()->ntripSettings(), _ntripManager);
    _ntripManager->init();
    _startupConnectPending = SettingsManager::instance()->rtkSettings()->connectOnStartup()->rawValue().toBool();
    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(1000);
    connect(_connectionTimer, &QTimer::timeout, this, &GPSManager::_updateConnections);
    if (!QGC::runningUnitTests()) {
        _connectionTimer->start();
    }
}

void GPSManager::_updateConnections()
{
    if (_shutdown || LinkManager::instance()->connectionsSuspended()) {
        return;
    }
    if (std::exchange(_startupConnectPending, false)) {
        _gpsRtk->connectionPolicy()->connectSaved();
        return;
    }
    _gpsRtk->connectionPolicy()->update();
}

void GPSManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    qCDebug(GPSManagerLog) << "Shutting down GPS sources and correction outputs";
    if (_connectionTimer) {
        _connectionTimer->stop();
    }
    _gpsRtk->disconnectGPS();
    _ntripManager->shutdown();
    _corrections->shutdown();
    _positionManager->shutdown();
}
