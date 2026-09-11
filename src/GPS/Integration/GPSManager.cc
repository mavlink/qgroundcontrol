#include "GPSManager.h"

#include <QtCore/QCoreApplication>

#include "AppMessages.h"
#include "AutoConnectSettings.h"
#include "GPSBaseReferenceSave.h"
#include "GPSBaseStationState.h"
#include "GPSConnectionSettingsController.h"
#include "GPSDriverRevision.h"
#include "GPSMavlinkOutput.h"
#include "GPSReceiver.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverSettingsPresentation.h"
#include "GPSSettings.h"
#include "GPSSourceBindings.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "NMEASourceManager.h"
#include "NTRIPManager.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "VehicleGPSPositionProvider.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPortRegistry.h"
#endif
#include <QtCore/QApplicationStatic>

#include <utility>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject* parent)
    : GPSManager(
          *SettingsManager::instance(), QGCPositionManager::instance(),
          []() { return LinkManager::instance()->connectionsSuspended(); }, parent)
{
    qCDebug(GPSManagerLog) << this;
    connect(LinkManager::instance(), &LinkManager::connectionsSuspendedChanged, this, &GPSManager::_updateConnections);
    auto* output = new GPSMavlinkOutput(this);
    _corrections.rtcmMavlink()->setOutputProvider([output]() { return output->outputs(); });
    _vehiclePositionProvider = new VehicleGPSPositionProvider(this);
    auto* vehicles = MultiVehicleManager::instance();
    _vehiclePositionProvider->setVehicle(vehicles->activeVehicle());
    connect(vehicles, &MultiVehicleManager::activeVehicleChanged, _vehiclePositionProvider,
            &VehicleGPSPositionProvider::setVehicle);
}

GPSManager::GPSManager(SettingsManager& settingsManager, QGCPositionManager* positionManager,
                       std::function<bool()> connectionsSuspended, QObject* parent)
    : QObject(parent)
    , _settings(settingsManager)
    , _positionManager(positionManager)
    , _corrections(this)
    , _recording(this)
    , _satellites(this)
    , _nmeaSatellites(this)
    , _relativePosition(this)
    , _receiverSession(this)
    , _receiverState(_receiverSession, this)
    , _receiver(new GPSReceiver(_receiverState, this))
    , _baseStationState(new GPSBaseStationState(_receiverState, *_receiver->facts()->rtk(), this))
{
    qCDebug(GPSManagerLog) << this;
    _nmeaSources = new NMEASourceManager(this);
    _receiverAutoConnect = new GPSReceiverAutoConnect(&_receiverSession, _receiverState.health(), this);
    _sourceBindings =
        new GPSSourceBindings(_settings, positionManager, _receiverState, *_nmeaSources, _corrections, this);
    _settingsController = new GPSConnectionSettingsController(_settings, *_receiverAutoConnect, *_nmeaSources,
                                                              std::move(connectionsSuspended), this);
    _recording.buffer()->setProvenance(
        {QStringLiteral("QGroundControl"), QCoreApplication::applicationVersion(), GPS_DRIVER_CONFIGURATION_REVISION});
    _nmeaSources->setRecordingBuffer(_recording.buffer());
    _receiverSession.setRecordingBuffer(_recording.buffer());
    _relativePosition.bindStore(_receiverState.relativePosition());
    connect(&_receiverSession, &GPSReceiverSession::configurationStarted, this, [this]() {
        if (!_shutdown) {
            _satellites.beginSession(QStringLiteral("nativeReceiver"), _receiverSession.sessionId());
        }
    });
    connect(_receiverState.satellites(), &GPSSatelliteStore::observationChanged, &_satellites,
            &GPSSatelliteModel::updateObservation);
    connect(&_receiverSession, &GPSReceiverSession::disconnected, &_satellites, &GPSSatelliteModel::reset);
    connect(_nmeaSources, &NMEASourceManager::satellitesReceived, this, &GPSManager::_updateNmeaSatellites);
    connect(_nmeaSources, &NMEASourceManager::stateChanged, this, &GPSManager::_updateNmeaSatellites);
    connect(&_receiverSession, &GPSReceiverSession::receiverTypeChanged, this, [this](GPSType type) {
        const auto capabilities = GPSReceiverCapabilities::forType(type);
        if (capabilities.recognized()) {
            _settings.rtkSettings()->baseReceiverManufacturers()->setRawValue(capabilities.manufacturerId);
        }
    });
    connect(_settingsController, &GPSConnectionSettingsController::receiverSettingsChanged, this,
            &GPSManager::receiverSettingsChanged);
    connect(&_receiverSession, &GPSReceiverSession::capabilitiesUpdated, this, &GPSManager::receiverSettingsChanged);
    connect(&_receiverSession, &GPSReceiverSession::configurationReported, this,
            &GPSManager::configurationReportChanged);
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, this, &GPSManager::receiverSettingsChanged);
    connect(this, &GPSManager::receiverSettingsChanged, this, &GPSManager::baseReferenceSaveStateChanged);
    connect(&_receiverState, &GPSReceiverState::referenceChanged, this, &GPSManager::baseReferenceSaveStateChanged);
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, this, &GPSManager::baseReferenceSaveStateChanged);
    auto* receiverSettings = _settings.rtkSettings();
    for (Fact* fact : {receiverSettings->receiverRole(), receiverSettings->useFixedBasePosition(),
                       receiverSettings->fixedBasePositionAccuracy()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSManager::baseReferenceSaveStateChanged);
    }
}

GPSManager::~GPSManager()
{
    qCDebug(GPSManagerLog) << this;

    shutdown();
    delete _sourceBindings;
    delete _settingsController;
    delete _receiverAutoConnect;
    delete _baseStationState;
    delete _receiver;
}

GPSManager* GPSManager::instance()
{
    return _gpsManager();
}

void GPSManager::init(NTRIPManager* ntrip)
{
    if (_initialized || _shutdown) {
        return;
    }
    const QPointer<GPSManager> guard(this);
    _initialized = true;
    _sourceBindings->init(ntrip);
    if (!guard || _shutdown) {
        return;
    }
    _ntrip = ntrip;
    if (_ntrip) {
        const QPointer<GPSManager> owner(this);
        using Source = NTRIPGgaProvider::PositionSource;
        _ntrip->setPositionProvider(Source::VehicleGPS, [owner]() {
            return owner && owner->_vehiclePositionProvider
                       ? PositionResult{owner->_vehiclePositionProvider->gpsPosition(), QStringLiteral("Vehicle GPS")}
                       : PositionResult{};
        });
        _ntrip->setPositionProvider(Source::VehicleEKF, [owner]() {
            return owner && owner->_vehiclePositionProvider
                       ? PositionResult{owner->_vehiclePositionProvider->ekfPosition(), QStringLiteral("Vehicle EKF")}
                       : PositionResult{};
        });
        _ntrip->setPositionProvider(Source::RTKBase, [owner]() {
            if (!owner || owner->_shutdown) {
                return PositionResult{};
            }
            const auto reference = owner->_receiverState.reference();
            const auto coordinate = reference.observation.position.coordinate();
            const bool accepted = reference.isValid() && (coordinate.latitude() != 0 || coordinate.longitude() != 0);
            return accepted ? PositionResult{reference.observation, QStringLiteral("RTK Base"), true}
                            : PositionResult{};
        });
        _ntrip->setPositionProvider(Source::GCSPosition, [owner]() {
            if (!owner || owner->_shutdown || !owner->_positionManager) {
                return PositionResult{};
            }
            const auto observation = owner->_positionManager->acceptedObservation(GPSObservation::PositionUse::NTRIP);
            if (!observation) {
                return PositionResult{};
            }
            const auto coordinate = observation->position.coordinate();
            return coordinate.latitude() != 0 || coordinate.longitude() != 0
                       ? PositionResult{*observation, QStringLiteral("GCS Position")}
                       : PositionResult{};
        });
    }
#ifndef QGC_NO_SERIAL_LINK
    auto* serialDiscovery = new GPSSerialPortRegistry(SerialPortManager::instance(), this);
    _receiverAutoConnect->setSerialDiscovery(serialDiscovery);
    _nmeaSources->setSerialDiscovery(serialDiscovery);
#endif
    if (_ntrip) {
        _ntrip->init();
    }
    if (!guard || _shutdown) {
        return;
    }
    _settingsController->start();
}

void GPSManager::_updateNmeaSatellites()
{
    if (_shutdown || !_nmeaSources->positionSource()) {
        _nmeaSatellites.reset();
        return;
    }
    const quint64 session = _nmeaSources->sessionId();
    const QPointer<GPSManager> guard(this);
    if (_nmeaSatellites.sessionId() != session) {
        _nmeaSatellites.beginSession(QStringLiteral("nmeaReceiver"), session);
    }
    if (!guard || _shutdown || session != _nmeaSources->sessionId() || !_nmeaSources->positionSource()) {
        return;
    }
    _nmeaSatellites.updateObservation(_nmeaSources->satelliteObservation());
}

bool GPSManager::canSaveBaseReference() const
{
    return !_savingBaseReference && baseReferenceSaveError().isEmpty();
}

QString GPSManager::baseReferenceSaveError() const
{
    if (_shutdown || !_receiverSession.hasReceiver()) {
        return tr("No base receiver is connected");
    }
    const auto configuration = GPSSettings::receiver(*_settings.rtkSettings(), *_settings.autoConnectSettings());
    return GPSBaseReferenceSave::prepare(_receiverState.reference(), _receiverSession.sessionId(),
                                         configuration.profile.receiver)
        .error;
}

bool GPSManager::saveBaseReference()
{
    if (_savingBaseReference || _shutdown || !_receiverSession.hasReceiver()) {
        return false;
    }
    const auto configuration = GPSSettings::receiver(*_settings.rtkSettings(), *_settings.autoConnectSettings());
    const auto prepared = GPSBaseReferenceSave::prepare(_receiverState.reference(), _receiverSession.sessionId(),
                                                        configuration.profile.receiver);
    if (!prepared.configuration) {
        QGC::showAppMessage(prepared.error);
        return false;
    }
    const QPointer<GPSManager> guard(this);
    _savingBaseReference = true;
    _settingsController->beginReceiverSettingsBatch();
    const bool saved = _settings.rtkSettings()->saveFixedBasePosition(*prepared.configuration);
    if (!guard) {
        return false;
    }
    _savingBaseReference = false;
    _settingsController->endReceiverSettingsBatch();
    if (guard) {
        emit baseReferenceSaveStateChanged();
    }
    return saved;
}

bool GPSManager::connectNmea()
{
    return !_shutdown && !_settingsController->connectionsSuspended() && _nmeaSources->connectSource();
}

void GPSManager::disconnectNmea()
{
    _nmeaSources->disconnectSource();
}

bool GPSManager::connectReceiver()
{
    return !_shutdown && !_settingsController->connectionsSuspended() && _receiverAutoConnect->connectSelected();
}

void GPSManager::disconnectReceiver()
{
    _receiverAutoConnect->disconnectSelected();
}

void GPSManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    const QPointer<GPSManager> guard(this);
    _settingsController->shutdown();
    _sourceBindings->stop();
    if (!guard) {
        return;
    }
    const QPointer<NTRIPManager> ntrip = std::exchange(_ntrip, nullptr);
    if (ntrip) {
        ntrip->disconnect(this);
        ntrip->stopNTRIP();
        if (!guard) {
            return;
        }
    }
    if (_nmeaSources) {
        _nmeaSources->shutdown();
        if (!guard) {
            return;
        }
    }
    _receiverAutoConnect->stop();
    if (!guard) {
        return;
    }
    _receiverSession.shutdown();
    if (!guard) {
        return;
    }
    _recording.stop();
    if (!guard) {
        return;
    }
    _satellites.reset();
    if (!guard) {
        return;
    }
    _nmeaSatellites.reset();
    if (!guard) {
        return;
    }
    _receiverState.reset();
    if (!guard) {
        return;
    }
    _sourceBindings->shutdown();
    if (!guard) {
        return;
    }
    _corrections.shutdown();
}

QVariantList GPSManager::receiverSettings() const
{
    auto* settings = _settings.rtkSettings();
    const auto capabilities = _receiverSession.hasReceiver()
                                  ? _receiverSession.capabilities()
                                  : GPSReceiverCapabilities::forType(
                                        static_cast<GPSType>(settings->networkReceiverType()->rawValue().toInt()));
    const bool baseStation = settings->receiverRole()->rawValue().toInt() == RTKSettings::RTKBase;
    return GPSReceiverSettingsPresentation::settingDescriptors(capabilities, baseStation);
}

QVariantList GPSManager::configurationReport() const
{
    return GPSReceiverSettingsPresentation::configurationReport(_receiverSession.configurationReport());
}

void GPSManager::_updateConnections()
{
    if (!_shutdown) {
        _settingsController->updateConnections();
    }
}
