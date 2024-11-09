/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PositionManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "SimulatedPosition.h"
#include "DeviceInfo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QPermissions>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/private/qgeopositioninfosource_p.h>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtQml/QtQml>

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "qgc.positionmanager.positionmanager")

Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject *parent)
    : QObject(parent)
{
    // qCDebug(QGCPositionManagerLog) << Q_FUNC_INFO << this;

    (void) qmlRegisterUncreatableType<QGCPositionManager>("QGroundControl.QGCPositionManager", 1, 0, "QGCPositionManager", "Reference only");
}

QGCPositionManager::~QGCPositionManager()
{
    // qCDebug(QGCPositionManagerLog) << Q_FUNC_INFO << this;
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
}

void QGCPositionManager::_setupPositionSources()
{
    _defaultSource = qgcApp()->toolbox()->corePlugin()->createPositionSource(this);
    if (_defaultSource) {
        _usingPluginSource = true;
    } else {
        qCDebug(QGCPositionManagerLog) << Q_FUNC_INFO << QGeoPositionInfoSource::availableSources();

        _defaultSource = QGeoPositionInfoSource::createDefaultSource(this);
        if (!_defaultSource) {
            qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << "No default source available";
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
        qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << "Location Permission Denied";
    }
}

void QGCPositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);

    const Qt::PermissionStatus permissionStatus = qgcApp()->checkPermission(locationPermission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        qgcApp()->requestPermission(locationPermission, this, [this](const QPermission &permission) {
            _handlePermissionStatus(permission.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
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
    _setPositionSource(QGCPositionManager::NmeaGPS);
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo &update)
{
    _geoPositionInfo = update;

    QGeoCoordinate newGCSPosition(_gcsPosition);

    if (update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        if ((qAbs(update.coordinate().latitude()) > 0.001) && (qAbs(update.coordinate().longitude()) > 0.001)) {
            _gcsPositionHorizontalAccuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
            if (_gcsPositionHorizontalAccuracy <= kMinHorizonalAccuracyMeters) {
                newGCSPosition.setLatitude(update.coordinate().latitude());
                newGCSPosition.setLongitude(update.coordinate().longitude());
            }
            emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
        }
    }

    if (update.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        _gcsPositionVerticalAccuracy = update.attribute(QGeoPositionInfo::VerticalAccuracy);
        if (_gcsPositionVerticalAccuracy <= kMinVerticalAccuracyMeters) {
            newGCSPosition.setAltitude(update.coordinate().altitude());
        }
    }

    _gcsPositionAccuracy = sqrt(pow(_gcsPositionHorizontalAccuracy, 2) + pow(_gcsPositionVerticalAccuracy, 2));

    _setGCSPosition(newGCSPosition);

    if (update.hasAttribute(QGeoPositionInfo::DirectionAccuracy)) {
        _gcsDirectionAccuracy = update.attribute(QGeoPositionInfo::DirectionAccuracy);
        if (_gcsDirectionAccuracy <= kMinDirectionAccuracyDegrees) {
            _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
        }
    } else if (_usingPluginSource) {
        _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
    }

    emit positionInfoUpdated(update);
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
        _gcsPosition = QGeoCoordinate();
        _gcsHeading = qQNaN();
        _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();

        emit gcsPositionChanged(_gcsPosition);
        emit gcsHeadingChanged(_gcsHeading);
        emit positionInfoUpdated(_geoPositionInfo);
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
        break;
    default:
        _currentSource = _defaultSource;
        break;
    }

    if (_currentSource != nullptr) {
        _currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _updateInterval = _currentSource->minimumUpdateInterval();
        #if !defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)
            _currentSource->setUpdateInterval(_updateInterval);
        #endif
        (void) connect(_currentSource, &QGeoPositionInfoSource::positionUpdated, this, &QGCPositionManager::_positionUpdated);
        (void) connect(_currentSource, &QGeoPositionInfoSource::errorOccurred, this, [](QGeoPositionInfoSource::Error positioningError) {
            qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << positioningError;
        });

        // (void) connect(QGCCompass::instance(), &QGCCompass::positionUpdated, this, &QGCPositionManager::_positionUpdated);

        _currentSource->startUpdates();
    }
}
