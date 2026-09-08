#include "PositionManager.h"
#include "AppMessages.h"
#include "QGCCorePlugin.h"
#include "SimulatedPosition.h"
// #include "QGCSensors.h"
#include "QGCLoggingCategory.h"

#include <cmath>

#include <QtCore/QApplicationStatic>
#include <QtCore/QPermissions>

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "GPS.PositionManager.QGCPositionManager")

Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject* parent)
    : QObject(parent)
    , _externalHealth(this)
{
    qCDebug(QGCPositionManagerLog) << this;
}

QGCPositionManager::~QGCPositionManager()
{
    qCDebug(QGCPositionManagerLog) << this;
    QObject::disconnect(_healthConnection);
    QObject::disconnect(_healthDestroyedConnection);
}

QGCPositionManager *QGCPositionManager::instance()
{
    return _positionManager();
}

void QGCPositionManager::init()
{
    if (QGC::runningUnitTests()) {
        _simulatedSource = new SimulatedPosition(this);
        _setPositionSource(QGCPositionSource::Simulated);
    } else {
        _checkPermission();
    }
}

void QGCPositionManager::_setupPositionSources()
{
    _defaultSource = QGCCorePlugin::instance()->createPositionSource(this);
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

    _selectPositionSource();
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

    const Qt::PermissionStatus permissionStatus = QCoreApplication::instance()->checkPermission(locationPermission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        QCoreApplication::instance()->requestPermission(locationPermission, this, [this](const QPermission &permission) {
            _handlePermissionStatus(permission.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

void QGCPositionManager::setReceiverPositionSource(QGeoPositionInfoSource* source, GPSSourceHealth* health)
{
    if (_receiverSource == source && _receiverHealth == health) {
        return;
    }
    QObject::disconnect(_receiverDestroyedConnection);
    _receiverSource = source;
    _receiverHealth = source ? health : nullptr;
    if (source) {
        _receiverDestroyedConnection = connect(source, &QObject::destroyed, this, [this, source]() {
            if (_currentSource == source) {
                _currentSource = nullptr;
                _clearPosition();
            }
            _receiverSource = nullptr;
            _receiverHealth = nullptr;
            _selectPositionSource();
        });
    }
    _selectPositionSource();
}

void QGCPositionManager::clearReceiverPositionSource(QGeoPositionInfoSource* source)
{
    if (_receiverSource == source) {
        setReceiverPositionSource(nullptr);
    }
}

bool QGCPositionManager::_isExternalSource() const
{
    return _currentSource && (_currentSource == _nmeaSource || _currentSource == _receiverSource);
}

void QGCPositionManager::_selectPositionSource()
{
    _setPositionSource(_receiverSource ? ExternalGPS : (_nmeaSource ? NmeaGPS : InternalGPS));
}

void QGCPositionManager::setNmeaPositionSource(QGeoPositionInfoSource* source, GPSSourceHealth* health)
{
    if (_nmeaSource == source && _nmeaHealth == health) {
        return;
    }
    QObject::disconnect(_nmeaDestroyedConnection);
    _nmeaSource = source;
    _nmeaHealth = source ? health : nullptr;
    if (source) {
        _nmeaDestroyedConnection = connect(source, &QObject::destroyed, this, [this, source]() {
            if (_currentSource == source) {
                _currentSource = nullptr;
                _clearPosition();
            }
            _nmeaSource = nullptr;
            _nmeaHealth = nullptr;
            _selectPositionSource();
        });
    }
    _selectPositionSource();
}

void QGCPositionManager::clearNmeaPositionSource(QGeoPositionInfoSource* source)
{
    if (_nmeaSource == source) {
        setNmeaPositionSource(nullptr);
    }
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo &update)
{
    if (_isExternalSource()) {
        _externalHealth.updatePosition(update);
        return;
    }
    _geoPositionInfo = update;
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    QGeoCoordinate newGCSPosition(_gcsPosition);

    if (update.isValid() && update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        _gcsPositionHorizontalAccuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
        if (qIsFinite(_gcsPositionHorizontalAccuracy) && _gcsPositionHorizontalAccuracy > 0 &&
            _gcsPositionHorizontalAccuracy <= kMinHorizonalAccuracyMeters) {
            newGCSPosition.setLatitude(update.coordinate().latitude());
            newGCSPosition.setLongitude(update.coordinate().longitude());
            // Stamp the local arrival time so consumers can tell how fresh gcsPosition is.
            // Updates rejected by the accuracy gate leave the stamp alone, since they leave
            // the previous coordinate in place as well.
            _gcsPositionTimestamp = QDateTime::currentDateTimeUtc();
            _gcsPositioningError = QGeoPositionInfoSource::NoError;
        }
        emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
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
    } else if (_usingPluginSource && _currentSource == _defaultSource) {
        _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
    }

    emit positionInfoUpdated(update);
}

void QGCPositionManager::_externalPositionChanged()
{
    if (!_currentHealth) {
        return;
    }
    if (!_currentHealth->usable()) {
        if (_currentHealth->state() != GPSSourceHealth::NoData) {
            _positionError(QGeoPositionInfoSource::UpdateTimeoutError);
        }
        _clearPosition();
        return;
    }
    const GPSObservation observation = _currentHealth->observation();
    const quint64 generation = _sourceGeneration;
    _geoPositionInfo = observation.position;
    _gcsPositionTimestamp = observation.receivedAt;
    _gcsPositioningError = QGeoPositionInfoSource::NoError;
    _gcsPositionHorizontalAccuracy = observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    _gcsPositionVerticalAccuracy = observation.coordinate().type() == QGeoCoordinate::Coordinate3D
                                       ? observation.position.attribute(QGeoPositionInfo::VerticalAccuracy)
                                       : qInf();
    _gcsDirectionAccuracy = observation.position.hasAttribute(QGeoPositionInfo::DirectionAccuracy)
                                ? observation.position.attribute(QGeoPositionInfo::DirectionAccuracy)
                                : qInf();
    _gcsPositionAccuracy = std::hypot(_gcsPositionHorizontalAccuracy, _gcsPositionVerticalAccuracy);
    emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
    if (generation != _sourceGeneration) {
        return;
    }
    _setGCSHeading(observation.heading());
    if (generation != _sourceGeneration) {
        return;
    }
    _setGCSPosition(observation.coordinate());
    if (generation == _sourceGeneration) {
        emit positionInfoUpdated(observation.position);
    }
}

void QGCPositionManager::_positionError(QGeoPositionInfoSource::Error gcsPositioningError)
{
    if (_gcsPositioningError == gcsPositioningError) {
        return;
    }
    _gcsPositioningError = gcsPositioningError;
    if (gcsPositioningError != QGeoPositionInfoSource::NoError) {
        qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << "Positioning error:" << gcsPositioningError;
    }
}

void QGCPositionManager::_setGCSHeading(qreal newGCSHeading)
{
    if (newGCSHeading != _gcsHeading && !(qIsNaN(newGCSHeading) && qIsNaN(_gcsHeading))) {
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

void QGCPositionManager::_clearPosition()
{
    _geoPositionInfo = QGeoPositionInfo();
    _gcsPositionTimestamp = QDateTime();
    _setGCSPosition(QGeoCoordinate());
    _setGCSHeading(qQNaN());
    _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
    _gcsPositionVerticalAccuracy = std::numeric_limits<qreal>::infinity();
    _gcsPositionAccuracy = std::numeric_limits<qreal>::infinity();
    _gcsDirectionAccuracy = std::numeric_limits<qreal>::infinity();
    emit positionInfoUpdated(_geoPositionInfo);
    emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
}

void QGCPositionManager::_setPositionSource(QGCPositionSource source)
{
    QGeoPositionInfoSource* nextSource = nullptr;
    const char* sourceName = "platform";
    switch (source) {
        case Simulated:
            sourceName = "simulated";
            nextSource = _simulatedSource;
            break;
        case NmeaGPS:
            sourceName = "NMEA";
            nextSource = _nmeaSource;
            break;
        case ExternalGPS:
            sourceName = "receiver";
            nextSource = _receiverSource;
            break;
        default:
            nextSource = _defaultSource;
            break;
    }
    GPSSourceHealth* nextHealth = nextSource && (source == ExternalGPS || source == NmeaGPS)
                                      ? (source == ExternalGPS ? _receiverHealth.data() : _nmeaHealth.data())
                                      : nullptr;
    if (nextSource && (source == ExternalGPS || source == NmeaGPS) && !nextHealth) {
        nextHealth = &_externalHealth;
    }
    if (_currentSource && _currentSource == nextSource && _currentHealth == nextHealth) {
        return;
    }
    qCDebug(QGCPositionManagerLog) << "Ground-station position source changed"
                                   << "source:" << (nextSource ? sourceName : "none")
                                   << "previous:" << _currentSource
                                   << "selected:" << nextSource;
    QObject::disconnect(_healthConnection);
    QObject::disconnect(_healthDestroyedConnection);
    _externalHealth.reset();
    QObject::disconnect(_positionUpdateConnection);
    QObject::disconnect(_positionErrorConnection);
    if (_currentSource) {
        _currentSource->stopUpdates();
    }
    ++_sourceGeneration;
    _currentSource = nextSource;
    _currentHealth = nextHealth;
    _clearPosition();
    emit sourceHealthChanged();
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    if (_currentHealth) {
        _healthConnection = connect(_currentHealth, &GPSSourceHealth::positionChanged, this,
                                    &QGCPositionManager::_externalPositionChanged);
        _healthDestroyedConnection = connect(_currentHealth, &QObject::destroyed, this, [this]() {
            _currentHealth = nullptr;
            _clearPosition();
            _selectPositionSource();
        });
    }
    if (_currentSource != nullptr) {
        _currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _updateInterval = _isExternalSource() ? 0 : _currentSource->minimumUpdateInterval();
        if (_isExternalSource()) {
            // Qt's NMEA minimum is 2 ms, which reports timeouts between normal receiver fixes.
            _currentSource->setUpdateInterval(0);
        }
#if !defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)
        _currentSource->setUpdateInterval(_updateInterval);
#endif

        const QPointer<QGeoPositionInfoSource> selectedSource = _currentSource;
        const quint64 generation = _sourceGeneration;
        _positionUpdateConnection =
            connect(_currentSource, &QGeoPositionInfoSource::positionUpdated, this,
                    [this, selectedSource, generation](const QGeoPositionInfo& update) {
                        if (selectedSource && _currentSource == selectedSource && generation == _sourceGeneration) {
                            if (!_currentHealth || _currentHealth == &_externalHealth) {
                                _positionUpdated(update);
                            }
                        }
                    });
        _positionErrorConnection =
            connect(_currentSource, &QGeoPositionInfoSource::errorOccurred, this,
                    [this, selectedSource, generation](QGeoPositionInfoSource::Error error) {
                        if (selectedSource && _currentSource == selectedSource && generation == _sourceGeneration) {
                            if (_currentHealth == &_externalHealth && error != QGeoPositionInfoSource::NoError) {
                                _externalHealth.invalidatePosition();
                            } else if (!_currentHealth) {
                                _positionError(error);
                            }
                        }
                    });

        // (void) connect(QGCCompass::instance(), &QGCCompass::positionUpdated, this, &QGCPositionManager::_positionUpdated);

        _currentSource->startUpdates();
    }
}
