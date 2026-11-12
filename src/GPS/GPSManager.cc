#include "GPSManager.h"

#include <functional>
#include <utility>

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QPointer>

#include "AppSettings.h"
#include "GPSCorrectionManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "LinkManager.h"
#include "NTRIPGgaReporter.h"
#include "NTRIPManager.h"
#include "NTRIPVehicleGgaSource.h"
#include "PositionManager.h"
#include "PositionReceiverSettings.h"
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

/// The folder @a name beside the other files the application saves, following a save location changed while it runs;
/// empty without a save location.
QString saveFolder(const QString& name)
{
    const QString savePath = SettingsManager::instance()->appSettings()->savePath()->rawValue().toString();
    return savePath.isEmpty() ? QString() : QDir(savePath).filePath(name);
}

/// The receiver's serial side over @a serialPorts, or over the application's serial devices without them, which
/// keeps discovery off the devices @a discoveryExcluded names.
std::function<GPSReceiverSerialPorts*(QObject*)> serialSide(
    [[maybe_unused]] SerialPortManager* serialPorts,
    [[maybe_unused]] std::function<bool(const QString&)> discoveryExcluded = {})
{
#ifdef QGC_NO_SERIAL_LINK
    return nullptr;
#else
    return [serialPorts, discoveryExcluded = std::move(discoveryExcluded)](QObject* parent) {
        GPSReceiverSerialPorts* const serial =
            GPSReceiverSerialPorts::create(serialPorts ? serialPorts : SerialPortManager::instance(), parent);
        serial->setDiscoveryExclusion(discoveryExcluded);
        return serial;
    };
#endif
}

GPSReceiver::Dependencies receiverDependencies(GPSCorrectionManager* corrections, PositionManager* positions,
                                               SerialPortManager* serialPorts,
                                               std::function<bool(const QString&)> discoveryExcluded)
{
    GPSReceiver::Dependencies dependencies{.corrections = corrections, .positions = positions};
    dependencies.serial = serialSide(serialPorts, std::move(discoveryExcluded));
    dependencies.recordingDirectory = []() { return saveFolder(QStringLiteral("GPSReceiver")); };
    // What "Detect automatically" finds on each link is kept in the settings, for its next connection there.
    dependencies.detectedReceiver = [](const QString& link) {
        return SettingsManager::instance()->rtkSettings()->detectedReceiver(link);
    };
    dependencies.rememberDetectedReceiver = [](const RTKDetectedReceiver& receiver) {
        SettingsManager::instance()->rtkSettings()->rememberDetectedReceiver(receiver);
    };
    return dependencies;
}

/// The position receiver has no correction manager, since what RTCM it sends is not a correction source, and
/// nothing to remember of a detection, since it is never asked what it is.
GPSReceiver::Dependencies positionReceiverDependencies(PositionManager* positions, SerialPortManager* serialPorts)
{
    GPSReceiver::Dependencies dependencies{.positions = positions, .positionOnly = true};
    dependencies.serial = serialSide(serialPorts);
    // A folder of its own: both receivers name a recording by the time it started and the receiver type.
    dependencies.recordingDirectory = []() { return saveFolder(QStringLiteral("GPSPositionReceiver")); };
    return dependencies;
}

QString correctionLogDirectory()
{
    return saveFolder(QStringLiteral("GPSCorrections"));
}

}  // namespace

GPSManager::GPSManager(QObject* parent, RuntimeScheduler* scheduler, SerialPortManager* serialPorts)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, nullptr))
    , _corrections(new GPSCorrectionManager(this, _scheduler, {.logDirectory = correctionLogDirectory}))
    , _positionManager(new PositionManager(this, _scheduler))
    , _receiver(new GPSReceiver(
          this, _scheduler,
          receiverDependencies(_corrections, _positionManager, serialPorts,
                               [this](const QString& device) { return _positionReceiverUses(device); })))
    , _positionReceiver(new GPSReceiver(this, _scheduler, positionReceiverDependencies(_positionManager, serialPorts)))
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

bool GPSManager::saveCurrentBasePosition(const QString& siteName)
{
    // A copy: a settings observer may reconfigure the receiver, which rewrites the receiver Facts.
    const std::optional<GPSBaseStationConfig::Fixed> base = _receiver->facts()->currentBasePosition();
    if (!base) {
        return false;
    }
    RTKSettings* settings = SettingsManager::instance()->rtkSettings();
    if (!siteName.trimmed().isEmpty() && !settings->saveBaseSite({.name = siteName,
                                                                  .latitude = base->position.latitudeDegrees,
                                                                  .longitude = base->position.longitudeDegrees,
                                                                  .altitude = base->position.altitudeMeters,
                                                                  .accuracy = base->accuracyMeters})) {
        return false;
    }
    _receiver->facts()->_setCurrentBasePositionSaved(true);
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
    SettingsManager* const settings = SettingsManager::instance();
    GPSSettingsBindings::bindPosition(settings->rtkSettings(), settings->positionReceiverSettings(), _positionManager);
    _positionManager->init();
    _initGgaSources();
    GPSSettingsBindings::bindReceiver(settings->rtkSettings(), _receiver);
    GPSSettingsBindings::bindPositionReceiver(settings->positionReceiverSettings(), _positionReceiver);
    GPSSettingsBindings::bindCorrections(settings->gpsCorrectionSettings(), _corrections);
    GPSSettingsBindings::bindNtrip(settings->ntripSettings(), _ntripManager);
    _ntripManager->init();
    const auto suspended = [] { return LinkManager::instance()->connectionsSuspended(); };
    _receiver->startConnectionPolling(suspended);
    _positionReceiver->startConnectionPolling(suspended);
}

bool GPSManager::_positionReceiverUses(const QString& device) const
{
    // Its settings make it a receiver that connects automatically for as long as it is in use.
    const GPSReceiver::Configuration& configuration = _positionReceiver->configuration();
    return configuration.autoConnect && configuration.connectionType == RTKSettings::Serial &&
           configuration.serialDevice.trimmed() == device;
}

void GPSManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    qCDebug(GPSManagerLog) << "Shutting down GPS sources and correction outputs";
    _receiver->shutdown();
    _positionReceiver->shutdown();
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
