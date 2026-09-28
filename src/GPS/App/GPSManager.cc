#include "GPSManager.h"

#include <memory>
#include <utility>

#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

#include "AppMessages.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSCorrectionStatus.h"
#include "GPSGgaSources.h"
#include "GPSMAVLinkOutput.h"
#include "GPSReceiver.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "NTRIPManager.h"
#include "NTRIPNetworkMonitor.h"
#include "PositionManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "SimulatedPosition.h"
#include "Vehicle.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPortManagerAdapter.h"
#include "SerialPortManager.h"
#endif

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject* parent)
    : QObject(parent)
    , _corrections(new GPSCorrectionManager(this))
    , _receiver(new GPSReceiver(this))
    , _receiverFacts(new GPSReceiverFactGroup(_receiver, this))
    , _ntripManager(new NTRIPManager(this))
    , _ntripNetworkMonitor(new QtNTRIPNetworkMonitor(this))
    , _positionManager(new PositionManager(this))
    , _correctionStatus(
          new GPSCorrectionStatus(_corrections, _ntripManager, _receiver,
                                  SettingsManager::instance()->gpsCorrectionSettings()->rtcmUdpInputEnabled(), this))
    , _ggaSources(new GPSGgaSources(_ntripManager, _receiver, _positionManager, this))
{
    qCDebug(GPSManagerLog) << this;
    _positionManager->setPlatformSourceFactory(
        [](QObject* sourceParent) { return QGCCorePlugin::instance()->createPositionSource(sourceParent); });
    _positionManager->setSimulated(QGC::runningUnitTests());
    connect(_positionManager, &PositionManager::simulatedPositionCreated, this,
            [](SimulatedPosition* simulated) { _followVehicleHome(MultiVehicleManager::instance(), simulated); });
    _corrections->rtcmMavlink()->setOutputProvider(createGPSMAVLinkOutputProvider());
    _receiver->setCorrectionManager(_corrections);
#ifndef QGC_NO_SERIAL_LINK
    _receiver->setSerialPorts(new GPSSerialPortManagerAdapter(SerialPortManager::instance(), this));
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

bool GPSManager::saveCurrentBasePosition()
{
    if (!_receiverFacts->canSaveCurrentBasePosition()) {
        return false;
    }
    // Read every value first: a settings observer may reconfigure the receiver, which republishes these Facts.
    const QVariant latitude = _receiverFacts->currentLatitude()->rawValue();
    const QVariant longitude = _receiverFacts->currentLongitude()->rawValue();
    const QVariant altitude = _receiverFacts->currentAltitude()->rawValue();
    const QVariant accuracy = _receiverFacts->currentAccuracy()->rawValue();
    RTKSettings* settings = SettingsManager::instance()->rtkSettings();
    settings->fixedBasePositionLatitude()->setRawValue(latitude);
    settings->fixedBasePositionLongitude()->setRawValue(longitude);
    settings->fixedBasePositionAltitude()->setRawValue(altitude);
    settings->fixedBasePositionAccuracy()->setRawValue(accuracy);
    return true;
}

void GPSManager::init()
{
    if (_connectionTimer || _shutdown) {
        return;
    }
    GPSSettingsBindings::bindPosition(SettingsManager::instance()->rtkSettings(), _positionManager);
    _positionManager->init();
    _ggaSources->init();
    _receiver->setPositionService(_positionManager);
    GPSSettingsBindings::bindRtk(SettingsManager::instance()->rtkSettings(), _receiver);
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

void GPSManager::_followVehicleHome(MultiVehicleManager* vehicles, SimulatedPosition* simulated)
{
    struct HomeFollow
    {
        QMetaObject::Connection homeChanged;
        // Invalidates the pending home connection of a vehicle that is no longer followed.
        quint64 revision = 0;
    };

    const auto follow = std::make_shared<HomeFollow>();
    connect(vehicles, &MultiVehicleManager::vehicleAdded, simulated, [simulated, follow](Vehicle* vehicle) {
        if (!vehicle) {
            return;
        }
        disconnect(std::exchange(follow->homeChanged, {}));
        const quint64 revision = ++follow->revision;
        if (vehicle->homePosition().isValid()) {
            simulated->setReferencePosition(vehicle->homePosition());
            return;
        }
        follow->homeChanged = connect(vehicle, &Vehicle::homePositionChanged, simulated,
                                      [simulated, follow, revision](const QGeoCoordinate& homePosition) {
                                          if (revision != follow->revision || !homePosition.isValid()) {
                                              return;
                                          }
                                          ++follow->revision;
                                          simulated->setReferencePosition(homePosition);
                                          disconnect(std::exchange(follow->homeChanged, {}));
                                      });
    });
}

void GPSManager::_updateConnections()
{
    if (_shutdown || LinkManager::instance()->connectionsSuspended()) {
        return;
    }
    if (std::exchange(_startupConnectPending, false)) {
        _receiver->connectionPolicy()->connectSaved();
        return;
    }
    _receiver->connectionPolicy()->update();
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
    _receiver->disconnectGPS();
    _ntripManager->shutdown();
    _corrections->shutdown();
    _positionManager->shutdown();
}
