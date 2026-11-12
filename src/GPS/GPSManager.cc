#include "GPSManager.h"

#include <functional>
#include <memory>
#include <utility>

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QPointer>

#include "AppSettings.h"
#include "GPSCorrectionManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverBluetooth.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "LinkManager.h"
#include "NTRIPConnectedVehicles.h"
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

/// One device discovery of @a platform, or of the build's without one, for every caller: created as a child of
/// @a owner when first asked for.
std::function<QObject*()> sharedBluetoothDiscovery(const GPSBluetoothPlatform* platform, QObject* owner)
{
    return [create = (platform ? *platform : GPSBluetoothPlatform::system()).createDiscovery, owner,
            discovery = std::make_shared<QPointer<QObject>>()]() -> QObject* {
        if (!*discovery && create) {
            *discovery = create(owner);
        }
        return *discovery;
    };
}

/// The receiver's Bluetooth side over @a platform, or over the build's without one, which scans with @a discovery
/// and leaves to another receiver the devices @a refusal gives a reason for.
std::function<GPSReceiverBluetooth*(QObject*)> bluetoothSide(const GPSBluetoothPlatform* platform,
                                                             std::function<QObject*()> discovery,
                                                             std::function<QString(const QString&)> refusal = {})
{
    return [platform = platform ? *platform : GPSBluetoothPlatform::system(), discovery = std::move(discovery),
            refusal = std::move(refusal)](QObject* parent) {
        auto* const bluetooth = new GPSReceiverBluetooth(parent, platform);
        bluetooth->setSharedDiscovery(discovery);
        bluetooth->setRefusal(refusal);
        return bluetooth;
    };
}

GPSReceiver::Dependencies receiverDependencies(GPSCorrectionManager* corrections, PositionManager* positions,
                                               SerialPortManager* serialPorts,
                                               std::function<bool(const QString&)> discoveryExcluded,
                                               const GPSBluetoothPlatform* bluetooth,
                                               std::function<QObject*()> bluetoothDiscovery,
                                               std::function<QString(const QString&)> bluetoothRefusal)
{
    GPSReceiver::Dependencies dependencies{.corrections = corrections, .positions = positions};
    dependencies.serial = serialSide(serialPorts, std::move(discoveryExcluded));
    dependencies.bluetooth = bluetoothSide(bluetooth, std::move(bluetoothDiscovery), std::move(bluetoothRefusal));
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
GPSReceiver::Dependencies positionReceiverDependencies(PositionManager* positions, SerialPortManager* serialPorts,
                                                       const GPSBluetoothPlatform* bluetooth,
                                                       std::function<QObject*()> bluetoothDiscovery)
{
    GPSReceiver::Dependencies dependencies{.positions = positions, .positionOnly = true};
    dependencies.serial = serialSide(serialPorts);
    dependencies.bluetooth = bluetoothSide(bluetooth, std::move(bluetoothDiscovery));
    // A folder of its own: both receivers name a recording by the time it started and the receiver type.
    dependencies.recordingDirectory = []() { return saveFolder(QStringLiteral("GPSPositionReceiver")); };
    return dependencies;
}

QString correctionLogDirectory()
{
    return saveFolder(QStringLiteral("GPSCorrections"));
}

}  // namespace

GPSManager::GPSManager(QObject* parent, RuntimeScheduler* scheduler, SerialPortManager* serialPorts,
                       const GPSBluetoothPlatform* bluetooth)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, nullptr))
    , _corrections(new GPSCorrectionManager(this, _scheduler, {.logDirectory = correctionLogDirectory}))
    , _positionManager(new PositionManager(this, _scheduler))
    , _bluetoothDiscovery(sharedBluetoothDiscovery(bluetooth, this))
    , _receiver(new GPSReceiver(
          this, _scheduler,
          receiverDependencies(
              _corrections, _positionManager, serialPorts,
              [this](const QString& device) { return _positionReceiverUses(device); }, bluetooth, _bluetoothDiscovery,
              [this](const QString& address) { return _positionReceiverBluetoothRefusal(address); })))
    , _positionReceiver(
          new GPSReceiver(this, _scheduler,
                          positionReceiverDependencies(_positionManager, serialPorts, bluetooth, _bluetoothDiscovery)))
    , _connectedVehicles(new NTRIPConnectedVehicles(this))
    , _ntripManager(new NTRIPManager(this, _scheduler,
                                     {.corrections = _corrections,
                                      .network = new QGCNetworkAvailabilityMonitor(this),
                                      .vehicles = _connectedVehicles}))
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
    // A base reports where its antenna is, so the saved position has no antenna above it.
    settings->fixedBaseAntennaHeight()->setRawValue(0.);
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
    _connectedVehicles->attach();
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

QString GPSManager::_positionReceiverBluetoothRefusal(const QString& address) const
{
    const GPSReceiver::Configuration& configuration = _positionReceiver->configuration();
    // Addresses are hexadecimal, in either case.
    const bool inUse = configuration.autoConnect &&
                       _positionReceiver->effectiveConnectionType() == RTKSettings::Bluetooth &&
                       configuration.bluetoothAddress.trimmed().compare(address.trimmed(), Qt::CaseInsensitive) == 0;
    return inUse ? tr("The position receiver uses this Bluetooth device. Select another device for one of the "
                      "receivers, or turn the position receiver off.")
                 : QString();
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
