#include "PositionManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QIODevice>
#include <QtCore/QPermissions>
#include <QtCore/QThread>
#include <QtPositioning/QNmeaPositionInfoSource>

#include "AppMessages.h"
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
    qCDebug(QGCPositionManagerLog) << this;
    _destroying = true;
    blockSignals(true);
    resetNmeaSourceDevice();
}

QGCPositionManager* QGCPositionManager::instance()
{
    return _positionManager();
}

void QGCPositionManager::init()
{
    if (QGC::runningUnitTests()) {
        setSimulatedPositionSource(new SimulatedPosition(this));
    } else {
        _checkPermission();
    }
}

void QGCPositionManager::_setupPositionSources()
{
    _platformSource = QGCCorePlugin::instance()->createPositionSource(this);
    const bool custom = !_platformSource.isNull();
    if (!custom) {
        _platformSource = QGeoPositionInfoSource::createDefaultSource(this);
    }
    setInternalPositionSource(_platformSource,
                              _platformSource ? SourceStatus::WaitingForFix : SourceStatus::BackendUnavailable, custom);
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

void QGCPositionManager::setNmeaSourceDevice(QIODevice* device)
{
    if (_destroying) {
        return;
    }
    if (QThread::currentThread() != thread() || (device && device->thread() != thread())) {
        qCWarning(QGCPositionManagerLog) << "NMEA device requires matching thread affinity";
        return;
    }
    const QPointer<QGCPositionManager> guard(this);
    const QPointer<QIODevice> deviceGuard(device);
    const quint64 revision = _nmeaRevision + 1;
    resetNmeaSourceDevice();
    if (!guard || _nmeaRevision != revision || !deviceGuard) {
        return;
    }
    _nmeaSource = std::make_unique<QNmeaPositionInfoSource>(QNmeaPositionInfoSource::RealTimeMode);
    _nmeaSource->setDevice(device);
    _nmeaSource->setUserEquivalentRangeError(5.1);
    _nmeaDeviceDestroyedConnection = connect(device, &QObject::destroyed, this, [this, revision]() {
        if (_nmeaRevision == revision) {
            resetNmeaSourceDevice();
        }
    });
    auto registration = registerPositionSource(SelectedSource::Nmea, _nmeaSource.get(), nullptr);
    if (guard && _nmeaRevision == revision) {
        _nmeaRegistration = std::move(registration);
    }
}

void QGCPositionManager::resetNmeaSourceDevice()
{
    if (QThread::currentThread() != thread()) {
        return;
    }
    ++_nmeaRevision;
    QObject::disconnect(_nmeaDeviceDestroyedConnection);
    // Keep the old source alive through retirement signals, even if they replace it or delete this manager.
    auto source = std::move(_nmeaSource);
    auto registration = std::move(_nmeaRegistration);
    registration.reset();
}
