#include "GPSPositionService.h"

#include <algorithm>
#include <utility>

#include <QtCore/QThread>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"
QGC_LOGGING_CATEGORY(GPSPositionServiceLog, "GPS.PositionManager.GPSPositionService")

namespace {
using SelectedSource = GPSPositionService::SelectedSource;
constexpr std::array SOURCE_KINDS = {SelectedSource::Receiver, SelectedSource::Internal, SelectedSource::Simulated};
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
{}

GPSPositionService::SourceBinding::~SourceBinding()
{
    disconnectNotifications();
    active = false;
    // Stopping a backend may re-enter; the adapter guards its own lifetime.
    adapter.reset();
}

QGeoPositionInfoSource* GPSPositionService::SourceBinding::backend() const
{
    return adapter ? adapter->backend() : nullptr;
}

void GPSPositionService::SourceBinding::disconnectNotifications()
{
    observeHealth(false);
    QObject::disconnect(std::exchange(producerDestroyed, {}));
    QObject::disconnect(std::exchange(backendDestroyed, {}));
}

void GPSPositionService::SourceBinding::bind(GPSSourceHealth* nextProducer,
                                             std::unique_ptr<GPSPositionBackendAdapter> nextAdapter,
                                             quint64 nextSession)
{
    if (!nextAdapter && producer == nextProducer && !adapter && sessionId == nextSession) {
        return;
    }
    const QPointer<GPSPositionService> bindingGuard(owner);
    const quint64 revision = ++generation;
    const QPointer<GPSSourceHealth> producerGuard(nextProducer);
    const QPointer<QGeoPositionInfoSource> backendGuard(nextAdapter ? nextAdapter->backend() : nullptr);
    disconnectNotifications();
    active = false;
    producer = nullptr;
    // Stopping the retired backend may re-enter, rebind this role, or delete the next producer.
    if (auto retired = std::move(adapter)) {
        retired->retire();
    }
    if (!bindingGuard || revision != generation) {
        return;
    }
    if (nextAdapter && !backendGuard) {
        nextAdapter.reset();
    }
    adapter = std::move(nextAdapter);
    producer = adapter ? adapter->health() : producerGuard.data();
    sessionId = nextSession;
    if (producer) {
        producerDestroyed = QObject::connect(producer, &QObject::destroyed, owner, [this]() {
            ++generation;
            disconnectNotifications();
            producer = nullptr;
            active = false;
            owner->_bindingChanged(kind);
        });
    }
    if (adapter) {
        adapter->setEventHandler([service = owner, role = kind](QGeoPositionInfoSource::Error error) {
            service->_backendError(role, error);
        });
        backendDestroyed = QObject::connect(adapter->backend(), &QObject::destroyed, owner, [this]() {
            ++generation;
            disconnectNotifications();
            producer = nullptr;
            active = false;
            adapter.reset();
            owner->_bindingChanged(kind);
        });
    }
    observeHealth(true);
    owner->_bindingChanged(kind);
}

void GPSPositionService::SourceBinding::observeHealth(bool observe)
{
    auto* observed = observe ? health() : nullptr;
    if (!observed) {
        QObject::disconnect(std::exchange(observationConnection, {}));
    } else if (!observationConnection) {
        observationConnection = QObject::connect(observed, &GPSSourceHealth::positionChanged, owner,
                                                 [this]() { owner->_sourceObservationChanged(kind); });
    }
}

void GPSPositionService::SourceBinding::setActive(bool enabled)
{
    enabled = enabled && producer;
    if (active == enabled) {
        return;
    }
    active = enabled;
    if (adapter) {
        adapter->setActive(enabled);
    }
}

int GPSPositionService::SourceBinding::updateInterval() const
{
    return adapter ? adapter->updateInterval() : 0;
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
    if (!_canBindBackend(SelectedSource::Internal, source)) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _usingPluginSource = custom;
    _platformStatus = status;
    _bindBackend(SelectedSource::Internal, source, 0);
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
    if (_canBindBackend(SelectedSource::Simulated, source)) {
        _bindBackend(SelectedSource::Simulated, source, 0);
    }
}

GPSPositionSourceRegistration GPSPositionService::registerPositionSource(SelectedSource kind, GPSSourceHealth* producer,
                                                                         quint64 sessionId)
{
    if (kind != SelectedSource::Receiver || !producer || !_canBindProducer(producer)) {
        return {};
    }
    auto registration = _register(kind);
    _bindProducer(kind, producer, sessionId);
    return registration;
}

GPSPositionSourceRegistration GPSPositionService::registerPositionSource(SelectedSource kind,
                                                                         QGeoPositionInfoSource* backend,
                                                                         quint64 sessionId)
{
    if (kind != SelectedSource::Receiver || !backend || !_canBindBackend(kind, backend)) {
        return {};
    }
    auto registration = _register(kind);
    _bindBackend(kind, backend, sessionId);
    return registration;
}

GPSPositionSourceRegistration GPSPositionService::_register(SelectedSource kind)
{
    // Binding increments the token, so the registration owns the next one.
    return GPSPositionSourceRegistration(this, static_cast<int>(kind), _binding(kind).token + 1);
}

void GPSPositionService::_retireRegistration(int kindValue, quint64 token)
{
    if (QThread::currentThread() != thread()) {
        return;
    }
    const auto kind = static_cast<SelectedSource>(kindValue);
    if (kind != SelectedSource::Receiver || _binding(kind).token != token) {
        return;
    }
    _bindProducer(kind, nullptr, 0);
}

bool GPSPositionService::_canBindProducer(const QObject* producer) const
{
    return QThread::currentThread() == thread() && (!producer || producer->thread() == thread());
}

bool GPSPositionService::_canBindBackend(SelectedSource kind, QGeoPositionInfoSource* backend) const
{
    if (!_canBindProducer(backend)) {
        return false;
    }
    // Two roles would start and stop the same backend independently.
    for (const auto other : SOURCE_KINDS) {
        if (other != kind && backend && _binding(other).backend() == backend) {
            return false;
        }
    }
    return true;
}

void GPSPositionService::_bindProducer(SelectedSource kind, GPSSourceHealth* producer, quint64 sessionId)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    auto& binding = _binding(kind);
    ++binding.token;
    binding.bind(producer, nullptr, sessionId);
    _selectPositionSource();
}

void GPSPositionService::_bindBackend(SelectedSource kind, QGeoPositionInfoSource* backend, quint64 sessionId)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    auto& binding = _binding(kind);
    ++binding.token;
    const bool platform = kind == SelectedSource::Internal || kind == SelectedSource::Simulated;
    const QString identity = kind == SelectedSource::Internal
                                 ? (_usingPluginSource ? QStringLiteral("Plugin") : QStringLiteral("Platform"))
                             : kind == SelectedSource::Simulated ? QStringLiteral("Simulated")
                                                                 : QStringLiteral("External GPS");
    // Rebinding the same backend keeps its running adapter and observation.
    if (backend && binding.adapter && binding.adapter->backend() == backend &&
        binding.adapter->identity() == identity && binding.sessionId == sessionId) {
        _selectPositionSource();
        return;
    }
    binding.bind(nullptr,
                 backend
                     ? std::make_unique<GPSPositionBackendAdapter>(backend, identity, platform, sessionId, _scheduler)
                     : nullptr,
                 sessionId);
    _selectPositionSource();
}

void GPSPositionService::setSourceMode(SourceMode mode)
{
    if (QThread::currentThread() != thread()) {
        qCWarning(GPSPositionServiceLog) << "Position source changes require matching thread affinity";
        return;
    }
    if ((mode != SourceMode::Automatic && mode != SourceMode::ReceiverOnly && mode != SourceMode::InternalOnly) ||
        _sourceMode == mode) {
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

GPSSourceHealth* GPSPositionService::_sourceFor(SelectedSource source) const
{
    return _binding(source).health();
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
        case SourceMode::InternalOnly:
            return internal;
        case SourceMode::Automatic:
            break;
    }
    const std::array priority = {SelectedSource::Receiver, internal};
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

void GPSPositionService::_sourceObservationChanged(SelectedSource kind)
{
    _binding(kind).pendingObservation = true;
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

GPSSourceHealth* GPSPositionService::sourceHealth(SelectedSource kind) const
{
    return _binding(kind).health();
}

QString GPSPositionService::selectedSourceName() const
{
    switch (_selectedSource) {
        case SelectedSource::None:
            return tr("None");
        case SelectedSource::Receiver:
            return tr("GNSS receiver");
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
    const auto selected = _currentHealth ? _selectedKind : SelectedSource::None;
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
    } else if (_sourceMode != SourceMode::ReceiverOnly) {
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
    const QPointer<GPSSourceHealth> nextHealth = _sourceFor(source);
    if (!_forceSourceRefresh && _selectedKind == source && _currentHealth == nextHealth &&
        _selectedBindingRevision == _binding(source).generation) {
        _updateSourceActivity();
        return;
    }
    qCDebug(GPSPositionServiceLog) << "Ground-station position source changed"
                                   << "source:" << static_cast<int>(source) << "previous:" << _currentHealth
                                   << "selected:" << nextHealth;
    _forceSourceRefresh = false;
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
