#include "GPSPositionService.h"

#include <utility>

#include <QtCore/QThread>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"
QGC_LOGGING_CATEGORY(GPSPositionServiceLog, "GPS.PositionManager.GPSPositionService")

namespace {
using SelectedSource = GPSPositionService::SelectedSource;
constexpr std::array SOURCE_KINDS = {SelectedSource::Receiver, SelectedSource::Nmea, SelectedSource::Internal,
                                     SelectedSource::Simulated};
}  // namespace

GPSPositionService::GPSPositionService(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent), _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this)), _recoveryTask(_scheduler, this)
{
    qCDebug(GPSPositionServiceLog) << this;
    if (_scheduler->thread() != thread()) {
        qCWarning(GPSPositionServiceLog) << "Scheduler must share the positioning service thread";
        _scheduler = nullptr;
        return;
    }
    for (const auto kind : SOURCE_KINDS) {
        auto& adapter = _binding(kind).adapter;
        adapter = std::make_unique<GPSPositionSourceAdapter>(this, _scheduler);
        connect(adapter.get(), &GPSPositionSourceAdapter::bindingChanged, this, [this, kind]() {
            _pendingObservations[static_cast<size_t>(kind)] = false;
            if (_selectedKind == kind) {
                ++_sourceGeneration;
            }
            _refreshSourceBindings();
            _selectPositionSource();
        });
        connect(adapter.get(), &GPSPositionSourceAdapter::observationChanged, this,
                [this, kind]() { _sourceObservationChanged(kind); });
        connect(adapter.get(), &GPSPositionSourceAdapter::backendError, this,
                [this, kind](QGeoPositionInfoSource::Error error) {
                    if (kind == SelectedSource::Internal) {
                        if (error == QGeoPositionInfoSource::AccessError) {
                            _platformStatus = SourceStatus::PermissionDenied;
                        } else if (error == QGeoPositionInfoSource::ClosedError ||
                                   error == QGeoPositionInfoSource::UnknownSourceError) {
                            _platformStatus = SourceStatus::BackendUnavailable;
                        } else if (error == QGeoPositionInfoSource::NoError) {
                            _platformStatus = SourceStatus::WaitingForFix;
                        }
                    }
                    if (_selectedKind == kind) {
                        _positionError(error);
                    }
                    _updateSelectionStatus();
                });
    }
    connect(_scheduler, &QObject::destroyed, this, [this]() {
        _scheduler = nullptr;
        _recoveryTask.cancel();
        _selector.reset();
        _selectPositionSource();
    });
}

GPSPositionService::~GPSPositionService()
{
    qCDebug(GPSPositionServiceLog) << this;
    if (_scheduler) {
        _scheduler->disconnect(this);
    }
    _recoveryTask.cancel();
    for (auto& binding : _bindings) {
        if (binding.adapter) {
            binding.adapter->disconnect(this);
        }
    }
}

void GPSPositionService::setInternalPositionSource(QGeoPositionInfoSource* source, SourceStatus status, bool custom)
{
    if (!_canBindSource(SelectedSource::Internal, source)) {
        return;
    }
    _usingPluginSource = custom;
    _platformStatus = status;
    _setBinding(SelectedSource::Internal, source);
}

void GPSPositionService::setInternalPositionStatus(SourceStatus status)
{
    if (QThread::currentThread() != thread()) {
        return;
    }
    _platformStatus = status;
    _selectPositionSource();
}

void GPSPositionService::setSimulatedPositionSource(QGeoPositionInfoSource* source)
{
    if (_canBindSource(SelectedSource::Simulated, source)) {
        _setBinding(SelectedSource::Simulated, source);
    }
}

GPSPositionSourceRegistration GPSPositionService::registerPositionSource(SelectedSource kind, QObject* source,
                                                                         GPSSourceHealth* health, quint64 sessionId)
{
    if ((kind != SelectedSource::Receiver && kind != SelectedSource::Nmea) || !source ||
        (!health && !qobject_cast<QGeoPositionInfoSource*>(source)) || !_canBindSource(kind, source, health)) {
        return {};
    }
    const quint64 token = _binding(kind).token + 1;
    GPSPositionSourceRegistration registration(this, static_cast<int>(kind), token);
    _setBinding(kind, source, health, sessionId);
    return registration;
}

void GPSPositionService::_retireRegistration(int kindValue, quint64 token)
{
    if (QThread::currentThread() != thread()) {
        return;
    }
    const auto kind = static_cast<SelectedSource>(kindValue);
    if ((kind != SelectedSource::Receiver && kind != SelectedSource::Nmea) || _binding(kind).token != token) {
        return;
    }
    _setBinding(kind, nullptr);
}

bool GPSPositionService::_canBindSource(SelectedSource kind, QObject* source, GPSSourceHealth* health) const
{
    if (!_scheduler || QThread::currentThread() != thread() || (source && source->thread() != thread()) ||
        (health && health->thread() != thread())) {
        return false;
    }
    if (qobject_cast<QGeoPositionInfoSource*>(source)) {
        for (const auto other : SOURCE_KINDS) {
            const auto* adapter = _binding(other).adapter.get();
            if (other != kind && adapter && adapter->producer() == source && (!health || !adapter->providedHealth())) {
                return false;
            }
        }
    }
    return true;
}

void GPSPositionService::_setBinding(SelectedSource kind, QObject* source, GPSSourceHealth* health, quint64 sessionId)
{
    if ((source && !_scheduler) || QThread::currentThread() != thread() || (source && source->thread() != thread()) ||
        (health && health->thread() != thread())) {
        qCWarning(GPSPositionServiceLog) << "Position source changes require matching thread affinity";
        return;
    }
    auto& binding = _binding(kind);
    ++binding.token;
    if (!binding.adapter) {
        return;
    }
    const QString identity = kind == SelectedSource::Internal
                                 ? (_usingPluginSource ? QStringLiteral("Plugin") : QStringLiteral("Platform"))
                             : kind == SelectedSource::Simulated ? QStringLiteral("Simulated")
                                                                 : QStringLiteral("External GPS");
    const QPointer<GPSPositionService> guard(this);
    binding.adapter->configure(source, source ? health : nullptr, identity,
                               kind == SelectedSource::Internal || kind == SelectedSource::Simulated, sessionId);
    if (guard) {
        _selectPositionSource();
    }
}

void GPSPositionService::setSourceMode(SourceMode mode)
{
    if (QThread::currentThread() != thread()) {
        qCWarning(GPSPositionServiceLog) << "Position source changes require matching thread affinity";
        return;
    }
    if (mode < SourceMode::LegacyPriority || mode > SourceMode::InternalOnly || _sourceMode == mode) {
        return;
    }
    _sourceMode = mode;
    _pendingObservations.fill(false);
    _forceSourceRefresh = true;
    ++_sourceGeneration;
    _selector.reset();
    _recoveryTask.cancel();
    const QPointer<GPSPositionService> guard(this);
    _selectPositionSource();
    if (guard) {
        emit sourceModeChanged();
    }
}

QObject* GPSPositionService::_sourceFor(SelectedSource source) const
{
    return _scheduler && _binding(source).adapter ? _binding(source).adapter->source() : nullptr;
}

void GPSPositionService::_selectPositionSource()
{
    if (!_binding(SelectedSource::Internal).adapter) {
        return;
    }
    _selectionPending = true;
    if (_selectingSource) {
        return;
    }
    _selectingSource = true;
    const QPointer<GPSPositionService> guard(this);
    do {
        _selectionPending = false;
        _setPositionSource(_choosePositionSource());
        if (!guard) {
            return;
        }
        if (_selectionPending) {
            continue;
        }
        const bool publishSelection = std::exchange(_selectionPublicationPending, false);
        const bool publishObservation = _pendingObservations[static_cast<size_t>(_selectedKind)];
        _pendingObservations.fill(false);
        // Standby deadlines can run before the selected source's deadline.
        const bool publishedFixRejected = _gcsPosition.isValid() && !_acceptedSourceObservation(_selectedKind);
        if (publishObservation || publishedFixRejected || (publishSelection && _sourceMode == SourceMode::Automatic)) {
            _externalPositionChanged();
            if (!guard) {
                return;
            }
            if (_selectionPending) {
                continue;
            }
        }
        _updateSelectionStatus();
        if (!guard) {
            return;
        }
    } while (_selectionPending);
    _selectingSource = false;
}

GPSPositionService::SelectedSource GPSPositionService::_choosePositionSource()
{
    const SelectedSource internal =
        _sourceFor(SelectedSource::Internal)
            ? SelectedSource::Internal
            : (_sourceFor(SelectedSource::Simulated) ? SelectedSource::Simulated : SelectedSource::Internal);
    switch (_sourceMode) {
        case SourceMode::ReceiverOnly:
            return SelectedSource::Receiver;
        case SourceMode::NmeaOnly:
            return SelectedSource::Nmea;
        case SourceMode::InternalOnly:
            return internal;
        case SourceMode::LegacyPriority:
            return _sourceFor(SelectedSource::Receiver)
                       ? SelectedSource::Receiver
                       : (_sourceFor(SelectedSource::Nmea) ? SelectedSource::Nmea : internal);
        case SourceMode::Automatic:
            break;
    }
    const std::array priorityKinds = {SelectedSource::Receiver, SelectedSource::Nmea, internal};
    std::array<GPSPositionSourceSelector::Candidate, 3> priority{};
    for (size_t index = 0; index < priority.size(); ++index) {
        const auto kind = priorityKinds[index];
        auto* health = _binding(kind).adapter->health();
        priority[index] = {static_cast<int>(kind), _sourceFor(kind) != nullptr,
                           health && _acceptedSourceObservation(kind).has_value()};
    }
    const qint64 nowMs = _scheduler ? _scheduler->nowMs() : 0;
    const auto selected = static_cast<SelectedSource>(_selector.select(
        priority, _currentSource ? static_cast<int>(_selectedKind) : -1, nowMs, RECOVERY_DELAY.count()));
    if (_selector.recovering() && _scheduler) {
        _recoveryTask.schedule(std::chrono::milliseconds(_selector.remainingRecoveryMs(nowMs, RECOVERY_DELAY.count())),
                               [this]() { _selectPositionSource(); });
    } else {
        _recoveryTask.cancel();
    }
    return selected;
}

void GPSPositionService::_refreshSourceBindings()
{
    for (const auto kind : SOURCE_KINDS) {
        auto* adapter = _binding(kind).adapter.get();
        adapter->setRawBindingAllowed(_canBindSource(kind, adapter->producer(), adapter->providedHealth()));
    }
    for (const auto kind : SOURCE_KINDS) {
        auto* adapter = _binding(kind).adapter.get();
        bool observe = adapter->health() != nullptr;
        for (const auto other : SOURCE_KINDS) {
            if (other == kind) {
                break;
            }
            if (_binding(other).adapter->health() == adapter->health()) {
                observe = false;
                break;
            }
        }
        // Shared health reports once, even when it serves several roles.
        adapter->observeHealth(observe);
    }
}

void GPSPositionService::_sourceObservationChanged(SelectedSource kind)
{
    auto* health = _binding(kind).adapter->health();
    if (!health) {
        return;
    }
    if (_selectingSource && health == _currentHealth) {
        ++_positionRevision;
    }
    for (const auto other : SOURCE_KINDS) {
        if (_binding(other).adapter->health() == health) {
            _pendingObservations[static_cast<size_t>(other)] = true;
        }
    }
    _selectPositionSource();
}

void GPSPositionService::_updateSourceActivity()
{
    const QPointer<GPSPositionService> guard(this);
    for (const auto kind : SOURCE_KINDS) {
        _binding(kind).adapter->setActive(_scheduler &&
                                          (_sourceMode == SourceMode::Automatic || _selectedKind == kind));
        if (!guard || _selectionPending) {
            return;
        }
    }
}

QString GPSPositionService::selectedSourceName() const
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

QString GPSPositionService::sourceStatusText() const
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

void GPSPositionService::_updateSelectionStatus()
{
    const auto selected = _currentSource ? _selectedKind : SelectedSource::None;
    SourceStatus status = SourceStatus::NoSource;
    if (_currentHealth) {
        switch (_currentHealth->state()) {
            case GPSSourceHealth::State::NoData:
                status = SourceStatus::WaitingForFix;
                break;
            case GPSSourceHealth::State::Usable:
                status = !_acceptedSourceObservation(_selectedKind) ? SourceStatus::Stale
                         : _gcsPosition.isValid()                   ? SourceStatus::Active
                                                                    : SourceStatus::WaitingForFix;
                break;
            case GPSSourceHealth::State::Stale:
                status = SourceStatus::Stale;
                break;
            case GPSSourceHealth::State::Invalid:
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
    const bool changed = _selectedSource != selected || _sourceStatus != status || _selectionReason != reason;
    _selectedSource = selected;
    _sourceStatus = status;
    _selectionReason = reason;
    const auto name = selectedSourceName();
    if (changed || _selectionName != name) {
        _selectionName = name;
        emit selectionChanged();
    }
}

std::optional<GPSObservation> GPSPositionService::acceptedObservation(
    GPSObservation::PositionUse use, std::optional<std::chrono::milliseconds> maximumAge) const
{
    return _currentHealth && _selectedObservationAuthorized ? _acceptedSourceObservation(_selectedKind, use, maximumAge)
                                                            : std::nullopt;
}

std::optional<GPSObservation> GPSPositionService::_acceptedSourceObservation(
    SelectedSource source, GPSObservation::PositionUse use, std::optional<std::chrono::milliseconds> maximumAge) const
{
    if (!_sourceFor(source)) {
        return std::nullopt;
    }
    auto* health = _binding(source).adapter->health();
    auto observation = health ? health->acceptedObservation(use, maximumAge) : std::nullopt;
    const quint64 session = _binding(source).adapter->sessionId();
    if (observation && session != 0 && observation->sessionId != session) {
        return std::nullopt;
    }
    return observation;
}

void GPSPositionService::_externalPositionChanged()
{
    if (!_currentHealth) {
        return;
    }
    // Pinned selections need a new observation, not a timeout change.
    _selectedObservationAuthorized =
        _sourceMode == SourceMode::Automatic || _currentHealth->observationRevision() != _selectionObservationRevision;
    const auto accepted = acceptedObservation();
    if (!accepted) {
        if (_currentHealth->state() != GPSSourceHealth::State::NoData &&
            (_gcsPositioningError == QGeoPositionInfoSource::NoError ||
             _gcsPositioningError == QGeoPositionInfoSource::UpdateTimeoutError)) {
            _positionError(QGeoPositionInfoSource::UpdateTimeoutError);
        }
        if (_gcsPosition.isValid() || _currentHealth->state() != GPSSourceHealth::State::Stale) {
            _clearPosition();
        }
        return;
    }
    _gcsPositioningError = QGeoPositionInfoSource::NoError;
    _publishPosition(accepted);
}

void GPSPositionService::_publishPosition(const std::optional<GPSObservation>& observation)
{
    const QPointer<GPSPositionService> guard(this);
    const quint64 generation = _sourceGeneration;
    const quint64 revision = ++_positionRevision;
    if (observation) {
        // Legacy consumers still need the unfiltered fix.
        _geoPositionInfo = _currentHealth->observation().position;
        _gcsPosition = observation->position.coordinate();
        _gcsPositionTimestamp = observation->receivedAt;
        _gcsHeading = observation->heading();
        _gcsPositionHorizontalAccuracy = observation->position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    } else {
        _geoPositionInfo = {};
        _gcsPosition = {};
        _gcsPositionTimestamp = {};
        _gcsHeading = qQNaN();
        _gcsPositionHorizontalAccuracy = qInf();
    }
    _updateSelectionStatus();
    if (!guard || generation != _sourceGeneration || revision != _positionRevision) {
        return;
    }
    if (_gcsPositionHorizontalAccuracy != _notifiedHorizontalAccuracy) {
        _notifiedHorizontalAccuracy = _gcsPositionHorizontalAccuracy;
        emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
    }
    if (!guard || generation != _sourceGeneration || revision != _positionRevision) {
        return;
    }
    if (_gcsHeading != _notifiedHeading && !(qIsNaN(_gcsHeading) && qIsNaN(_notifiedHeading))) {
        _notifiedHeading = _gcsHeading;
        emit gcsHeadingChanged(_gcsHeading);
    }
    if (!guard || generation != _sourceGeneration || revision != _positionRevision) {
        return;
    }
    if (_gcsPosition != _notifiedPosition) {
        _notifiedPosition = _gcsPosition;
        emit gcsPositionChanged(_gcsPosition);
    }
    if (guard && generation == _sourceGeneration && revision == _positionRevision) {
        emit positionInfoUpdated(_geoPositionInfo);
    }
}

void GPSPositionService::_positionError(QGeoPositionInfoSource::Error gcsPositioningError)
{
    if (_gcsPositioningError == gcsPositioningError) {
        return;
    }
    _gcsPositioningError = gcsPositioningError;
    if (gcsPositioningError != QGeoPositionInfoSource::NoError) {
        qCDebug(GPSPositionServiceLog) << "Positioning error:" << gcsPositioningError;
    }
}

void GPSPositionService::_clearPosition()
{
    _publishPosition(std::nullopt);
}

void GPSPositionService::_setPositionSource(SelectedSource source)
{
    const QPointer<GPSPositionService> guard(this);
    const QPointer<QObject> nextSource = _sourceFor(source);
    QPointer<GPSSourceHealth> nextHealth = nextSource ? _binding(source).adapter->health() : nullptr;
    if (!_forceSourceRefresh && _selectedKind == source && _currentSource == nextSource &&
        _currentHealth == nextHealth && _selectedBindingRevision == _binding(source).adapter->bindingRevision()) {
        _updateSourceActivity();
        return;
    }
    qCDebug(GPSPositionServiceLog) << "Ground-station position source changed"
                                   << "source:" << static_cast<int>(source) << "previous:" << _currentSource
                                   << "selected:" << nextSource;
    _forceSourceRefresh = false;
    const quint64 generation = ++_sourceGeneration;
    _currentSource = nextSource;
    _currentHealth = nextHealth;
    _selectedKind = source;
    _selectedBindingRevision = _binding(source).adapter->bindingRevision();
    _selectionObservationRevision = _currentHealth ? _currentHealth->observationRevision() : 0;
    _selectedObservationAuthorized = false;
    if (_sourceMode != SourceMode::Automatic) {
        _pendingObservations[static_cast<size_t>(source)] = false;
    }
    _selectionPublicationPending = true;
    _updateInterval = _binding(source).adapter->updateInterval();
    _clearPosition();
    if (!guard || generation != _sourceGeneration) {
        return;
    }
    emit sourceHealthChanged();
    if (!guard || generation != _sourceGeneration) {
        return;
    }
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    _updateSourceActivity();
}
