#include "PositionManager.h"
#include "AppMessages.h"
#include "QGCCorePlugin.h"
#include "SimulatedPosition.h"
// #include "QGCSensors.h"
#include <QtCore/QApplicationStatic>
#include <QtCore/QPermissions>
#include <QtCore/QThread>

#include <cmath>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "GPS.PositionManager.QGCPositionManager")

Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject* parent)
    : QObject(parent)
    , _recoveryTimer(this)
    , _externalHealth(this)
{
    qCDebug(QGCPositionManagerLog) << this;
    _recoveryTimer.setSingleShot(true);
    _recoveryTimer.setTimerType(Qt::PreciseTimer);
    _recoveryTimer.setInterval(5000);
    connect(&_recoveryTimer, &QTimer::timeout, this, &QGCPositionManager::_selectPositionSource);
}

QGCPositionManager::~QGCPositionManager()
{
    qCDebug(QGCPositionManagerLog) << this;
    QObject::disconnect(_healthConnection);
    QObject::disconnect(_healthDestroyedConnection);
    for (auto& watched : _automaticSources) {
        for (const auto& connection : watched.connections) {
            QObject::disconnect(connection);
        }
        if (watched.source) {
            watched.source->disconnect(this);
            watched.source->stopUpdates();
        }
    }
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
            _platformStatus = SourceStatus::BackendUnavailable;
            _selectPositionSource();
            return;
        }
    }

    _selectPositionSource();
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
        QCoreApplication::instance()->requestPermission(locationPermission, this, [this](const QPermission &permission) {
            _handlePermissionStatus(permission.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

void QGCPositionManager::setReceiverPositionSource(QGeoPositionInfoSource* source, GPSSourceHealth* health)
{
    if (QThread::currentThread() != thread() || (source && source->thread() != thread()) ||
        (health && health->thread() != thread())) {
        qCWarning(QGCPositionManagerLog) << "Position source changes require matching thread affinity";
        return;
    }
    if (_receiverSource == source && _receiverHealth == health) {
        return;
    }
    ++_positionRevision;
    QObject::disconnect(_receiverDestroyedConnection);
    _receiverSource = source;
    _receiverHealth = source ? health : nullptr;
    if (source) {
        _receiverDestroyedConnection = connect(source, &QObject::destroyed, this, [this, source]() {
            const QPointer<QGCPositionManager> guard(this);
            ++_positionRevision;
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

void QGCPositionManager::setSourceMode(SourceMode mode)
{
    if (QThread::currentThread() != thread()) {
        qCWarning(QGCPositionManagerLog) << "Position source changes require matching thread affinity";
        return;
    }
    if (mode < SourceMode::LegacyPriority || mode > SourceMode::InternalOnly || _sourceMode == mode) {
        return;
    }
    _sourceMode = mode;
    _forceSourceRefresh = true;
    ++_sourceGeneration;
    _recoveryCandidate.reset();
    _recoveryTimer.stop();
    const QPointer<QGCPositionManager> guard(this);
    _selectPositionSource();
    if (guard) {
        emit sourceModeChanged();
    }
}

QGeoPositionInfoSource* QGCPositionManager::_sourceFor(QGCPositionSource source) const
{
    switch (source) {
        case Simulated:
            return _simulatedSource;
        case InternalGPS:
            return _defaultSource;
        case NmeaGPS:
            return _nmeaSource;
        case ExternalGPS:
            return _receiverSource;
        case Log:
            return nullptr;
    }
    return nullptr;
}

GPSSourceHealth* QGCPositionManager::_automaticHealthFor(QGCPositionSource source) const
{
    return _automaticSources[static_cast<size_t>(source)].health;
}

void QGCPositionManager::_selectPositionSource()
{
    _selectionPending = true;
    if (_selectingSource) {
        return;
    }
    _selectingSource = true;
    const QPointer<QGCPositionManager> guard(this);
    do {
        _selectionPending = false;
        if (_sourceMode == SourceMode::Automatic) {
            _refreshAutomaticSources();
        } else if (_monitoringAutomatic) {
            _stopAutomaticSources();
        }
        if (!guard) {
            return;
        }
        _setPositionSource(_choosePositionSource());
        if (!guard) {
            return;
        }
        if (_selectionPending) {
            _forceSourceRefresh = true;
            continue;
        }
        _updateSelectionStatus();
        if (!guard) {
            return;
        }
    } while (_selectionPending);
    _selectingSource = false;
}

QGCPositionManager::QGCPositionSource QGCPositionManager::_choosePositionSource()
{
    const QGCPositionSource internal = _defaultSource ? InternalGPS : (_simulatedSource ? Simulated : InternalGPS);
    switch (_sourceMode) {
        case SourceMode::ReceiverOnly:
            return ExternalGPS;
        case SourceMode::NmeaOnly:
            return NmeaGPS;
        case SourceMode::InternalOnly:
            return internal;
        case SourceMode::LegacyPriority:
            return _receiverSource ? ExternalGPS : (_nmeaSource ? NmeaGPS : internal);
        case SourceMode::Automatic:
            break;
    }
    const std::array priority = {ExternalGPS, NmeaGPS, internal};
    std::optional<QGCPositionSource> best;
    std::optional<QGCPositionSource> current;
    for (const auto source : priority) {
        if (_sourceFor(source) && _sourceFor(source) == _currentSource) {
            current = source;
        }
        const auto* health = _automaticHealthFor(source);
        if (!best && _sourceFor(source) && health &&
            health->acceptedObservation(GPSObservation::PositionUse::GroundStation)) {
            best = source;
        }
    }
    const auto* currentHealth = current ? _automaticHealthFor(*current) : nullptr;
    if (best && current && *best != *current && currentHealth &&
        currentHealth->acceptedObservation(GPSObservation::PositionUse::GroundStation)) {
        if (_recoveryCandidate != best) {
            _recoveryCandidate = best;
            _recoveryElapsed.start();
            _recoveryTimer.start();
        }
        if (_recoveryElapsed.elapsed() < _recoveryTimer.interval()) {
            return *current;
        }
    }
    _recoveryCandidate.reset();
    _recoveryTimer.stop();
    if (best) {
        return *best;
    }
    return current.value_or(_receiverSource ? ExternalGPS : (_nmeaSource ? NmeaGPS : internal));
}

void QGCPositionManager::_refreshAutomaticSources()
{
    _monitoringAutomatic = true;
    const QPointer<QGCPositionManager> guard(this);
    for (const auto kind : {ExternalGPS, NmeaGPS, InternalGPS, Simulated}) {
        auto& watched = _automaticSources[static_cast<size_t>(kind)];
        QPointer<QGeoPositionInfoSource> source = _sourceFor(kind);
        QPointer<GPSSourceHealth> health = kind == ExternalGPS ? _receiverHealth.data()
                                           : kind == NmeaGPS   ? _nmeaHealth.data()
                                                               : nullptr;
        if (source && !health) {
            if (!watched.fallback) {
                watched.fallback = std::make_unique<GPSSourceHealth>(this);
            }
            health = watched.fallback.get();
        }
        if (watched.source == source && watched.health == health) {
            continue;
        }
        for (const auto& connection : watched.connections) {
            QObject::disconnect(connection);
        }
        watched.connections.clear();
        if (watched.source && watched.source != source) {
            watched.source->stopUpdates();
        }
        if (!guard) {
            return;
        }
        if (watched.fallback) {
            watched.fallback->reset();
        }
        if (!guard) {
            return;
        }
        watched.source = source;
        watched.health = health;
        if (!source || !health) {
            continue;
        }
        watched.connections.append(
            connect(health, &GPSSourceHealth::positionChanged, this, &QGCPositionManager::_selectPositionSource));
        watched.connections.append(
            connect(source, &QObject::destroyed, this, &QGCPositionManager::_selectPositionSource));
        watched.connections.append(
            connect(health, &QObject::destroyed, this, &QGCPositionManager::_selectPositionSource));
        if (health == watched.fallback.get()) {
            watched.connections.append(connect(
                source, &QGeoPositionInfoSource::positionUpdated, this,
                [this, source, health, kind](const QGeoPositionInfo& update) {
                    if (_sourceMode != SourceMode::Automatic || !source || !health || _sourceFor(kind) != source) {
                        return;
                    }
                    if (kind == InternalGPS) {
                        _platformStatus = SourceStatus::WaitingForFix;
                    }
                    GPSObservation observation;
                    observation.position = update;
                    observation.receivedAt = QDateTime::currentDateTimeUtc();
                    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
                    observation.sourceId =
                        kind == InternalGPS ? QStringLiteral("Platform") : QStringLiteral("External GPS");
                    health->updateObservation(observation);
                }));
            watched.connections.append(connect(source, &QGeoPositionInfoSource::errorOccurred, this,
                                               [this, source, health, kind](QGeoPositionInfoSource::Error error) {
                                                   if (_sourceMode != SourceMode::Automatic || !source || !health ||
                                                       _sourceFor(kind) != source) {
                                                       return;
                                                   }
                                                   if (kind == InternalGPS) {
                                                       if (error == QGeoPositionInfoSource::AccessError) {
                                                           _platformStatus = SourceStatus::PermissionDenied;
                                                       } else if (error == QGeoPositionInfoSource::ClosedError ||
                                                                  error == QGeoPositionInfoSource::UnknownSourceError) {
                                                           _platformStatus = SourceStatus::BackendUnavailable;
                                                       }
                                                   }
                                                   if (error != QGeoPositionInfoSource::NoError) {
                                                       health->invalidatePosition();
                                                   }
                                               }));
        }
        source->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        if (!guard || !source || _sourceMode != SourceMode::Automatic) {
            return;
        }
        source->setUpdateInterval(kind == InternalGPS ? source->minimumUpdateInterval() : 0);
        if (!guard || !source || _sourceMode != SourceMode::Automatic) {
            return;
        }
        source->startUpdates();
        if (!guard) {
            return;
        }
    }
}

void QGCPositionManager::_stopAutomaticSources()
{
    _monitoringAutomatic = false;
    const QPointer<QGCPositionManager> guard(this);
    for (auto& watched : _automaticSources) {
        for (const auto& connection : watched.connections) {
            QObject::disconnect(connection);
        }
        watched.connections.clear();
        if (watched.source) {
            watched.source->stopUpdates();
        }
        if (!guard) {
            return;
        }
        watched.source = nullptr;
        watched.health = nullptr;
    }
}

QString QGCPositionManager::selectedSourceName() const
{
    switch (_selectedSource) {
        case SelectedSource::None:
            return tr("None");
        case SelectedSource::Receiver:
            return tr("Configured receiver");
        case SelectedSource::Nmea:
            return tr("NMEA");
        case SelectedSource::Internal:
            return _usingPluginSource ? tr("Plugin positioning") : tr("Internal positioning");
        case SelectedSource::Simulated:
            return tr("Simulated positioning");
    }
    return {};
}

QString QGCPositionManager::sourceStatusText() const
{
    switch (_sourceStatus) {
        case SourceStatus::NoSource:
            return tr("Selected position source is not connected");
        case SourceStatus::PermissionRequired:
            return tr("Waiting for location permission");
        case SourceStatus::PermissionDenied:
            return tr("Location permission denied");
        case SourceStatus::BackendUnavailable:
            return tr("No internal positioning backend is available");
        case SourceStatus::WaitingForFix:
            return tr("Waiting for a position fix");
        case SourceStatus::Active:
            return tr("Position is usable");
        case SourceStatus::Stale:
            return tr("Position data is stale");
        case SourceStatus::InvalidFix:
            return tr("Position fix does not meet accuracy requirements");
    }
    return {};
}

void QGCPositionManager::_updateSelectionStatus()
{
    const auto selected = !_currentSource                      ? SelectedSource::None
                          : _currentSource == _receiverSource  ? SelectedSource::Receiver
                          : _currentSource == _nmeaSource      ? SelectedSource::Nmea
                          : _currentSource == _simulatedSource ? SelectedSource::Simulated
                                                               : SelectedSource::Internal;
    SourceStatus status = SourceStatus::NoSource;
    if (_currentHealth) {
        switch (_currentHealth->state()) {
            case GPSSourceHealth::NoData:
                status = SourceStatus::WaitingForFix;
                break;
            case GPSSourceHealth::Usable:
                status = _currentHealth->acceptedObservation(GPSObservation::PositionUse::GroundStation)
                             ? SourceStatus::Active
                             : SourceStatus::Stale;
                break;
            case GPSSourceHealth::Stale:
                status = SourceStatus::Stale;
                break;
            case GPSSourceHealth::Invalid:
                status = SourceStatus::InvalidFix;
                break;
        }
    } else if (_sourceMode != SourceMode::ReceiverOnly && _sourceMode != SourceMode::NmeaOnly) {
        status = _platformStatus;
    }
    if (selected == SelectedSource::Internal &&
        (_platformStatus == SourceStatus::PermissionDenied || _platformStatus == SourceStatus::BackendUnavailable)) {
        status = _platformStatus;
    }
    QString reason;
    switch (_sourceMode) {
        case SourceMode::LegacyPriority:
            reason = tr("Receiver, then NMEA, then internal positioning; no health-based switching");
            break;
        case SourceMode::ReceiverOnly:
            reason = tr("Pinned to the configured receiver");
            break;
        case SourceMode::NmeaOnly:
            reason = tr("Pinned to the NMEA source");
            break;
        case SourceMode::InternalOnly:
            reason = tr("Pinned to internal positioning");
            break;
        case SourceMode::Automatic:
            reason = _recoveryCandidate ? tr("Keeping the current source while the preferred source recovers")
                     : status == SourceStatus::Active ? tr("Using the highest-priority healthy source")
                                                      : tr("Waiting for a healthy position source");
            break;
    }
    if (_selectedSource != selected || _sourceStatus != status || _selectionReason != reason) {
        _selectedSource = selected;
        _sourceStatus = status;
        _selectionReason = reason;
        emit selectionChanged();
    }
}

void QGCPositionManager::setNmeaPositionSource(QGeoPositionInfoSource* source, GPSSourceHealth* health)
{
    if (QThread::currentThread() != thread() || (source && source->thread() != thread()) ||
        (health && health->thread() != thread())) {
        qCWarning(QGCPositionManagerLog) << "Position source changes require matching thread affinity";
        return;
    }
    if (_nmeaSource == source && _nmeaHealth == health) {
        return;
    }
    ++_positionRevision;
    QObject::disconnect(_nmeaDestroyedConnection);
    _nmeaSource = source;
    _nmeaHealth = source ? health : nullptr;
    if (source) {
        _nmeaDestroyedConnection = connect(source, &QObject::destroyed, this, [this, source]() {
            const QPointer<QGCPositionManager> guard(this);
            ++_positionRevision;
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
    // Default and pinned policies wait for a new observation when selecting a standby source.
    return _currentHealth && _gcsPosition.isValid() ? _currentHealth->acceptedObservation(use) : std::nullopt;
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo& update)
{
    if (!_isExternalSource()) {
        _platformStatus = SourceStatus::WaitingForFix;
    }
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
    const QPointer<QGCPositionManager> guard(this);
    const quint64 generation = _sourceGeneration;
    _updateSelectionStatus();
    if (!guard || generation != _sourceGeneration) {
        return;
    }
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
    if (_sourceMode == SourceMode::Automatic) {
        nextHealth = nextSource ? _automaticHealthFor(source) : nullptr;
    } else if (nextSource && !nextHealth) {
        nextHealth = &_externalHealth;
    }
    if (!_forceSourceRefresh && _currentSource == nextSource && _currentHealth == nextHealth) {
        return;
    }
    qCDebug(QGCPositionManagerLog) << "Ground-station position source changed"
                                   << "source:" << (nextSource ? sourceName : "none")
                                   << "previous:" << _currentSource
                                   << "selected:" << nextSource;
    _forceSourceRefresh = false;
    const quint64 generation = ++_sourceGeneration;
    QObject::disconnect(_healthConnection);
    QObject::disconnect(_healthDestroyedConnection);
    _externalHealth.reset();
    if (!guard || generation != _sourceGeneration || _selectionPending) {
        return;
    }
    QObject::disconnect(_positionUpdateConnection);
    QObject::disconnect(_positionErrorConnection);
    if (_currentSource && _sourceMode != SourceMode::Automatic) {
        _currentSource->stopUpdates();
    }
    if (!guard || generation != _sourceGeneration || _selectionPending) {
        return;
    }
    _currentSource = nextSource;
    _currentHealth = nextHealth;
    _clearPosition();
    if (!guard || generation != _sourceGeneration || _selectionPending) {
        return;
    }
    emit sourceHealthChanged();
    if (!guard || generation != _sourceGeneration || _selectionPending) {
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
    if (_currentSource && _sourceMode == SourceMode::Automatic) {
        _externalPositionChanged();
        return;
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
                            if (_currentSource == _defaultSource) {
                                if (error == QGeoPositionInfoSource::AccessError) {
                                    _platformStatus = SourceStatus::PermissionDenied;
                                } else if (error == QGeoPositionInfoSource::ClosedError ||
                                           error == QGeoPositionInfoSource::UnknownSourceError) {
                                    _platformStatus = SourceStatus::BackendUnavailable;
                                }
                            }
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
