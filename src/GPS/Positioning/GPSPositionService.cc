#include "GPSPositionService.h"

#include <algorithm>
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

GPSPositionSourceRegistration::GPSPositionSourceRegistration() = default;

GPSPositionSourceRegistration::GPSPositionSourceRegistration(GPSPositionSourceRegistration&& other) noexcept
{
    _swap(other);
}

GPSPositionSourceRegistration& GPSPositionSourceRegistration::operator=(GPSPositionSourceRegistration&& other) noexcept
{
    if (this != &other) {
        GPSPositionSourceRegistration retired(std::move(*this));
        _swap(other);
    }
    return *this;
}

void GPSPositionSourceRegistration::_swap(GPSPositionSourceRegistration& other) noexcept
{
    _manager.swap(other._manager);
    std::swap(_kind, other._kind);
    std::swap(_token, other._token);
}

GPSPositionSourceRegistration::GPSPositionSourceRegistration(GPSPositionService* manager, int kind, quint64 token)
    : _manager(manager)
    , _kind(kind)
    , _token(token)
{}

GPSPositionSourceRegistration::~GPSPositionSourceRegistration()
{
    reset();
}

void GPSPositionSourceRegistration::reset()
{
    const auto manager = std::exchange(_manager, {});
    const int kind = std::exchange(_kind, 0);
    const quint64 token = std::exchange(_token, 0);
    if (manager && token) {
        QMetaObject::invokeMethod(manager.data(), &GPSPositionService::_retireRegistration, Qt::AutoConnection, kind,
                                  token);
    }
}

GPSPositionService::SourceBinding::SourceBinding(GPSPositionService* service, SelectedSource sourceKind)
    : owner(service)
    , kind(sourceKind)
    , fallbackHealth(service, service->_scheduler)
{}

GPSPositionService::SourceBinding::~SourceBinding()
{
    disconnectSource();
}

QObject* GPSPositionService::SourceBinding::source() const
{
    return providedHealth || rawBindingAllowed ? producer.data() : nullptr;
}

GPSSourceHealth* GPSPositionService::SourceBinding::health()
{
    return source() ? (providedHealth ? providedHealth.data() : &fallbackHealth) : nullptr;
}

void GPSPositionService::SourceBinding::disconnectNotifications()
{
    observeHealth(false);
    for (const auto& connection : connections) {
        QObject::disconnect(connection);
    }
    connections.clear();
}

void GPSPositionService::SourceBinding::disconnectSource()
{
    disconnectNotifications();
    active = false;
    const bool stop = std::exchange(updatesStarted, false);
    if (rawSource && stop) {
        rawSource->stopUpdates();
    }
}

void GPSPositionService::SourceBinding::configure(QObject* nextProducer, GPSSourceHealth* nextHealth,
                                                  const QString& nextIdentity, bool nextPlatform, quint64 nextSession)
{
    if (producer == nextProducer && providedHealth == nextHealth && identity == nextIdentity &&
        platform == nextPlatform && sessionId == nextSession) {
        return;
    }
    const QPointer<GPSPositionService> bindingGuard(owner);
    const quint64 revision = ++generation;
    const QPointer<QObject> producerGuard(nextProducer);
    const QPointer<GPSSourceHealth> healthGuard(nextHealth);
    disconnectSource();
    if (!bindingGuard || revision != generation) {
        return;
    }
    producer = producerGuard;
    rawSource = qobject_cast<QGeoPositionInfoSource*>(producerGuard.data());
    providedHealth = healthGuard;
    identity = nextIdentity;
    platform = nextPlatform;
    sessionId = nextSession;
    fallbackHealth.reset();
    if (!bindingGuard || revision != generation) {
        return;
    }
    if (producer) {
        connections.append(QObject::connect(producer, &QObject::destroyed, owner, [this]() {
            producer = nullptr;
            rawSource = nullptr;
            disconnectSource();
            owner->_bindingChanged(kind);
        }));
        if (providedHealth) {
            connections.append(QObject::connect(providedHealth, &QObject::destroyed, owner, [this]() {
                observeHealth(false);
                providedHealth = nullptr;
                active = false;
                owner->_bindingChanged(kind);
            }));
        }
        observeHealth(true);
        if (rawSource) {
            connections.append(QObject::connect(rawSource, &QGeoPositionInfoSource::positionUpdated, owner,
                                                [this, revision](const QGeoPositionInfo& position) {
                                                    if (generation == revision && active && !providedHealth) {
                                                        updatePosition(position);
                                                    }
                                                }));
            connections.append(QObject::connect(rawSource, &QGeoPositionInfoSource::errorOccurred, owner,
                                                [this, revision](QGeoPositionInfoSource::Error error) {
                                                    if (generation != revision || !active || providedHealth) {
                                                        return;
                                                    }
                                                    const QPointer<GPSPositionService> errorGuard(owner);
                                                    const quint64 event = ++backendRevision;
                                                    owner->_backendError(kind, error);
                                                    if (errorGuard && generation == revision &&
                                                        event == backendRevision && active && rawSource &&
                                                        !providedHealth && error != QGeoPositionInfoSource::NoError &&
                                                        error != QGeoPositionInfoSource::UpdateTimeoutError) {
                                                        fallbackHealth.invalidatePosition();
                                                    }
                                                }));
        }
    }
    owner->_bindingChanged(kind);
}

void GPSPositionService::SourceBinding::observeHealth(bool observe)
{
    auto* observed = observe ? health() : nullptr;
    if (!observed) {
        QObject::disconnect(std::exchange(observationConnection, {}));
    } else if (!observationConnection) {
        observationConnection = QObject::connect(observed, &GPSSourceHealth::positionChanged, owner, [this]() {
            if (providedHealth || active) {
                owner->_sourceObservationChanged(kind);
            }
        });
    }
}

void GPSPositionService::SourceBinding::setActive(bool enabled)
{
    enabled = enabled && source();
    if (active == enabled || !producer) {
        return;
    }
    active = enabled;
    ++backendRevision;
    if (providedHealth || !rawSource) {
        return;
    }
    const QPointer<GPSPositionService> guard(owner);
    const quint64 revision = generation;
    if (!enabled) {
        if (std::exchange(updatesStarted, false)) {
            rawSource->stopUpdates();
        }
        if (guard && revision == generation && !active) {
            fallbackHealth.reset();
        }
        return;
    }
    rawSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
    if (!guard || revision != generation || !rawSource || !active) {
        return;
    }
#if defined(Q_OS_DARWIN) || defined(Q_OS_IOS)
    if (!platform) {
        rawSource->setUpdateInterval(0);
    }
#else
    rawSource->setUpdateInterval(updateInterval());
#endif
    if (guard && revision == generation && rawSource && active) {
        updatesStarted = true;
        rawSource->startUpdates();
    }
}

int GPSPositionService::SourceBinding::updateInterval() const
{
    return platform && rawSource ? rawSource->minimumUpdateInterval() : 0;
}

void GPSPositionService::SourceBinding::updatePosition(const QGeoPositionInfo& position)
{
    const QPointer<GPSPositionService> guard(owner);
    const quint64 revision = generation;
    const quint64 event = ++backendRevision;
    owner->_backendError(kind, QGeoPositionInfoSource::NoError);
    if (!guard || revision != generation || event != backendRevision || !active || !rawSource || providedHealth) {
        return;
    }
    GPSObservation observation;
    observation.position = position;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = owner->_scheduler->nowUs();
    observation.sourceId = identity;
    observation.sessionId = sessionId;
    fallbackHealth.updateObservation(observation);
}

GPSPositionService::GPSPositionService(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _recoveryTask(_scheduler, this)
{
    qCDebug(GPSPositionServiceLog) << this;
    for (const auto kind : SOURCE_KINDS) {
        _bindings[static_cast<size_t>(kind)] = std::make_unique<SourceBinding>(this, kind);
    }
}

GPSPositionService::~GPSPositionService()
{
    qCDebug(GPSPositionServiceLog) << this;
    _notifications.close();
    _recoveryTask.cancel();
    // Stopping one backend can destroy another while bindings themselves are being destroyed.
    for (const auto& binding : _bindings) {
        if (binding) {
            binding->disconnectNotifications();
        }
    }
}

void GPSPositionService::_bindingChanged(SelectedSource kind)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _binding(kind).pendingObservation = false;
    _refreshSourceBindings();
    _selectPositionSource();
}

void GPSPositionService::_backendError(SelectedSource kind, QGeoPositionInfoSource::Error error)
{
    const GPSNotificationQueue::Scope publish(_notifications);
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
}

void GPSPositionService::_clearPendingObservations()
{
    for (const auto& binding : _bindings) {
        if (binding) {
            binding->pendingObservation = false;
        }
    }
}

void GPSPositionService::setInternalPositionSource(QGeoPositionInfoSource* source, SourceStatus status, bool custom)
{
    if (!_canBindSource(SelectedSource::Internal, source)) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _usingPluginSource = custom;
    _platformStatus = status;
    _setBinding(SelectedSource::Internal, source);
}

void GPSPositionService::setInternalPositionStatus(SourceStatus status)
{
    if (QThread::currentThread() != thread()) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
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
    if (QThread::currentThread() != thread() || (source && source->thread() != thread()) ||
        (health && health->thread() != thread())) {
        return false;
    }
    if (qobject_cast<QGeoPositionInfoSource*>(source)) {
        for (const auto other : SOURCE_KINDS) {
            const auto& binding = _binding(other);
            if (other != kind && binding.producer == source && (!health || !binding.providedHealth)) {
                return false;
            }
        }
    }
    return true;
}

void GPSPositionService::_setBinding(SelectedSource kind, QObject* source, GPSSourceHealth* health, quint64 sessionId)
{
    if (QThread::currentThread() != thread() || (source && source->thread() != thread()) ||
        (health && health->thread() != thread())) {
        qCWarning(GPSPositionServiceLog) << "Position source changes require matching thread affinity";
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    auto& binding = _binding(kind);
    ++binding.token;
    const QString identity = kind == SelectedSource::Internal
                                 ? (_usingPluginSource ? QStringLiteral("Plugin") : QStringLiteral("Platform"))
                             : kind == SelectedSource::Simulated ? QStringLiteral("Simulated")
                                                                 : QStringLiteral("External GPS");
    binding.configure(source, source ? health : nullptr, identity,
                      kind == SelectedSource::Internal || kind == SelectedSource::Simulated, sessionId);
    _selectPositionSource();
}

void GPSPositionService::setSourceMode(SourceMode mode)
{
    if (QThread::currentThread() != thread()) {
        qCWarning(GPSPositionServiceLog) << "Position source changes require matching thread affinity";
        return;
    }
    if (mode < SourceMode::Automatic || mode > SourceMode::InternalOnly || _sourceMode == mode) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _sourceMode = mode;
    _clearPendingObservations();
    _forceSourceRefresh = true;
    _recovery = {};
    _recoveryTask.cancel();
    _selectPositionSource();
    _notifications.emitSignal(this, &GPSPositionService::sourceModeChanged);
}

QObject* GPSPositionService::_sourceFor(SelectedSource source) const
{
    return _binding(source).source();
}

void GPSPositionService::_selectPositionSource()
{
    if (!_bindings[static_cast<size_t>(SelectedSource::Internal)]) {
        return;
    }
    _selectionPending = true;
    if (_selectingSource) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _selectingSource = true;
    // Source activity changes can synchronously report positions that request another selection.
    do {
        _selectionPending = false;
        _setPositionSource(_choosePositionSource());
        if (_selectionPending) {
            continue;
        }
        const bool publishSelection = std::exchange(_selectionPublicationPending, false);
        const bool publishObservation = _binding(_selectedKind).pendingObservation;
        _clearPendingObservations();
        // Standby deadlines can run before the selected source's deadline.
        const bool publishedFixRejected = _published.position.isValid() && !_acceptedSourceObservation(_selectedKind);
        if (publishObservation || publishedFixRejected || (publishSelection && _sourceMode == SourceMode::Automatic)) {
            _externalPositionChanged();
            if (_selectionPending) {
                continue;
            }
        }
        _updateSelectionStatus();
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
        case SourceMode::Automatic:
            break;
    }
    const std::array priority = {SelectedSource::Receiver, SelectedSource::Nmea, internal};
    const bool currentAvailable =
        std::find(priority.begin(), priority.end(), _selectedKind) != priority.end() && _sourceFor(_selectedKind);
    std::optional<SelectedSource> best;
    for (const auto kind : priority) {
        if (_acceptedSourceObservation(kind)) {
            best = kind;
            break;
        }
    }
    const qint64 nowMs = _scheduler->nowMs();
    if (best && *best != _selectedKind && currentAvailable && _acceptedSourceObservation(_selectedKind)) {
        if (_recovery.candidate != best) {
            _recovery = {best, nowMs};
        }
        const auto remaining = RECOVERY_DELAY - std::chrono::milliseconds(nowMs - _recovery.sinceMs);
        if (remaining > std::chrono::milliseconds::zero()) {
            _recoveryTask.schedule(remaining, [this]() { _selectPositionSource(); });
            return _selectedKind;
        }
    }
    _recovery = {};
    _recoveryTask.cancel();
    if (best) {
        return *best;
    }
    if (currentAvailable) {
        return _selectedKind;
    }
    for (const auto kind : priority) {
        if (_sourceFor(kind)) {
            return kind;
        }
    }
    return internal;
}

void GPSPositionService::_refreshSourceBindings()
{
    for (const auto kind : SOURCE_KINDS) {
        auto& binding = _binding(kind);
        binding.rawBindingAllowed = _canBindSource(kind, binding.producer, binding.providedHealth);
    }
    for (const auto kind : SOURCE_KINDS) {
        auto& binding = _binding(kind);
        bool observe = binding.health() != nullptr;
        for (const auto other : SOURCE_KINDS) {
            if (other == kind) {
                break;
            }
            if (_binding(other).health() == binding.health()) {
                observe = false;
                break;
            }
        }
        // Shared health reports once, even when it serves several roles.
        binding.observeHealth(observe);
    }
}

void GPSPositionService::_sourceObservationChanged(SelectedSource kind)
{
    auto* health = _binding(kind).health();
    if (!health) {
        return;
    }
    for (const auto other : SOURCE_KINDS) {
        if (_binding(other).health() == health) {
            _binding(other).pendingObservation = true;
        }
    }
    _selectPositionSource();
}

void GPSPositionService::_updateSourceActivity()
{
    const QPointer<GPSPositionService> guard(this);
    for (const auto kind : SOURCE_KINDS) {
        _binding(kind).setActive(_sourceMode == SourceMode::Automatic || _selectedKind == kind);
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
            return tr("RTK receiver");
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
                         : _published.position.isValid()            ? SourceStatus::Active
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
    const bool changed = _selectedSource != selected || _sourceStatus != status;
    _selectedSource = selected;
    _sourceStatus = status;
    const auto name = selectedSourceName();
    if (changed || _selectionName != name) {
        _selectionName = name;
        _notifications.emitSignal(this, &GPSPositionService::selectionChanged);
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
    auto* health = _binding(source).health();
    auto observation = health ? health->acceptedObservation(use, maximumAge) : std::nullopt;
    const quint64 session = _binding(source).sessionId;
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
        if (_published.position.isValid() || _currentHealth->state() != GPSSourceHealth::State::Stale) {
            _clearPosition();
        }
        return;
    }
    _gcsPositioningError = QGeoPositionInfoSource::NoError;
    _publishPosition(accepted);
}

void GPSPositionService::_publishPosition(const std::optional<GPSObservation>& observation)
{
    if (observation) {
        _published.position = observation->position.coordinate();
        _published.timestamp = observation->receivedAt;
        _published.heading = observation->heading();
        _published.horizontalAccuracy = observation->position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    } else {
        _published = {};
    }
    _updateSelectionStatus();
    if (_published.horizontalAccuracy != _notified.horizontalAccuracy) {
        _notified.horizontalAccuracy = _published.horizontalAccuracy;
        _notifications.emitSignal(this, &GPSPositionService::gcsPositionHorizontalAccuracyChanged,
                                  _published.horizontalAccuracy);
    }
    if (_published.heading != _notified.heading && !(qIsNaN(_published.heading) && qIsNaN(_notified.heading))) {
        _notified.heading = _published.heading;
        _notifications.emitSignal(this, &GPSPositionService::gcsHeadingChanged, _published.heading);
    }
    if (_published.position != _notified.position) {
        _notified.position = _published.position;
        _notifications.emitSignal(this, &GPSPositionService::gcsPositionChanged, _published.position);
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
    const QPointer<QObject> nextSource = _sourceFor(source);
    QPointer<GPSSourceHealth> nextHealth = nextSource ? _binding(source).health() : nullptr;
    if (!_forceSourceRefresh && _selectedKind == source && _currentSource == nextSource &&
        _currentHealth == nextHealth && _selectedBindingRevision == _binding(source).generation) {
        _updateSourceActivity();
        return;
    }
    qCDebug(GPSPositionServiceLog) << "Ground-station position source changed"
                                   << "source:" << static_cast<int>(source) << "previous:" << _currentSource
                                   << "selected:" << nextSource;
    _forceSourceRefresh = false;
    _currentSource = nextSource;
    _currentHealth = nextHealth;
    _selectedKind = source;
    _selectedBindingRevision = _binding(source).generation;
    _selectionObservationRevision = _currentHealth ? _currentHealth->observationRevision() : 0;
    _selectedObservationAuthorized = false;
    if (_sourceMode != SourceMode::Automatic) {
        _binding(source).pendingObservation = false;
    }
    _selectionPublicationPending = true;
    _updateInterval = _binding(source).updateInterval();
    _clearPosition();
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    _updateSourceActivity();
}
