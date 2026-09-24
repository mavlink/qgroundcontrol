#include "PositionManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QPermissions>

#include "AppMessages.h"
#include "NMEASourceManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "SimulatedPosition.h"

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "GPS.PositionManager.QGCPositionManager")
Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

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

QGCPositionManager* QGCPositionManager::instance()
{
    return _positionManager();
}

void QGCPositionManager::init()
{
    if (QGC::runningUnitTests()) {
        setSimulatedPositionSource(new SimulatedPosition(this, scheduler()));
    } else {
        _checkPermission();
    }
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

NMEASourceManager* QGCPositionManager::nmeaInput() const
{
    return _nmeaInput;
}

void QGCPositionManager::setNmeaInput(NMEASourceManager* input)
{
    if (_destroying || _nmeaInput == input) {
        return;
    }
    QObject::disconnect(_nmeaInputDestroyedConnection);
    _nmeaInput = input;
    if (input) {
        _nmeaInputDestroyedConnection =
            connect(input, &QObject::destroyed, this, &QGCPositionManager::nmeaInputChanged);
    }
    emit nmeaInputChanged();
}
