#include "PositionManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QPermissions>

#include "AppMessages.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "SimulatedPosition.h"
QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "GPS.Integration.QGCPositionManager")
Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject* parent)
    : GPSPositionService(parent)
{
    qCDebug(QGCPositionManagerLog) << this;
}

QGCPositionManager::~QGCPositionManager()
{
    qCDebug(QGCPositionManagerLog) << this;
}

QGCPositionManager* QGCPositionManager::instance()
{
    return _positionManager();
}

void QGCPositionManager::init()
{
    if (QGC::runningUnitTests()) {
        _simulatedSource = new SimulatedPosition(this);
        _selectPositionSource();
    } else {
        _checkPermission();
    }
}

void QGCPositionManager::_setupPositionSources()
{
    _setupPositionSources(
        QGCCorePlugin::instance()->createPositionSource(this),
        [](const QString& name, QObject* parent) { return QGeoPositionInfoSource::createSource(name, parent); });
}

void QGCPositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);

    const Qt::PermissionStatus permissionStatus = QCoreApplication::instance()->checkPermission(locationPermission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        _platformStatus = SourceStatus::PermissionRequired;
        const QPointer<QGCPositionManager> guard(this);
        _selectPositionSource();
        if (!guard) {
            return;
        }
        QCoreApplication::instance()->requestPermission(
            locationPermission, this,
            [this](const QPermission& permission) { _handlePermissionStatus(permission.status()); });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

QString QGCPositionManager::_platformSourceName()
{
#if defined(Q_OS_ANDROID)
    return QStringLiteral("android");
#elif defined(Q_OS_IOS) || defined(Q_OS_MACOS)
    return QStringLiteral("corelocation");
#elif defined(Q_OS_WIN)
    return QStringLiteral("winrt");
#elif defined(Q_OS_WASM)
    return QStringLiteral("wasm");
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD) || defined(Q_OS_NETBSD) || \
    defined(Q_OS_HURD)
    return QStringLiteral("geoclue2");
#else
    return {};
#endif
}

void QGCPositionManager::_setupPositionSources(
    QGeoPositionInfoSource* customSource,
    const std::function<QGeoPositionInfoSource*(const QString&, QObject*)>& createPlatformSource)
{
    _defaultSource = customSource;
    _usingPluginSource = customSource != nullptr;
    if (!_defaultSource) {
        const QString name = _platformSourceName();
        qCDebug(QGCPositionManagerLog) << "Platform positioning provider:" << name;
        // Qt's default-source fallback can open a serial NMEA device outside QGC's reservations.
        if (!name.isEmpty()) {
            _defaultSource = createPlatformSource(name, this);
        }
    }
    const auto status = _defaultSource ? SourceStatus::WaitingForFix : SourceStatus::BackendUnavailable;
    if (!_defaultSource) {
        qCWarning(QGCPositionManagerLog) << "Platform positioning backend unavailable";
    }
    setInternalPositionSource(_defaultSource, status, _usingPluginSource);
}

void QGCPositionManager::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    if (permissionStatus == Qt::PermissionStatus::Granted) {
        _platformStatus = SourceStatus::WaitingForFix;
        _setupPositionSources();
    } else {
        _platformStatus = SourceStatus::PermissionDenied;
        qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << "Location Permission Denied";
        _selectPositionSource();
    }
}
