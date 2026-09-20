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
{
    qCDebug(QGCPositionManagerLog) << this;
    if (this->scheduler()) {
        connect(this->scheduler(), &QObject::destroyed, this,
                [this]() { _resetNmeaSourceDevice("scheduler destroyed"); });
    }
}

QGCPositionManager::~QGCPositionManager()
{
    qCDebug(QGCPositionManagerLog) << "Position manager shutdown:" << this;
    _destroying = true;
    blockSignals(true);
    _resetNmeaSourceDevice("manager shutdown");
}

QGCPositionManager* QGCPositionManager::instance()
{
    return _positionManager();
}

void QGCPositionManager::init()
{
    if (!scheduler()) {
        qCWarning(QGCPositionManagerLog) << "Positioning requires a live scheduler";
        return;
    }
    if (QGC::runningUnitTests()) {
        setSimulatedPositionSource(new SimulatedPosition(this, scheduler()));
    } else {
        _checkPermission();
    }
}

void QGCPositionManager::_setupPositionSources()
{
    if (!scheduler()) {
        qCWarning(QGCPositionManagerLog) << "Positioning requires a live scheduler";
        return;
    }
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
    if (QThread::currentThread() != thread() || (device && (device->thread() != thread() || !scheduler()))) {
        qCWarning(QGCPositionManagerLog) << "NMEA device requires matching thread affinity and a live scheduler";
        return;
    }
    const QPointer<QGCPositionManager> guard(this);
    const QPointer<QIODevice> deviceGuard(device);
    const quint64 revision = _nmeaRevision + 1;
    _resetNmeaSourceDevice(device ? "device replacement" : "device cleared");
    if (!guard || _nmeaRevision != revision || !deviceGuard) {
        return;
    }
    _nmeaDevice = deviceGuard;
    _nmeaSource = std::make_unique<NMEADecoderSession>(nullptr, scheduler());
    if (!_nmeaSource->start(device)) {
        qCDebug(QGCPositionManagerLog) << "NMEA session rejected:"
                                       << "reason: decoder start failed"
                                       << "revision:" << revision;
        _nmeaSource.reset();
        _nmeaDevice = nullptr;
        return;
    }
    connect(_nmeaSource.get(), &NMEADecoderSession::activityChanged, this, &QGCPositionManager::nmeaActivityChanged);
    _nmeaDeviceClosedConnection = connect(
        device, &QIODevice::aboutToClose, this,
        [this, revision]() {
            if (_nmeaRevision == revision) {
                _resetNmeaSourceDevice("device closed");
            }
        },
        Qt::QueuedConnection);
    _nmeaDeviceDestroyedConnection = connect(device, &QObject::destroyed, this, [this, revision]() {
        if (_nmeaRevision == revision) {
            _resetNmeaSourceDevice("device destroyed");
        }
    });
    auto registration =
        registerPositionSource(SelectedSource::Nmea, _nmeaSource->positionSource(), _nmeaSource->health());
    if (guard && _nmeaRevision == revision) {
        _nmeaRegistration = std::move(registration);
        qCDebug(QGCPositionManagerLog) << "NMEA session installed:"
                                       << "revision:" << revision << "device:" << static_cast<const void*>(device)
                                       << "registered:" << bool(_nmeaRegistration);
        emit nmeaSourceChanged();
    }
}

void QGCPositionManager::resetNmeaSourceDevice()
{
    _resetNmeaSourceDevice("reset requested");
}

void QGCPositionManager::resetNmeaSourceDevice(QIODevice* expectedDevice)
{
    if (expectedDevice && _nmeaDevice == expectedDevice) {
        _resetNmeaSourceDevice("reset requested");
    }
}

QIODevice* QGCPositionManager::nmeaSourceDevice() const
{
    return _nmeaDevice;
}

void QGCPositionManager::_resetNmeaSourceDevice(const char* reason)
{
    if (QThread::currentThread() != thread()) {
        return;
    }
    if (_nmeaSource) {
        qCDebug(QGCPositionManagerLog) << "NMEA session retired:"
                                       << "reason:" << reason << "revision:" << _nmeaRevision;
    }
    ++_nmeaRevision;
    _nmeaDevice = nullptr;
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
