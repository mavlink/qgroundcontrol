#include "PositionManager.h"
#include "AppMessages.h"
#include "QGCCorePlugin.h"
#include "SimulatedPosition.h"
// #include "QGCSensors.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QPermissions>
#include <QtPositioning/QNmeaPositionInfoSource>

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "GPS.PositionManager.QGCPositionManager")

Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject* parent)
    : QObject(parent)
    , _externalStaleTimer(this)
{
    qCDebug(QGCPositionManagerLog) << this;

    _externalStaleTimer.setSingleShot(true);
    _externalStaleTimer.setInterval(std::chrono::seconds(5));
    connect(&_externalStaleTimer, &QTimer::timeout, this, [this]() {
        qCDebug(QGCPositionManagerLog) << "External position source timed out"
                                       << "source:" << _currentSource;
        _positionError(QGeoPositionInfoSource::UpdateTimeoutError);
        _clearPosition();
    });
}

QGCPositionManager::~QGCPositionManager()
{
    qCDebug(QGCPositionManagerLog) << this;
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

void QGCPositionManager::setReceiverPositionSource(QGeoPositionInfoSource* source)
{
    if (_receiverSource == source) {
        return;
    }
    QObject::disconnect(_receiverDestroyedConnection);
    _receiverSource = source;
    if (source) {
        _receiverDestroyedConnection = connect(source, &QObject::destroyed, this, [this, source]() {
            if (_currentSource == source) {
                _currentSource = nullptr;
                _clearPosition();
            }
            _receiverSource = nullptr;
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
    if (_receiverSource && _nmeaSource) {
        _nmeaNeedsRestart = true;
    } else if (_nmeaNeedsRestart && _nmeaSource) {
        _nmeaNeedsRestart = false;
        // Discard input and parser state accumulated while the receiver had priority.
        auto* device = _nmeaSource->device();
        if (device) {
            device->readAll();
            setNmeaSourceDevice(device);
            return;
        }
    }
    _setPositionSource(_receiverSource ? ExternalGPS : (_nmeaSource ? NmeaGPS : InternalGPS));
}

void QGCPositionManager::setNmeaSourceDevice(QIODevice *device)
{
    if (_nmeaSource) {
        _nmeaSource->stopUpdates();
        (void) _nmeaSource->disconnect(this);

        if (_currentSource == _nmeaSource) {
            _currentSource = nullptr;
        }

        delete _nmeaSource;
        _nmeaSource = nullptr;
    }

    _nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
    _nmeaSource->setDevice(device);
    _nmeaSource->setUserEquivalentRangeError(5.1);
    _selectPositionSource();
}

void QGCPositionManager::resetNmeaSourceDevice()
{
    if (!_nmeaSource) {
        return;
    }

    auto* retiredSource = _nmeaSource;
    _nmeaSource = nullptr;
    _nmeaNeedsRestart = false;
    _selectPositionSource();
    delete retiredSource;
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo &update)
{
    const bool receiver = _receiverSource && _currentSource == _receiverSource;
    if (receiver) {
        const double accuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
        if (!update.isValid() || !update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) || !qIsFinite(accuracy) ||
            accuracy <= 0 || accuracy > kMinHorizonalAccuracyMeters) {
            qCDebug(QGCPositionManagerLog) << "Rejected RTK ground-station fix: invalid position or horizontal accuracy"
                                           << "valid:" << update.isValid()
                                           << "horizontalAccuracy:" << accuracy
                                           << "accuracyLimit:" << kMinHorizonalAccuracyMeters;
            _externalStaleTimer.stop();
            _clearPosition();
            _positionError(QGeoPositionInfoSource::UpdateTimeoutError);
            return;
        }
        _gcsPositionVerticalAccuracy = std::numeric_limits<qreal>::infinity();
        _gcsDirectionAccuracy = std::numeric_limits<qreal>::infinity();
        if (!update.hasAttribute(QGeoPositionInfo::DirectionAccuracy) ||
            update.attribute(QGeoPositionInfo::DirectionAccuracy) > kMinDirectionAccuracyDegrees) {
            _setGCSHeading(qQNaN());
        }
    }
    _geoPositionInfo = update;
    if (!_isExternalSource()) {
        _gcsPositioningError = QGeoPositionInfoSource::NoError;
    }

    QGeoCoordinate newGCSPosition(receiver ? QGeoCoordinate() : _gcsPosition);

    if (update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        if (receiver ||
            ((qAbs(update.coordinate().latitude()) > 0.001) && (qAbs(update.coordinate().longitude()) > 0.001))) {
            _gcsPositionHorizontalAccuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
            if (_gcsPositionHorizontalAccuracy <= kMinHorizonalAccuracyMeters) {
                newGCSPosition.setLatitude(update.coordinate().latitude());
                newGCSPosition.setLongitude(update.coordinate().longitude());
                // Stamp the local arrival time so consumers can tell how fresh gcsPosition is.
                // Updates rejected by the accuracy gate leave the stamp alone, since they leave
                // the previous coordinate in place as well.
                _gcsPositionTimestamp = QDateTime::currentDateTimeUtc();
                _gcsPositioningError = QGeoPositionInfoSource::NoError;
                if (_isExternalSource()) {
                    _externalStaleTimer.start();
                }
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
    } else if (_usingPluginSource && _currentSource == _defaultSource) {
        _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
    }

    if (receiver) {
        qCDebug(QGCPositionManagerLog) << "Accepted RTK ground-station fix"
                                       << "coordinate:" << _gcsPosition
                                       << "horizontalAccuracy:" << _gcsPositionHorizontalAccuracy
                                       << "verticalAccuracy:" << _gcsPositionVerticalAccuracy
                                       << "heading:" << _gcsHeading
                                       << "arrivalTime:" << _gcsPositionTimestamp;
    }
    emit positionInfoUpdated(update);
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
    if (_currentSource && _currentSource == nextSource) {
        return;
    }
    qCDebug(QGCPositionManagerLog) << "Ground-station position source changed"
                                   << "source:" << (nextSource ? sourceName : "none")
                                   << "previous:" << _currentSource
                                   << "selected:" << nextSource;
    _externalStaleTimer.stop();
    if (_currentSource) {
        _currentSource->stopUpdates();
        (void) _currentSource->disconnect(this);
    }
    ++_sourceGeneration;
    _currentSource = nextSource;
    _clearPosition();
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    if (_currentSource != nullptr) {
        _currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _updateInterval = _isExternalSource() ? 0 : _currentSource->minimumUpdateInterval();
        if (_isExternalSource()) {
            // Qt's NMEA minimum is 2 ms, which reports timeouts between normal receiver fixes.
            _currentSource->setUpdateInterval(0);
            _externalStaleTimer.start();
        }
#if !defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)
        _currentSource->setUpdateInterval(_updateInterval);
#endif

        const QPointer<QGeoPositionInfoSource> selectedSource = _currentSource;
        const quint64 generation = _sourceGeneration;
        connect(_currentSource, &QGeoPositionInfoSource::positionUpdated, this,
                [this, selectedSource, generation](const QGeoPositionInfo& update) {
                    if (selectedSource && _currentSource == selectedSource && generation == _sourceGeneration) {
                        _positionUpdated(update);
                    }
                });
        connect(_currentSource, &QGeoPositionInfoSource::errorOccurred, this,
                [this, selectedSource, generation](QGeoPositionInfoSource::Error error) {
                    if (selectedSource && _currentSource == selectedSource && generation == _sourceGeneration) {
                        _positionError(error);
                        if (_receiverSource && _currentSource == _receiverSource &&
                            error != QGeoPositionInfoSource::NoError) {
                            _clearPosition();
                        }
                    }
                });

        // (void) connect(QGCCompass::instance(), &QGCCompass::positionUpdated, this, &QGCPositionManager::_positionUpdated);

        _currentSource->startUpdates();
    }
}
