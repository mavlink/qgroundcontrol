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
{
    qCDebug(QGCPositionManagerLog) << this;
    for (auto& adapter : _sourceAdapters) {
        adapter = std::make_unique<GPSPositionSourceAdapter>(this);
        connect(adapter.get(), &GPSPositionSourceAdapter::bindingChanged, this,
                &QGCPositionManager::_selectPositionSource);
        connect(adapter.get(), &GPSPositionSourceAdapter::observationChanged, this, [this]() {
            if (_sourceMode == SourceMode::Automatic) {
                _selectPositionSource();
            }
        });
    }
    connect(_sourceAdapters[InternalGPS].get(), &GPSPositionSourceAdapter::backendError, this,
            [this](QGeoPositionInfoSource::Error error) {
                if (error == QGeoPositionInfoSource::AccessError) {
                    _platformStatus = SourceStatus::PermissionDenied;
                } else if (error == QGeoPositionInfoSource::ClosedError ||
                           error == QGeoPositionInfoSource::UnknownSourceError) {
                    _platformStatus = SourceStatus::BackendUnavailable;
                } else if (error == QGeoPositionInfoSource::NoError) {
                    _platformStatus = SourceStatus::WaitingForFix;
                }
            });
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
    for (auto& adapter : _sourceAdapters) {
        adapter->disconnect(this);
        adapter->fallbackHealth().disconnect(this);
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
        _selectPositionSource();
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

std::unique_ptr<GPSPositionSourceRegistration> QGCPositionManager::registerPositionSource(
    SelectedSource kind, QGeoPositionInfoSource* source, GPSSourceHealth* health)
{
    if ((kind != SelectedSource::Receiver && kind != SelectedSource::Nmea) || !source ||
        QThread::currentThread() != thread() || source->thread() != thread() ||
        (health && health->thread() != thread())) {
        return {};
    }
    const auto index = static_cast<size_t>(kind);
    const quint64 token = _registrationTokens[index] + 1;
    auto registration = std::unique_ptr<GPSPositionSourceRegistration>(
        new GPSPositionSourceRegistration(this, static_cast<int>(kind), token));
    if (kind == SelectedSource::Receiver) {
        setReceiverPositionSource(source, health);
    } else {
        setNmeaPositionSource(source, health);
    }
    return registration;
}

void QGCPositionManager::_retireRegistration(int kind, quint64 token)
{
    if (kind < 0 || kind >= static_cast<int>(_registrationTokens.size()) ||
        _registrationTokens[static_cast<size_t>(kind)] != token) {
        return;
    }
    ++_registrationTokens[static_cast<size_t>(kind)];
    if (kind == static_cast<int>(SelectedSource::Receiver)) {
        setReceiverPositionSource(nullptr);
    } else if (kind == static_cast<int>(SelectedSource::Nmea)) {
        setNmeaPositionSource(nullptr);
    }
}

void QGCPositionManager::setReceiverPositionSource(QGeoPositionInfoSource* source, GPSSourceHealth* health)
{
    if (QThread::currentThread() != thread() || (source && source->thread() != thread()) ||
        (health && health->thread() != thread())) {
        qCWarning(QGCPositionManagerLog) << "Position source changes require matching thread affinity";
        return;
    }
    ++_registrationTokens[static_cast<size_t>(SelectedSource::Receiver)];
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
    _selector.reset();
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
        _refreshSourceAdapters();
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
    const std::array priorityKinds = {ExternalGPS, NmeaGPS, internal};
    std::array<GPSPositionSourceSelector::Candidate, 3> priority{};
    for (size_t index = 0; index < priority.size(); ++index) {
        const auto kind = priorityKinds[index];
        auto* health = _sourceAdapters[kind]->health();
        priority[index] = {
            kind, _sourceFor(kind) != nullptr,
            health && health->acceptedObservation(GPSObservation::PositionUse::GroundStation).has_value()};
    }
    const qint64 nowMs = static_cast<qint64>(GPSObservation::monotonicNowUs() / 1000);
    const auto selected = static_cast<QGCPositionSource>(
        _selector.select(priority, _currentSource ? _selectedKind : -1, nowMs, _recoveryTimer.interval()));
    if (_selector.recovering()) {
        if (!_recoveryTimer.isActive()) {
            _recoveryTimer.start();
        }
    } else {
        _recoveryTimer.stop();
    }
    return selected;
}

void QGCPositionManager::_refreshSourceAdapters()
{
    const QPointer<QGCPositionManager> guard(this);
    for (const auto kind : {ExternalGPS, NmeaGPS, InternalGPS, Simulated}) {
        auto* supplied = kind == ExternalGPS ? _receiverHealth.data() : kind == NmeaGPS ? _nmeaHealth.data() : nullptr;
        const QString identity = kind == InternalGPS
                                     ? (_usingPluginSource ? QStringLiteral("Plugin") : QStringLiteral("Platform"))
                                 : kind == Simulated ? QStringLiteral("Simulated")
                                                     : QStringLiteral("External GPS");
        _sourceAdapters[kind]->configure(_sourceFor(kind), supplied, identity,
                                         kind == InternalGPS || kind == Simulated);
        if (!guard) {
            return;
        }
    }
}

void QGCPositionManager::_updateSourceActivity()
{
    const QPointer<QGCPositionManager> guard(this);
    for (const auto kind : {ExternalGPS, NmeaGPS, InternalGPS, Simulated}) {
        _sourceAdapters[kind]->setActive(_sourceMode == SourceMode::Automatic || _currentSource == _sourceFor(kind));
        if (!guard || _selectionPending) {
            return;
        }
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
            reason = _selector.recovering() ? tr("Keeping the current source while the preferred source recovers")
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
    ++_registrationTokens[static_cast<size_t>(SelectedSource::Nmea)];
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
    _sourceAdapters[_selectedKind]->updatePosition(update);
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
    QPointer<GPSSourceHealth> nextHealth = nextSource ? _sourceAdapters[source]->health() : nullptr;
    if (!_forceSourceRefresh && _currentSource == nextSource && _currentHealth == nextHealth) {
        _updateSourceActivity();
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
    _currentSource = nextSource;
    _currentHealth = nextHealth;
    _selectedKind = source;
    _updateInterval = _sourceAdapters[source]->updateInterval();
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
    _updateSourceActivity();
    if (!guard || generation != _sourceGeneration || _selectionPending) {
        return;
    }
    if (_currentSource && _sourceMode == SourceMode::Automatic) {
        _externalPositionChanged();
    }
}
