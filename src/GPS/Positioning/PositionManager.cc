#include "PositionManager.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>

#include "QGCLoggingCategory.h"
#include "SimulatedPosition.h"

QGC_LOGGING_CATEGORY(PositionManagerLog, "GPS.PositionManager.PositionManager")

PositionManager::PositionManager(QObject* parent, RuntimeScheduler* scheduler)
    : GPSPositionService(parent, scheduler)
{
    qCDebug(PositionManagerLog) << this;
}

PositionManager::~PositionManager()
{
    qCDebug(PositionManagerLog) << "Position manager shutdown:" << this;
    _destroying = true;
    blockSignals(true);
}

void PositionManager::setConfiguration(const Configuration& configuration)
{
    if (_shutdown) {
        return;
    }
    setSourceMode(configuration.sourceMode);
}

void PositionManager::setPlatformSourceFactory(SourceFactory factory)
{
    _platformSourceFactory = std::move(factory);
}

void PositionManager::setSimulated(bool simulated)
{
    _simulated = simulated;
}

void PositionManager::init()
{
    if (_shutdown) {
        return;
    }
    if (_simulated) {
        auto* const simulated = new SimulatedPosition(this, scheduler());
        emit simulatedPositionCreated(simulated);
        setSimulatedPositionSource(simulated);
    } else {
        _checkPermission();
    }
}

void PositionManager::shutdown()
{
    if (std::exchange(_shutdown, true)) {
        return;
    }
    qCDebug(PositionManagerLog) << "Releasing position sources";
    setSimulatedPositionSource(nullptr);
    setInternalPositionSource(nullptr, SourceStatus::NoSource);
}

void PositionManager::_setupPositionSources()
{
    auto* platformSource = _platformSourceFactory ? _platformSourceFactory(this) : nullptr;
    const bool custom = platformSource != nullptr;
    if (!custom) {
        platformSource = QGeoPositionInfoSource::createDefaultSource(this);
    }
    setInternalPositionSource(platformSource,
                              platformSource ? SourceStatus::WaitingForFix : SourceStatus::BackendUnavailable, custom);
}

void PositionManager::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    if (_shutdown) {
        return;
    }
    if (permissionStatus == Qt::PermissionStatus::Granted) {
        _setupPositionSources();
    } else {
        setInternalPositionStatus(SourceStatus::PermissionDenied);
    }
}

void PositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);
    const auto status = QCoreApplication::instance()->checkPermission(locationPermission);
    if (status == Qt::PermissionStatus::Undetermined) {
        const QPointer<PositionManager> guard(this);
        setInternalPositionStatus(SourceStatus::PermissionRequired);
        if (guard) {
            QCoreApplication::instance()->requestPermission(
                locationPermission, this,
                [this](const QPermission& permission) { _handlePermissionStatus(permission.status()); });
        }
    } else {
        _handlePermissionStatus(status);
    }
}
