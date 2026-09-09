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
            const QPointer<QGCPositionManager> guard(this);
            _receiverSource = nullptr;
            _receiverHealth = nullptr;
            if (_currentSource == source) {
                _currentSource = nullptr;
                _clearPosition();
            }
            if (guard) {
                _selectPositionSource();
            }
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
            const QPointer<QGCPositionManager> guard(this);
            _nmeaSource = nullptr;
            _nmeaHealth = nullptr;
            if (_currentSource == source) {
                _currentSource = nullptr;
                _clearPosition();
            }
            if (guard) {
                _selectPositionSource();
            }
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

std::optional<GPSObservation> QGCPositionManager::acceptedObservation(GPSObservation::PositionUse use) const
{
    // Selecting a standby source waits for a new observation, even when its cache is fresh.
    return _currentHealth && _gcsPosition.isValid() ? _currentHealth->acceptedObservation(use) : std::nullopt;
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo& update)
{
    GPSObservation observation;
    observation.position = update;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.sourceId = _isExternalSource()
                               ? QStringLiteral("External GPS")
                               : (_usingPluginSource ? QStringLiteral("Plugin") : QStringLiteral("Platform"));
    _externalHealth.updateObservation(observation);
}

void QGCPositionManager::_externalPositionChanged()
{
    if (!_currentHealth) {
        return;
    }
    const auto accepted = _currentHealth->acceptedObservation(GPSObservation::PositionUse::GroundStation);
    if (!accepted) {
        if (_currentHealth->state() != GPSSourceHealth::NoData) {
            _positionError(QGeoPositionInfoSource::UpdateTimeoutError);
        }
        _clearPosition();
        return;
    }
    _gcsPositioningError = QGeoPositionInfoSource::NoError;
    _publishPosition(accepted);
}

void QGCPositionManager::_publishPosition(const std::optional<GPSObservation>& observation)
{
    const QPointer<QGCPositionManager> guard(this);
    const quint64 generation = _sourceGeneration;
    const quint64 revision = ++_positionRevision;
    const QGeoCoordinate previousPosition = _gcsPosition;
    const qreal previousHeading = _gcsHeading;
    if (observation) {
        // Preserve the raw fix for diagnostics; consumers request their own accepted projection.
        _geoPositionInfo = _currentHealth->observation().position;
        _gcsPosition = observation->position.coordinate();
        _gcsPositionTimestamp = observation->receivedAt;
        _gcsHeading = observation->heading();
        _gcsPositionHorizontalAccuracy = observation->position.attribute(QGeoPositionInfo::HorizontalAccuracy);
        _gcsPositionVerticalAccuracy = observation->position.hasAttribute(QGeoPositionInfo::VerticalAccuracy)
                                           ? observation->position.attribute(QGeoPositionInfo::VerticalAccuracy)
                                           : qInf();
        _gcsDirectionAccuracy = observation->position.hasAttribute(QGeoPositionInfo::DirectionAccuracy)
                                    ? observation->position.attribute(QGeoPositionInfo::DirectionAccuracy)
                                    : qInf();
        _gcsPositionAccuracy = std::hypot(_gcsPositionHorizontalAccuracy, _gcsPositionVerticalAccuracy);
    } else {
        _geoPositionInfo = {};
        _gcsPosition = {};
        _gcsPositionTimestamp = {};
        _gcsHeading = qQNaN();
        _gcsPositionHorizontalAccuracy = qInf();
        _gcsPositionVerticalAccuracy = qInf();
        _gcsPositionAccuracy = qInf();
        _gcsDirectionAccuracy = qInf();
    }
    emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
    if (!guard || generation != _sourceGeneration || revision != _positionRevision) {
        return;
    }
    if (_gcsHeading != previousHeading && !(qIsNaN(_gcsHeading) && qIsNaN(previousHeading))) {
        emit gcsHeadingChanged(_gcsHeading);
    }
    if (!guard || generation != _sourceGeneration || revision != _positionRevision) {
        return;
    }
    if (_gcsPosition != previousPosition) {
        emit gcsPositionChanged(_gcsPosition);
    }
    if (guard && generation == _sourceGeneration && revision == _positionRevision) {
        emit positionInfoUpdated(_geoPositionInfo);
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

void QGCPositionManager::_clearPosition()
{
    _publishPosition(std::nullopt);
}

void QGCPositionManager::_setPositionSource(QGCPositionSource source)
{
    const QPointer<QGCPositionManager> guard(this);
    QPointer<QGeoPositionInfoSource> nextSource;
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
    QPointer<GPSSourceHealth> nextHealth = nextSource && (source == ExternalGPS || source == NmeaGPS)
                                               ? (source == ExternalGPS ? _receiverHealth.data() : _nmeaHealth.data())
                                               : nullptr;
    if (nextSource && !nextHealth) {
        nextHealth = &_externalHealth;
    }
    if (_currentSource && _currentSource == nextSource && _currentHealth == nextHealth) {
        return;
    }
    qCDebug(QGCPositionManagerLog) << "Ground-station position source changed"
                                   << "source:" << (nextSource ? sourceName : "none")
                                   << "previous:" << _currentSource
                                   << "selected:" << nextSource;
    const quint64 generation = ++_sourceGeneration;
    QObject::disconnect(_healthConnection);
    QObject::disconnect(_healthDestroyedConnection);
    _externalHealth.reset();
    if (!guard || generation != _sourceGeneration) {
        return;
    }
    QObject::disconnect(_positionUpdateConnection);
    QObject::disconnect(_positionErrorConnection);
    if (_currentSource) {
        _currentSource->stopUpdates();
    }
    if (!guard || generation != _sourceGeneration) {
        return;
    }
    _currentSource = nextSource;
    _currentHealth = nextHealth;
    _clearPosition();
    if (!guard || generation != _sourceGeneration) {
        return;
    }
    emit sourceHealthChanged();
    if (!guard || generation != _sourceGeneration) {
        return;
    }
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    if (_currentHealth) {
        _healthConnection = connect(_currentHealth, &GPSSourceHealth::positionChanged, this,
                                    &QGCPositionManager::_externalPositionChanged);
        _healthDestroyedConnection = connect(_currentHealth, &QObject::destroyed, this, [this]() {
            const QPointer<QGCPositionManager> managerGuard(this);
            _currentHealth = nullptr;
            _clearPosition();
            if (managerGuard) {
                _selectPositionSource();
            }
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
