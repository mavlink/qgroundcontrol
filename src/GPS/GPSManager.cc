#include "GPSManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QPointer>

#include "GPSCorrectionManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "LinkManager.h"
#include "NTRIPGgaReporter.h"
#include "NTRIPManager.h"
#include "NTRIPVehicleGgaSource.h"
#include "PositionManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkAvailabilityMonitor.h"
#include "RTKSettings.h"
#include "RuntimeScheduler.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

namespace {

GPSReceiver::Dependencies receiverDependencies(GPSCorrectionManager* corrections, PositionManager* positions)
{
    GPSReceiver::Dependencies dependencies{.corrections = corrections, .positions = positions};
#ifndef QGC_NO_SERIAL_LINK
    dependencies.serialPorts = SerialPortManager::instance();
#endif
    return dependencies;
}

}  // namespace

GPSManager::GPSManager(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, nullptr))
    , _corrections(new GPSCorrectionManager(this, _scheduler))
    , _positionManager(new PositionManager(this, _scheduler))
    , _receiver(new GPSReceiver(this, _scheduler, receiverDependencies(_corrections, _positionManager)))
    , _ntripManager(new NTRIPManager(this, _scheduler,
                                     {.corrections = _corrections, .network = new QGCNetworkAvailabilityMonitor(this)}))
    , _vehicleGga(new NTRIPVehicleGgaSource(_scheduler, this))
{
    qCDebug(GPSManagerLog) << this;
    _positionManager->setPlatformSourceFactory(
        [](QObject* sourceParent) { return QGCCorePlugin::instance()->createPositionSource(sourceParent); });
    if (!scheduler) {
        // The last child, so the services it runs are destroyed before it.
        _scheduler->setParent(this);
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

bool GPSManager::saveCurrentBasePosition()
{
    // A copy: a settings observer may reconfigure the receiver, which rewrites the receiver Facts.
    const std::optional<GPSBaseStationConfig::Fixed> base = _receiver->facts()->currentBasePosition();
    if (!base) {
        return false;
    }
    _receiver->facts()->_setCurrentBasePositionSaved(true);
    RTKSettings* settings = SettingsManager::instance()->rtkSettings();
    settings->fixedBasePositionLatitude()->setRawValue(base->position.latitudeDegrees);
    settings->fixedBasePositionLongitude()->setRawValue(base->position.longitudeDegrees);
    settings->fixedBasePositionAltitude()->setRawValue(base->position.altitudeMeters);
    settings->fixedBasePositionAccuracy()->setRawValue(base->accuracyMeters);
    return true;
}

void GPSManager::init()
{
    if (_initialized || _shutdown) {
        return;
    }
    _initialized = true;
    GPSSettingsBindings::bindPosition(SettingsManager::instance()->rtkSettings(), _positionManager);
    _positionManager->init();
    _initGgaSources();
    GPSSettingsBindings::bindReceiver(SettingsManager::instance()->rtkSettings(), _receiver);
    GPSSettingsBindings::bindCorrections(SettingsManager::instance()->gpsCorrectionSettings(), _corrections);
    GPSSettingsBindings::bindNtrip(SettingsManager::instance()->ntripSettings(), _ntripManager);
    _ntripManager->init();
    _receiver->startConnectionPolling([] { return LinkManager::instance()->connectionsSuspended(); });
}

void GPSManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    qCDebug(GPSManagerLog) << "Shutting down GPS sources and correction outputs";
    _receiver->shutdown();
    _ntripManager->shutdown();
    _corrections->shutdown();
    _positionManager->shutdown();
}

void GPSManager::_initGgaSources()
{
    using Source = NTRIPGgaReporter::PositionSource;
    using Observation = std::optional<GPSObservation>;
    _vehicleGga->attach(_ntripManager);
    _ntripManager->setGgaPositionProvider(
        Source::RTKReceiver, [receiver = QPointer<GPSReceiver>(_receiver)]() -> Observation {
            return receiver ? receiver->acceptedPositionObservation(GPSObservation::PositionUse::Gga) : std::nullopt;
        });
    _ntripManager->setGgaPositionProvider(
        Source::GCSPosition, [positionManager = QPointer<PositionManager>(_positionManager)]() -> Observation {
            return positionManager ? positionManager->acceptedObservation(GPSObservation::PositionUse::Gga)
                                   : std::nullopt;
        });
}
