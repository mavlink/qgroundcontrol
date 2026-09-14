#include "PositionManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QIODevice>
#include <QtCore/QPermissions>
#include <QtCore/QThread>

#include "AppMessages.h"
#include "NMEADecoderSession.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "SimulatedPosition.h"

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "GPS.PositionManager.QGCPositionManager")
Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject* parent, RuntimeScheduler* scheduler)
    : GPSPositionService(parent, scheduler)
    , _nmeaScheduler(scheduler)
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
    _nmeaSource = std::make_unique<NMEADecoderSession>(nullptr, _nmeaScheduler);
    if (!_nmeaSource->start(device)) {
        _nmeaSource.reset();
        return;
    }
    connect(_nmeaSource.get(), &NMEADecoderSession::activityChanged, this, &QGCPositionManager::nmeaActivityChanged);
    _nmeaDeviceClosedConnection = connect(
        device, &QIODevice::aboutToClose, this,
        [this, revision]() {
            if (_nmeaRevision == revision) {
                resetNmeaSourceDevice();
            }
        },
        Qt::QueuedConnection);
    _nmeaDeviceDestroyedConnection = connect(device, &QObject::destroyed, this, [this, revision]() {
        if (_nmeaRevision == revision) {
            resetNmeaSourceDevice();
        }
    });
    auto registration =
        registerPositionSource(SelectedSource::Nmea, _nmeaSource->positionSource(), _nmeaSource->health());
    if (guard && _nmeaRevision == revision) {
        _nmeaRegistration = std::move(registration);
        emit nmeaSourceChanged();
    }
}

void QGCPositionManager::resetNmeaSourceDevice()
{
    if (QThread::currentThread() != thread()) {
        return;
    }
    ++_nmeaRevision;
    QObject::disconnect(_nmeaDeviceDestroyedConnection);
    QObject::disconnect(_nmeaDeviceClosedConnection);
    // Keep the old source alive through retirement signals, even if they replace it or delete this manager.
    auto source = std::move(_nmeaSource);
    auto registration = std::move(_nmeaRegistration);
    const QPointer<QGCPositionManager> guard(this);
    const quint64 revision = _nmeaRevision;
    registration.reset();
    if (guard && revision == _nmeaRevision) {
        emit nmeaSourceChanged();
        if (guard && revision == _nmeaRevision) {
            emit nmeaActivityChanged();
        }
    }
}

GPSSourceHealth* QGCPositionManager::nmeaHealth() const
{
    return _nmeaSource ? _nmeaSource->health() : nullptr;
}

bool QGCPositionManager::nmeaReceiving() const
{
    return _nmeaSource && _nmeaSource->receiving();
}

bool QGCPositionManager::nmeaHasData() const
{
    return _nmeaSource && _nmeaSource->hasReceivedData();
}
