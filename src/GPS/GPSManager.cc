#include "GPSManager.h"

#include "AppMessages.h"
#include "GPSCorrectionManager.h"
#include "GPSMavlinkOutput.h"
#include "GPSObservation.h"
#include "GPSRtk.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "NMEASourceManager.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPManager.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleLinkManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "RTKAutoConnect.h"
#include "SerialPortManager.h"
#endif
#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

namespace {

Vehicle* activeVehicleForGga()
{
    auto* manager = MultiVehicleManager::instance();
    Vehicle* vehicle = manager ? manager->activeVehicle() : nullptr;
    if (!vehicle || vehicle->isOfflineEditingVehicle() || !vehicle->vehicleLinkManager() ||
        vehicle->vehicleLinkManager()->communicationLost()) {
        return nullptr;
    }
    return vehicle;
}

PositionResult ggaPosition(const QGeoCoordinate& coordinate, const QString& label, GPSAltitudeDatum datum)
{
    if (!coordinate.isValid() || !qIsFinite(coordinate.altitude())) {
        return {};
    }
    return {coordinate, label, datum};
}

PositionResult ggaPosition(const std::optional<GPSObservation>& observation, const QString& label)
{
    if (!observation || observation->altitudeDatum != GPSAltitudeDatum::MeanSeaLevel) {
        return {};
    }
    auto result = ggaPosition(observation->position.coordinate(), label, observation->altitudeDatum);
    result.fixQuality = observation->fixQuality;
    result.satellitesUsed = observation->satellitesUsed;
    result.horizontalDop = observation->horizontalDop;
    return result;
}

}  // namespace

GPSManager::GPSManager(QObject* parent)
    : QObject(parent)
    , _corrections(new GPSCorrectionManager(this))
    , _gpsRtk(new GPSRtk(this))
    , _ntripManager(new NTRIPManager(this))
{
    qCDebug(GPSManagerLog) << this;
    _corrections->rtcmMavlink()->setOutputProvider(createGpsMavlinkOutputProvider());
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

void GPSManager::_configureGgaProviders()
{
    using Source = NTRIPGgaProvider::PositionSource;
    _ntripManager->setGgaPositionProvider(Source::VehicleGPS, []() -> PositionResult {
        Vehicle* vehicle = activeVehicleForGga();
        if (!vehicle) {
            return {};
        }
        const auto* gps = qobject_cast<VehicleGPSFactGroup*>(vehicle->gpsFactGroup());
        return gps ? ggaPosition(gps->acceptedObservation(), QStringLiteral("Vehicle GPS")) : PositionResult{};
    });
    _ntripManager->setGgaPositionProvider(Source::VehicleEKF, []() -> PositionResult {
        Vehicle* vehicle = activeVehicleForGga();
        return vehicle ? ggaPosition(vehicle->acceptedPositionObservation(), QStringLiteral("Vehicle EKF"))
                       : PositionResult{};
    });
    _ntripManager->setGgaPositionProvider(Source::GCSPosition, []() -> PositionResult {
        auto* manager = QGCPositionManager::instance();
        return manager ? ggaPosition(manager->acceptedObservation(GPSObservation::PositionUse::Gga),
                                     QStringLiteral("GCS Position"))
                       : PositionResult{};
    });
}

void GPSManager::init()
{
    if (_connectionTimer || _shutdown) {
        return;
    }
    _configureGgaProviders();
    _corrections->init(SettingsManager::instance()->gpsCorrectionSettings());
    _ntripManager->init();
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
    if (_shutdown || !_nmeaSources || LinkManager::instance()->connectionsSuspended()) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    _nmeaSources->update();
    if (!guard || _shutdown) {
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_rtkAutoConnect) {
        _rtkAutoConnect->update();
    }
#endif
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
    if (_nmeaSources) {
        _nmeaSources->stop();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_rtkAutoConnect) {
        _rtkAutoConnect->stop();
    }
#endif
    _gpsRtk->disconnectGPS();
    _ntripManager->shutdown();
    _corrections->shutdown();
}
