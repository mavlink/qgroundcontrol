#include "PositionManager.h"

#include <utility>

#include <QtCore/QPermissions>

#include "AppMessages.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "SimulatedPosition.h"

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "GPS.PositionManager.QGCPositionManager")

QGCPositionManager::QGCPositionManager(QObject* parent, RuntimeScheduler* scheduler)
    : GPSPositionService(parent, scheduler)
{
    qCDebug(QGCPositionManagerLog) << this;
}

QGCPositionManager::~QGCPositionManager()
{
    qCDebug(QGCPositionManagerLog) << "Position manager shutdown:" << this;
    _destroying = true;
    blockSignals(true);
}

void QGCPositionManager::init()
{
    if (_shutdown) {
        return;
    }
    if (!_sourceSettingConnection) {
        Fact* const sourceSetting = SettingsManager::instance()->rtkSettings()->gcsPositionSource();
        const auto applySource = [this, sourceSetting]() {
            setSourceMode(static_cast<SourceMode>(sourceSetting->rawValue().toInt()));
        };
        _sourceSettingConnection = connect(sourceSetting, &Fact::rawValueChanged, this, applySource);
        applySource();
    }
    if (QGC::runningUnitTests()) {
        setSimulatedPositionSource(new SimulatedPosition(this, scheduler()));
    } else {
        _checkPermission();
    }
}

void QGCPositionManager::shutdown()
{
    if (std::exchange(_shutdown, true)) {
        return;
    }
    qCDebug(QGCPositionManagerLog) << "Releasing position sources";
    QObject::disconnect(_sourceSettingConnection);
    setSimulatedPositionSource(nullptr);
    setInternalPositionSource(nullptr, SourceStatus::NoSource);
}

void QGCPositionManager::_setupPositionSources()
{
    auto* platformSource = QGCCorePlugin::instance()->createPositionSource(this);
    const bool custom = platformSource != nullptr;
    if (!custom) {
        platformSource = QGeoPositionInfoSource::createDefaultSource(this);
    }
    setInternalPositionSource(platformSource,
                              platformSource ? SourceStatus::WaitingForFix : SourceStatus::BackendUnavailable, custom);
}

void QGCPositionManager::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
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

void QGCPositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);
    const auto status = QCoreApplication::instance()->checkPermission(locationPermission);
    if (status == Qt::PermissionStatus::Undetermined) {
        const QPointer<QGCPositionManager> guard(this);
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
