#include "PositionManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "SimulatedPosition.h"
#include "QGCLoggingCategory.h"

#include "GPSManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QPermissions>
#include <QtPositioning/QNmeaPositionInfoSource>

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "PositionManager.QGCPositionManager")

Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(QGCPositionManagerLog) << this;
}

QGCPositionManager::~QGCPositionManager()
{
    if (_currentSource) {
        _currentSource->stopUpdates();
        (void) disconnect(_currentSource);
    }

    qCDebug(QGCPositionManagerLog) << this;
}

QGCPositionManager *QGCPositionManager::instance()
{
    return _positionManager();
}

void QGCPositionManager::init()
{
    if (qgcApp()->runningUnitTests()) {
        _simulatedSource = new SimulatedPosition(this);
        _setPositionSource(QGCPositionSource::Simulated);
    } else {
        _checkPermission();
    }

    (void) connect(GPSManager::instance(), &GPSManager::positionSourceChanged, this, [this]() {
        auto *source = GPSManager::instance()->gpsPositionSource();
        setExternalGPSSource(source);
    });
}

void QGCPositionManager::_setupPositionSources()
{
    _defaultSource = QGCCorePlugin::instance()->createPositionSource(this);
    if (_defaultSource) {
        _usingPluginSource = true;
    } else {
        qCDebug(QGCPositionManagerLog) << QGeoPositionInfoSource::availableSources();

        _defaultSource = QGeoPositionInfoSource::createDefaultSource(this);
        if (!_defaultSource) {
            qCWarning(QGCPositionManagerLog) << "No default source available";
            return;
        }
    }

    _setPositionSource(QGCPositionSource::InternalGPS);
}

void QGCPositionManager::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    if (permissionStatus == Qt::PermissionStatus::Granted) {
        _setupPositionSources();
    } else {
        qCWarning(QGCPositionManagerLog) << "Location Permission Denied";
    }
}

void QGCPositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);

    const Qt::PermissionStatus permissionStatus = QCoreApplication::instance()->checkPermission(locationPermission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        QCoreApplication::instance()->requestPermission(locationPermission, this, [this](const QPermission &permission) {
            _handlePermissionStatus(permission.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

void QGCPositionManager::setExternalGPSSource(QGeoPositionInfoSource *source)
{
    if (_externalSource == source) {
        return;
    }

    if (_externalSource && _currentSource == _externalSource) {
        _externalSource->stopUpdates();
        (void) disconnect(_externalSource);
        _currentSource = nullptr;
    }

    // We do NOT own the external source — it is owned by GPSRtk
    _externalSource = source;

    if (_externalSource) {
        qCDebug(QGCPositionManagerLog) << "Switching to external GPS position source";
        _setPositionSource(QGCPositionManager::ExternalGPS);
    } else {
        qCDebug(QGCPositionManagerLog) << "External GPS removed, falling back";
        if (_nmeaSource) {
            _setPositionSource(QGCPositionManager::NmeaGPS);
        } else if (_defaultSource) {
            _setPositionSource(QGCPositionManager::InternalGPS);
        }
    }
}

void QGCPositionManager::setNmeaSourceDevice(QIODevice *device)
{
    if (_nmeaSource) {
        _nmeaSource->stopUpdates();
        (void) disconnect(_nmeaSource);

        if (_currentSource == _nmeaSource) {
            _currentSource = nullptr;
        }

        delete _nmeaSource;
        _nmeaSource = nullptr;
    }

    _nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
    _nmeaSource->setDevice(device);
    _nmeaSource->setUserEquivalentRangeError(5.1);

    // External GPS (RTK base station) takes priority over NMEA
    if (!_externalSource) {
        _setPositionSource(QGCPositionManager::NmeaGPS);
    }
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo &update)
{
    _geoPositionInfo = update;
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    QGeoCoordinate newGCSPosition(_gcsPosition);

    const bool hasValidCoord = (qAbs(update.coordinate().latitude()) > 0.001)
                            && (qAbs(update.coordinate().longitude()) > 0.001);

    if (hasValidCoord) {
        if (update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
            _gcsPositionHorizontalAccuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
            emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);

            if (_gcsPositionHorizontalAccuracy <= kMinHorizonalAccuracyMeters) {
                newGCSPosition.setLatitude(update.coordinate().latitude());
                newGCSPosition.setLongitude(update.coordinate().longitude());
            }
        } else {
            // No accuracy info available — accept the position as-is
            newGCSPosition.setLatitude(update.coordinate().latitude());
            newGCSPosition.setLongitude(update.coordinate().longitude());
        }
    }

    if (update.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        _gcsPositionVerticalAccuracy = update.attribute(QGeoPositionInfo::VerticalAccuracy);
        if (_gcsPositionVerticalAccuracy <= kMinVerticalAccuracyMeters) {
            newGCSPosition.setAltitude(update.coordinate().altitude());
        }
    } else if (hasValidCoord && !qIsNaN(update.coordinate().altitude())) {
        newGCSPosition.setAltitude(update.coordinate().altitude());
    }

    _gcsPositionAccuracy = sqrt(pow(_gcsPositionHorizontalAccuracy, 2) + pow(_gcsPositionVerticalAccuracy, 2));

    _setGCSPosition(newGCSPosition);

    if (update.hasAttribute(QGeoPositionInfo::Direction)) {
        if (update.hasAttribute(QGeoPositionInfo::DirectionAccuracy)) {
            _gcsDirectionAccuracy = update.attribute(QGeoPositionInfo::DirectionAccuracy);
            if (_gcsDirectionAccuracy <= kMinDirectionAccuracyDegrees) {
                _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
            }
        } else {
            // No accuracy info — accept direction as-is (NMEA COG, plugin sources, etc.)
            _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
        }
    }

    emit positionInfoUpdated(update);
}

void QGCPositionManager::_positionError(QGeoPositionInfoSource::Error gcsPositioningError)
{
    _gcsPositioningError = gcsPositioningError;

    if (gcsPositioningError == QGeoPositionInfoSource::UpdateTimeoutError) {
        qCDebug(QGCPositionManagerLog) << "Position update timeout";
    } else {
        qCWarning(QGCPositionManagerLog) << "Positioning error:" << gcsPositioningError;
    }
}

void QGCPositionManager::_setGCSHeading(qreal newGCSHeading)
{
    if (newGCSHeading != _gcsHeading) {
        _gcsHeading = newGCSHeading;
        emit gcsHeadingChanged(_gcsHeading);
    }
}

void QGCPositionManager::_setGCSPosition(const QGeoCoordinate& newGCSPosition)
{
    if (newGCSPosition != _gcsPosition) {
        _gcsPosition = newGCSPosition;
        emit gcsPositionChanged(_gcsPosition);
    }
}

void QGCPositionManager::_setPositionSource(QGCPositionSource source)
{
    if (_currentSource != nullptr) {
        _currentSource->stopUpdates();
        (void) disconnect(_currentSource);

        _geoPositionInfo = QGeoPositionInfo();
        emit positionInfoUpdated(_geoPositionInfo);

        _setGCSPosition(QGeoCoordinate());

        _setGCSHeading(qQNaN());

        _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
        emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
    }

    switch (source) {
    case QGCPositionManager::Log:
        break;
    case QGCPositionManager::Simulated:
        _currentSource = _simulatedSource;
        break;
    case QGCPositionManager::NmeaGPS:
        _currentSource = _nmeaSource;
        break;
    case QGCPositionManager::InternalGPS:
        _currentSource = _defaultSource;
        break;
    case QGCPositionManager::ExternalGPS:
        _currentSource = _externalSource;
        break;
    default:
        _currentSource = _defaultSource;
        break;
    }

    if (_currentSource != nullptr) {
        _currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _updateInterval = qMax(_currentSource->minimumUpdateInterval(), kMinUpdateIntervalMs);
        #if !defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)
            _currentSource->setUpdateInterval(_updateInterval);
        #endif

        (void) connect(_currentSource, &QGeoPositionInfoSource::positionUpdated, this, &QGCPositionManager::_positionUpdated);
        (void) connect(_currentSource, &QGeoPositionInfoSource::errorOccurred, this, &QGCPositionManager::_positionError);

        // (void) connect(QGCCompass::instance(), &QGCCompass::positionUpdated, this, &QGCPositionManager::_positionUpdated);

        _currentSource->startUpdates();
    }
}
