#include "GPSCorrectionRouter.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionRouterLog, "GPS.Corrections.GPSCorrectionRouter")

GPSCorrectionRouter::GPSCorrectionRouter(QObject* parent, Clock clock)
    : QObject(parent)
    , _clock(clock ? std::move(clock) : Clock(GPSCorrectionFrame::monotonicNowMs))
{
    qCDebug(GPSCorrectionRouterLog) << this;
    _statistics[0].active = true;
}

GPSCorrectionRouter::~GPSCorrectionRouter()
{
    qCDebug(GPSCorrectionRouterLog) << this;
}

int GPSCorrectionRouter::_sourceIndex(GPSCorrectionSource source)
{
    const int index = static_cast<int>(source);
    return index >= 0 && index < 4 ? index : -1;
}

int GPSCorrectionRouter::_priority(GPSCorrectionSource source)
{
    return source == GPSCorrectionSource::Unknown ? 4 : static_cast<int>(source);
}

QString GPSCorrectionRouter::_key(GPSCorrectionSource source, const QString& instance)
{
    return QString::number(static_cast<int>(source)) + QLatin1Char('/') + instance;
}

quint64 GPSCorrectionRouter::beginSourceSession(GPSCorrectionSource source, const QString& instance)
{
    const int index = _sourceIndex(source);
    if (index < 0 || _shutdown) {
        return 0;
    }
    const QPointer<GPSCorrectionRouter> guard(this);
    const quint64 revisionAfterEnd = _revision + 1;
    endSourceSession(source);
    if (!guard || _shutdown || _revision != revisionAfterEnd) {
        return 0;
    }
    const quint64 session = _statistics[index].session + 1;
    _statistics[index] = Statistics{};
    _statistics[index].session = session;
    _statistics[index].active = true;
    _configuredInstances[index] = instance;
    return session;
}

void GPSCorrectionRouter::endSourceSession(GPSCorrectionSource source)
{
    const int index = _sourceIndex(source);
    if (index < 0) {
        return;
    }
    ++_revision;
    _statistics[index].active = false;
    for (auto it = _sources.begin(); it != _sources.end();) {
        if (it->category == source) {
            it = _sources.erase(it);
        } else {
            ++it;
        }
    }
    _select(_clock());
    if (_lastSubmittedSource.startsWith(QString::number(index) + QLatin1Char('/'))) {
        _lastSubmittedSource.clear();
        emit sourceInvalidated();
    }
}

quint64 GPSCorrectionRouter::sourceSession(GPSCorrectionSource source) const
{
    const int index = _sourceIndex(source);
    return index < 0 ? 0 : _statistics[index].session;
}

QString GPSCorrectionRouter::sourceInstance(GPSCorrectionSource source) const
{
    const int index = _sourceIndex(source);
    return index < 0 ? QString() : _configuredInstances[index];
}

void GPSCorrectionRouter::setPolicy(Policy policy)
{
    if (_policy == policy) {
        return;
    }
    ++_revision;
    _policy = policy;
    _active.clear();
    _candidate.clear();
    _select(_clock());
}

void GPSCorrectionRouter::setSelectedSource(GPSCorrectionSource source, const QString& instance)
{
    if (_sourceIndex(source) < 0 || (_manualSource == source && _manualInstance == instance)) {
        return;
    }
    ++_revision;
    _manualSource = source;
    _manualInstance = instance;
    _active.clear();
    _candidate.clear();
    _select(_clock());
}

QString GPSCorrectionRouter::activeInstance() const
{
    const auto active = _sources.constFind(_active);
    return active != _sources.cend() && _eligible(active.value(), _clock()) ? active->instance : QString();
}

GPSCorrectionSource GPSCorrectionRouter::activeSource() const
{
    const auto active = _sources.constFind(_active);
    return active != _sources.cend() && _eligible(active.value(), _clock()) ? active->category
                                                                            : GPSCorrectionSource::Unknown;
}

void GPSCorrectionRouter::setSink(const QString& id, Sink sink)
{
    if (!sink) {
        removeSink(id);
        return;
    }
    setDetailedSink(
        id,
        [sink = std::move(sink)](const GPSCorrectionFrame& frame) {
            const quint64 queued = sink(frame);
            return Submission{queued, 0,
                              queued ? GPSCorrectionReason::None : GPSCorrectionReason::DestinationUnavailable};
        },
        false);
}

void GPSCorrectionRouter::setDetailedSink(const QString& id, DetailedSink sink, bool reportsWrites)
{
    if (!sink) {
        removeSink(id);
        return;
    }
    if (id.isEmpty() || (!_destinations.contains(id) && _destinations.size() >= MAX_DESTINATIONS)) {
        return;
    }
    ++_revision;
    _sinks.insert(id, {std::move(sink), reportsWrites});
    auto& destination = _destinations[id];
    destination.id = id;
    destination.reportsWrites = reportsWrites;
}

void GPSCorrectionRouter::removeSink(const QString& id)
{
    ++_revision;
    _sinks.remove(id);
    const auto pending = _pendingDeliveries;
    for (const auto& delivery : pending) {
        if (delivery.destination == id) {
            invalidateDestination(id, delivery.destinationSession);
        }
    }
}

GPSCorrectionRouter::Statistics* GPSCorrectionRouter::_currentStatistics(const GPSCorrectionFrame& frame)
{
    const int index = _sourceIndex(frame.source);
    return index >= 0 && _statistics[index].session == frame.session ? &_statistics[index] : nullptr;
}

void GPSCorrectionRouter::_recordEvent(const GPSCorrectionFrame& frame, GPSCorrectionStage stage,
                                       GPSCorrectionReason reason, quint64 bytes, const QString& destination,
                                       quint64 destinationSession)
{
    if (_events.size() >= MAX_EVENTS) {
        _events.removeFirst();
    }
    _events.append({++_nextEvent, _clock(), frame.deliveryId, frame.source, frame.sourceInstance, frame.session,
                    destination, destinationSession, stage, reason, bytes});
}

void GPSCorrectionRouter::_recordDrop(const GPSCorrectionFrame& frame, GPSCorrectionReason reason, quint64 bytes,
                                      const QString& destination, quint64 destinationSession)
{
    if (auto* stats = _currentStatistics(frame)) {
        ++stats->droppedFrames;
        stats->droppedBytes += bytes;
    }
    if (!destination.isEmpty() && _destinations.contains(destination)) {
        auto& stats = _destinations[destination];
        ++stats.droppedFrames;
        stats.droppedBytes += bytes;
    }
    _recordEvent(frame, GPSCorrectionStage::Dropped, reason, bytes, destination, destinationSession);
}

void GPSCorrectionRouter::recordRejectedFrame(GPSCorrectionFrame frame, GPSCorrectionReason reason)
{
    if (_shutdown || frame.data.isEmpty()) {
        return;
    }
    frame.deliveryId = ++_nextDelivery;
    if (frame.sourceInstance.isEmpty()) {
        frame.sourceInstance = sourceInstance(frame.source);
    }
    if (auto* stats = _currentStatistics(frame)) {
        ++stats->receivedFrames;
        stats->receivedBytes += frame.data.size();
        ++stats->filteredFrames;
    }
    _recordEvent(frame, GPSCorrectionStage::Received, GPSCorrectionReason::None, frame.data.size());
    _recordDrop(frame, reason, frame.data.size());
}

bool GPSCorrectionRouter::recordDelivery(const GPSCorrectionDelivery& delivery)
{
    const QString key = QString::number(delivery.deliveryId) + QLatin1Char('/') + delivery.destinationId;
    const auto it = _pendingDeliveries.find(key);
    if (it == _pendingDeliveries.end()) {
        return false;
    }
    const PendingDelivery pending = it.value();
    if (pending.destinationSession != delivery.destinationSession || pending.frame.source != delivery.source ||
        pending.frame.sourceInstance != delivery.sourceInstance || pending.frame.session != delivery.sourceSession ||
        pending.queuedBytes != delivery.requestedBytes || delivery.writtenBytes > pending.queuedBytes) {
        _recordEvent(pending.frame, GPSCorrectionStage::Dropped, GPSCorrectionReason::InvalidDelivery, 0,
                     pending.destination, pending.destinationSession);
        return false;
    }
    _pendingDeliveries.erase(it);
    auto& destination = _destinations[pending.destination];
    --destination.pendingFrames;
    destination.pendingBytes -= pending.queuedBytes;
    const bool complete =
        delivery.outcome == GPSCorrectionOutcome::Written && delivery.writtenBytes == pending.queuedBytes;
    if (delivery.writtenBytes > 0) {
        destination.writtenBytes += delivery.writtenBytes;
        destination.writtenFrames += complete ? 1 : 0;
        if (auto* stats = _currentStatistics(pending.frame)) {
            stats->writtenBytes += delivery.writtenBytes;
            stats->writtenFrames += complete ? 1 : 0;
        }
        _recordEvent(pending.frame, GPSCorrectionStage::Written, GPSCorrectionReason::None, delivery.writtenBytes,
                     pending.destination, pending.destinationSession);
    }
    if (!complete) {
        const auto reason = delivery.outcome == GPSCorrectionOutcome::Written ? GPSCorrectionReason::PartialWrite
                                                                              : gpsCorrectionReason(delivery.outcome);
        _recordDrop(pending.frame, reason, pending.queuedBytes - delivery.writtenBytes, pending.destination,
                    pending.destinationSession);
    }
    return true;
}

void GPSCorrectionRouter::invalidateDestination(const QString& id, quint64 session)
{
    for (auto it = _pendingDeliveries.begin(); it != _pendingDeliveries.end();) {
        const auto& delivery = it.value();
        if (delivery.destination != id || delivery.destinationSession != session) {
            ++it;
            continue;
        }
        auto& destination = _destinations[id];
        --destination.pendingFrames;
        destination.pendingBytes -= delivery.queuedBytes;
        ++destination.unconfirmedFrames;
        destination.unconfirmedBytes += delivery.queuedBytes;
        if (auto* stats = _currentStatistics(delivery.frame)) {
            ++stats->unconfirmedFrames;
            stats->unconfirmedBytes += delivery.queuedBytes;
        }
        _recordEvent(delivery.frame, GPSCorrectionStage::Unconfirmed, GPSCorrectionReason::DeliveryUnconfirmed,
                     delivery.queuedBytes, id, session);
        it = _pendingDeliveries.erase(it);
    }
}

bool GPSCorrectionRouter::_eligible(const Source& source, qint64 now) const
{
    if (source.lastRoutableMs <= 0 || now < source.lastRoutableMs ||
        now - source.lastRoutableMs >= FRESHNESS_TIMEOUT_MS) {
        return false;
    }
    return _policy != Policy::Manual ||
           (source.category == _manualSource && (_manualInstance.isEmpty() || source.instance == _manualInstance));
}

void GPSCorrectionRouter::_select(qint64 now)
{
    auto best = _sources.cend();
    for (auto it = _sources.cbegin(); it != _sources.cend(); ++it) {
        if (_eligible(it.value(), now) &&
            (best == _sources.cend() || _priority(it->category) < _priority(best->category))) {
            best = it;
        }
    }
    const auto active = _sources.constFind(_active);
    if (active == _sources.cend() || !_eligible(active.value(), now)) {
        _active = best == _sources.cend() ? QString() : best.key();
        _candidate.clear();
        return;
    }
    if (best == _sources.cend() || _priority(best->category) >= _priority(active->category) ||
        _policy != Policy::Automatic) {
        _candidate.clear();
        return;
    }
    if (_candidate != best.key()) {
        _candidate = best.key();
        _candidateSinceMs = now;
    } else if (now - _candidateSinceMs >= SWITCH_HOLD_DOWN_MS) {
        _active = _candidate;
        _candidate.clear();
    }
}

bool GPSCorrectionRouter::acceptFrame(GPSCorrectionFrame frame)
{
    const int index = _sourceIndex(frame.source);
    if (_shutdown || _submitting || index < 0 || frame.data.isEmpty()) {
        return false;
    }
    frame.deliveryId = ++_nextDelivery;
    auto& stats = _statistics[index];
    if (!stats.active || stats.session != frame.session) {
        _recordEvent(
            frame, GPSCorrectionStage::Dropped,
            stats.session != frame.session ? GPSCorrectionReason::SessionMismatch : GPSCorrectionReason::InactiveSource,
            frame.data.size());
        return false;
    }
    if (frame.sourceInstance.isEmpty()) {
        frame.sourceInstance = _configuredInstances[index];
    } else if (!_configuredInstances[index].isEmpty() && frame.sourceInstance != _configuredInstances[index]) {
        _recordEvent(frame, GPSCorrectionStage::Dropped, GPSCorrectionReason::SessionMismatch, frame.data.size());
        return false;
    }
    ++stats.receivedFrames;
    stats.receivedBytes += frame.data.size();
    _recordEvent(frame, GPSCorrectionStage::Received, GPSCorrectionReason::None, frame.data.size());
    const qint64 now = _clock();
    if (frame.receivedAtMs <= 0 || frame.receivedAtMs > now) {
        ++stats.filteredFrames;
        _recordDrop(frame, GPSCorrectionReason::InvalidTimestamp, frame.data.size());
        return false;
    }
    if (frame.validated) {
        ++stats.validatedFrames;
        stats.validatedBytes += frame.data.size();
        _recordEvent(frame, GPSCorrectionStage::Validated, GPSCorrectionReason::None, frame.data.size());
        stats.lastValidMs = (std::max) (stats.lastValidMs, frame.receivedAtMs);
    }
    const QString key = _key(frame.source, frame.sourceInstance);
    if (!_sources.contains(key) && _sources.size() >= MAX_SOURCE_INSTANCES) {
        auto oldest = _sources.end();
        for (auto it = _sources.begin(); it != _sources.end(); ++it) {
            if (it.key() != _active && (oldest == _sources.end() || it->lastReceivedMs < oldest->lastReceivedMs)) {
                oldest = it;
            }
        }
        if (oldest != _sources.end()) {
            _sources.erase(oldest);
        }
    }
    auto& source = _sources[key];
    source.category = frame.source;
    source.instance = frame.sourceInstance;
    source.session = frame.session;
    source.lastReceivedMs = (std::max) (source.lastReceivedMs, frame.receivedAtMs);
    if (frame.filtered || now - frame.receivedAtMs >= FRESHNESS_TIMEOUT_MS) {
        ++stats.filteredFrames;
        _recordDrop(frame, frame.filtered ? GPSCorrectionReason::MessageFiltered : GPSCorrectionReason::Expired,
                    frame.data.size());
        return false;
    }
    source.lastRoutableMs = (std::max) (source.lastRoutableMs, frame.receivedAtMs);
    _select(now);
    if (_policy != Policy::All && key != _active) {
        ++stats.filteredFrames;
        _recordDrop(frame, GPSCorrectionReason::NotSelected, frame.data.size());
        return false;
    }
    if (frame.validated && frame.messageId == 0 && frame.data.size() >= 8 &&
        static_cast<quint8>(frame.data[0]) == 0xD3) {
        frame.messageId = (static_cast<quint8>(frame.data[3]) << 4) | (static_cast<quint8>(frame.data[4]) >> 4);
    }
    ++stats.routedFrames;
    ++stats.selectedFrames;
    stats.selectedBytes += frame.data.size();
    _recordEvent(frame, GPSCorrectionStage::Selected, GPSCorrectionReason::None, frame.data.size());
    const QPointer<GPSCorrectionRouter> guard(this);
    const quint64 revision = _revision;
    const auto sinks = _sinks;
    _submitting = true;
    const QString submissionSource = key + QLatin1Char('#') + QString::number(frame.session);
    if (_lastSubmittedSource != submissionSource) {
        _lastSubmittedSource = submissionSource;
        emit sourceSelected(frame.source, frame.sourceInstance);
        if (!guard) {
            return false;
        }
        if (_shutdown || revision != _revision) {
            _submitting = false;
            return false;
        }
    }
    for (auto it = sinks.cbegin(); it != sinks.cend(); ++it) {
        if (it->reportsWrites && _pendingDeliveries.size() >= MAX_PENDING_DELIVERIES) {
            _recordDrop(frame, GPSCorrectionReason::DiagnosticsBackpressure, frame.data.size(), it.key());
            continue;
        }
        const Submission submitted = it->submit(frame);
        if (!guard) {
            return false;
        }
        if (_shutdown || revision != _revision) {
            _submitting = false;
            return false;
        }
        if (submitted.queuedBytes == 0) {
            _recordDrop(frame,
                        submitted.reason == GPSCorrectionReason::None ? GPSCorrectionReason::DestinationUnavailable
                                                                      : submitted.reason,
                        frame.data.size(), it.key(), submitted.destinationSession);
            continue;
        }
        auto& sourceStats = _statistics[index];
        sourceStats.submittedBytes += submitted.queuedBytes;
        ++sourceStats.queuedFrames;
        sourceStats.queuedBytes += submitted.queuedBytes;
        auto& destination = _destinations[it.key()];
        destination.session = submitted.destinationSession;
        ++destination.queuedFrames;
        destination.queuedBytes += submitted.queuedBytes;
        _recordEvent(frame, GPSCorrectionStage::Queued, GPSCorrectionReason::None, submitted.queuedBytes, it.key(),
                     submitted.destinationSession);
        if (it->reportsWrites) {
            ++destination.pendingFrames;
            destination.pendingBytes += submitted.queuedBytes;
            _pendingDeliveries.insert(QString::number(frame.deliveryId) + QLatin1Char('/') + it.key(),
                                      {frame, it.key(), submitted.destinationSession, submitted.queuedBytes});
        }
    }
    _submitting = false;
    emit frameRouted(frame);
    return true;
}

void GPSCorrectionRouter::shutdown()
{
    ++_revision;
    _shutdown = true;
    for (auto& stats : _statistics) {
        stats.active = false;
    }
    _sources.clear();
    _active.clear();
    _candidate.clear();
    _sinks.clear();
    const auto pending = _pendingDeliveries;
    for (const auto& delivery : pending) {
        invalidateDestination(delivery.destination, delivery.destinationSession);
    }
}
