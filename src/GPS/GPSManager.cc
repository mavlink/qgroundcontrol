#include "GPSManager.h"

#include <utility>

#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

#include "AppMessages.h"
#include "Fact.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSMavlinkOutput.h"
#include "GPSObservation.h"
#include "GPSRtk.h"
#include "GPSSettingsBindings.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPManager.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RTKConnectionPolicy.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif
#include "VehicleGPSFactGroup.h"
#include "VehicleLinkManager.h"

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
    , _gpsRtk(new GPSRtk(SettingsManager::instance()->rtkSettings(), SettingsManager::instance()->autoConnectSettings(),
                         this))
    , _ntripManager(new NTRIPManager(this))
    , _udpInputEnabled(SettingsManager::instance()->gpsCorrectionSettings()->rtcmUdpInputEnabled())
{
    qCDebug(GPSManagerLog) << this;
    _corrections->rtcmMavlink()->setOutputProvider(createGpsMavlinkOutputProvider());
    _gpsRtk->setCorrectionManager(_corrections);
#ifndef QGC_NO_SERIAL_LINK
    _gpsRtk->setSerialPortManager(SerialPortManager::instance());
#endif
    _ntripManager->setCorrectionManager(_corrections);
    connect(_corrections, &GPSCorrectionManager::sourceInstancesChanged, this, &GPSManager::_updateCorrectionState);
    connect(_ntripManager, &NTRIPManager::connectionStatusChanged, this, &GPSManager::_updateCorrectionState);
    connect(_gpsRtk, &GPSRtk::receiverChanged, this, &GPSManager::_updateCorrectionState);
    connect(_udpInputEnabled, &Fact::rawValueChanged, this, &GPSManager::_updateCorrectionState);
    _updateCorrectionState();
}

void GPSManager::_updateCorrectionState()
{
    auto state = CorrectionState::Inactive;
    if (_corrections->hasSelectedStream()) {
        state = CorrectionState::Fresh;
    } else if (_ntripManager->connectionStatus() != NTRIPManager::ConnectionStatus::Disconnected ||
               _udpInputEnabled->rawValue().toBool() ||
               (_gpsRtk->hasReceiver() && _gpsRtk->activeRole() != GPSRtk::PositionOnly) ||
               !_corrections->sourceInstances().isEmpty()) {
        state = CorrectionState::Waiting;
    }
    if (std::exchange(_correctionState, state) != state) {
        emit correctionStateChanged();
    }
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

void GPSManager::_configureNtripProviders()
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
    _ntripManager->setGgaPositionProvider(Source::RTKReceiver, [this]() -> PositionResult {
        return ggaPosition(_gpsRtk->acceptedPositionObservation(GPSObservation::PositionUse::Gga),
                           QStringLiteral("RTK Receiver"));
    });
    _ntripManager->setSortPositionProvider([]() {
        auto* manager = MultiVehicleManager::instance();
        Vehicle* vehicle = manager ? manager->activeVehicle() : nullptr;
        return vehicle ? vehicle->coordinate() : QGeoCoordinate();
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
    _configureNtripProviders();
    _gpsRtk->setPositionService(QGCPositionManager::instance());
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
}
