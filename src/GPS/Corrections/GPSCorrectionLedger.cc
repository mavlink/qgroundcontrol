#include "GPSCorrectionLedger.h"

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionLedgerLog, "GPS.Corrections.GPSCorrectionLedger")

GPSCorrectionLedger::GPSCorrectionLedger(Clock clock)
    : _clock(std::move(clock))
{
    qCDebug(GPSCorrectionLedgerLog) << this;
    _statistics[0].active = true;
}

GPSCorrectionLedger::~GPSCorrectionLedger()
{
    qCDebug(GPSCorrectionLedgerLog) << this;
}

quint64 GPSCorrectionLedger::beginSource(GPSCorrectionSource source)
{
    const int index = static_cast<int>(source);
    if (index < 0 || index >= 4) {
        return 0;
    }
    auto& stats = _statistics[index];
    const auto session = stats.session + 1;
    stats = {};
    stats.session = session;
    stats.active = true;
    return session;
}

void GPSCorrectionLedger::endSource(GPSCorrectionSource source)
{
    const int index = static_cast<int>(source);
    if (index >= 0 && index < 4) {
        _statistics[index].active = false;
    }
}

void GPSCorrectionLedger::received(const GPSCorrectionFrame& frame)
{
    if (auto* stats = _currentStatistics(frame)) {
        ++stats->receivedFrames;
        stats->receivedBytes += frame.data.size();
    }
    recordEvent(frame, GPSCorrectionStage::Received, GPSCorrectionReason::None, frame.data.size());
}

void GPSCorrectionLedger::validated(const GPSCorrectionFrame& frame)
{
    if (auto* stats = _currentStatistics(frame)) {
        ++stats->validatedFrames;
        stats->validatedBytes += frame.data.size();
        stats->lastValidMs = (std::max) (stats->lastValidMs, frame.receivedAtMs);
    }
    recordEvent(frame, GPSCorrectionStage::Validated, GPSCorrectionReason::None, frame.data.size());
}

void GPSCorrectionLedger::filtered(const GPSCorrectionFrame& frame)
{
    if (auto* stats = _currentStatistics(frame)) {
        ++stats->filteredFrames;
    }
}

void GPSCorrectionLedger::selected(const GPSCorrectionFrame& frame)
{
    if (auto* stats = _currentStatistics(frame)) {
        ++stats->selectedFrames;
        stats->selectedBytes += frame.data.size();
    }
    recordEvent(frame, GPSCorrectionStage::Selected, GPSCorrectionReason::None, frame.data.size());
}

void GPSCorrectionLedger::queued(const GPSCorrectionFrame& frame, quint64 bytes, bool complete)
{
    if (auto* stats = _currentStatistics(frame)) {
        stats->queuedFrames += complete ? 1 : 0;
        stats->queuedBytes += bytes;
    }
}

void GPSCorrectionLedger::registerOutput(const QString& id, bool reportsWrites)
{
    _outputs.insert(id);
    auto& destination = _destinations[id];
    destination.id = id;
    destination.reportsWrites = reportsWrites;
}

void GPSCorrectionLedger::pruneDestinationHistory()
{
    qsizetype historyCount = 0;
    for (auto it = _destinations.cbegin(); it != _destinations.cend(); ++it) {
        if (!_outputs.contains(it.key()) && it->pendingFrames == 0) {
            ++historyCount;
        }
    }
    while (historyCount > MAX_DESTINATION_HISTORY) {
        auto oldest = _destinations.end();
        for (auto it = _destinations.begin(); it != _destinations.end(); ++it) {
            if (!_outputs.contains(it.key()) && it->pendingFrames == 0 &&
                (oldest == _destinations.end() || it->lastActivityMs < oldest->lastActivityMs)) {
                oldest = it;
            }
        }
        if (oldest == _destinations.end()) {
            return;
        }
        _destinations.erase(oldest);
        --historyCount;
    }
}

GPSCorrectionLedger::Statistics* GPSCorrectionLedger::_currentStatistics(const GPSCorrectionFrame& frame)
{
    const int index = static_cast<int>(frame.source);
    return index >= 0 && index < 4 && _statistics[index].session == frame.session ? &_statistics[index] : nullptr;
}

void GPSCorrectionLedger::recordEvent(const GPSCorrectionFrame& frame, GPSCorrectionStage stage,
                                      GPSCorrectionReason reason, quint64 bytes, const QString& destination,
                                      quint64 destinationSession)
{
    if (_events.size() >= MAX_EVENTS) {
        _events.removeFirst();
    }
    _events.append({++_nextEvent, _clock(), frame.deliveryId, frame.source, frame.sourceInstance, frame.session,
                    destination, destinationSession, stage, reason, bytes});
}

void GPSCorrectionLedger::recordDrop(const GPSCorrectionFrame& frame, GPSCorrectionReason reason, quint64 bytes,
                                     const QString& destination, quint64 destinationSession, bool creditSource)
{
    if (auto* stats = _currentStatistics(frame); stats && creditSource) {
        ++stats->droppedFrames;
        stats->droppedBytes += bytes;
    }
    if (!destination.isEmpty() && _destinations.contains(destination)) {
        auto& stats = _destinations[destination];
        ++stats.droppedFrames;
        stats.droppedBytes += bytes;
    }
    recordEvent(frame, GPSCorrectionStage::Dropped, reason, bytes, destination, destinationSession);
}

bool GPSCorrectionLedger::recordDelivery(const GPSCorrectionDelivery& delivery)
{
    const QString key = QString::number(delivery.deliveryId) + QLatin1Char('/') + delivery.destinationId;
    const auto it = _pendingDeliveries.find(key);
    if (it == _pendingDeliveries.end()) {
        return false;
    }
    const PendingDelivery pending = it.value();
    if (pending.destinationSession != delivery.destinationSession || pending.frame.source != delivery.source ||
        pending.frame.sourceInstance != delivery.sourceInstance || pending.frame.session != delivery.sourceSession ||
        pending.queuedBytes != delivery.requestedBytes || delivery.writtenBytes > pending.queuedBytes ||
        delivery.uncertainBytes > pending.queuedBytes - delivery.writtenBytes ||
        delivery.acceptedBytes > pending.queuedBytes || delivery.writtenBytes > delivery.acceptedBytes ||
        delivery.uncertainBytes > delivery.acceptedBytes - delivery.writtenBytes) {
        recordEvent(pending.frame, GPSCorrectionStage::Dropped, GPSCorrectionReason::InvalidDelivery, 0,
                    pending.destination, pending.destinationSession);
        return false;
    }
    _pendingDeliveries.erase(it);
    auto& destination = _destinations[pending.destination];
    --destination.pendingFrames;
    destination.pendingBytes -= pending.queuedBytes;
    destination.transportAcceptedBytes += delivery.acceptedBytes;
    if (auto* stats = _currentStatistics(pending.frame)) {
        stats->transportAcceptedBytes += delivery.acceptedBytes;
    }
    const bool complete = pending.complete && delivery.outcome == GPSCorrectionOutcome::Written &&
                          delivery.writtenBytes == pending.queuedBytes;
    if (delivery.writtenBytes > 0) {
        destination.writtenBytes += delivery.writtenBytes;
        destination.writtenFrames += complete ? 1 : 0;
        if (auto* stats = _currentStatistics(pending.frame)) {
            stats->writtenBytes += delivery.writtenBytes;
            stats->writtenFrames += complete ? 1 : 0;
        }
        recordEvent(pending.frame, GPSCorrectionStage::Written, GPSCorrectionReason::None, delivery.writtenBytes,
                    pending.destination, pending.destinationSession);
    }
    if (delivery.uncertainBytes > 0) {
        ++destination.unconfirmedFrames;
        destination.unconfirmedBytes += delivery.uncertainBytes;
        if (auto* stats = _currentStatistics(pending.frame)) {
            ++stats->unconfirmedFrames;
            stats->unconfirmedBytes += delivery.uncertainBytes;
        }
        recordEvent(pending.frame, GPSCorrectionStage::Unconfirmed, GPSCorrectionReason::DeliveryUnconfirmed,
                    delivery.uncertainBytes, pending.destination, pending.destinationSession);
    }
    const quint64 undelivered = pending.queuedBytes - delivery.writtenBytes - delivery.uncertainBytes;
    if (undelivered > 0) {
        const auto reason = delivery.outcome == GPSCorrectionOutcome::Written ? GPSCorrectionReason::PartialWrite
                                                                              : gpsCorrectionReason(delivery.outcome);
        recordDrop(pending.frame, reason, undelivered, pending.destination, pending.destinationSession);
    }
    pruneDestinationHistory();
    return true;
}

template <typename Predicate>
void GPSCorrectionLedger::_invalidatePending(Predicate matches)
{
    for (auto it = _pendingDeliveries.begin(); it != _pendingDeliveries.end();) {
        const auto& delivery = it.value();
        if (!matches(delivery)) {
            ++it;
            continue;
        }
        auto& destination = _destinations[delivery.destination];
        --destination.pendingFrames;
        destination.pendingBytes -= delivery.queuedBytes;
        ++destination.unconfirmedFrames;
        destination.unconfirmedBytes += delivery.queuedBytes;
        if (auto* stats = _currentStatistics(delivery.frame)) {
            ++stats->unconfirmedFrames;
            stats->unconfirmedBytes += delivery.queuedBytes;
        }
        recordEvent(delivery.frame, GPSCorrectionStage::Unconfirmed, GPSCorrectionReason::DeliveryUnconfirmed,
                    delivery.queuedBytes, delivery.destination, delivery.destinationSession);
        it = _pendingDeliveries.erase(it);
    }
    pruneDestinationHistory();
}

void GPSCorrectionLedger::invalidateDestination(const QString& id, quint64 session, quint64 excludedDeliveryId)
{
    _invalidatePending([&](const PendingDelivery& delivery) {
        return delivery.destination == id && delivery.destinationSession == session &&
               (!excludedDeliveryId || delivery.frame.deliveryId != excludedDeliveryId);
    });
}

void GPSCorrectionLedger::invalidateDelivery(quint64 deliveryId, const QString& outputId)
{
    _invalidatePending([&](const PendingDelivery& delivery) {
        return delivery.frame.deliveryId == deliveryId &&
               (outputId.isEmpty() || delivery.outputId == outputId || delivery.destination == outputId);
    });
}

void GPSCorrectionLedger::removeOutput(const QString& id, quint64 excludedDeliveryId)
{
    _outputs.remove(id);
    _invalidatePending([&](const PendingDelivery& delivery) {
        return (delivery.destination == id || delivery.outputId == id) &&
               (!excludedDeliveryId || delivery.frame.deliveryId != excludedDeliveryId);
    });
}

bool GPSCorrectionLedger::admitted(const GPSCorrectionFrame& frame, const QString& outputId, const QString& id,
                                   quint64 session, quint64 bytes, bool complete, bool reportsWrites)
{
    const QString key = QString::number(frame.deliveryId) + QLatin1Char('/') + id;
    if (reportsWrites && bytes && _pendingDeliveries.contains(key)) {
        recordEvent(frame, GPSCorrectionStage::Dropped, GPSCorrectionReason::InvalidDelivery, 0, id, session);
        return false;
    }
    auto& destination = _destinations[id];
    destination.id = id;
    destination.session = session;
    destination.lastActivityMs = _clock();
    destination.reportsWrites = reportsWrites;
    if (!bytes) {
        return true;
    }
    if (auto* stats = _currentStatistics(frame)) {
        stats->submittedBytes += bytes;
    }
    destination.queuedFrames += complete ? 1 : 0;
    destination.queuedBytes += bytes;
    recordEvent(frame, GPSCorrectionStage::Queued, GPSCorrectionReason::None, bytes, id, session);
    if (!reportsWrites) {
        return true;
    }
    if (!admissionAvailable()) {
        ++destination.unconfirmedFrames;
        destination.unconfirmedBytes += bytes;
        if (auto* stats = _currentStatistics(frame)) {
            ++stats->unconfirmedFrames;
            stats->unconfirmedBytes += bytes;
        }
        recordEvent(frame, GPSCorrectionStage::Unconfirmed, GPSCorrectionReason::DeliveryUnconfirmed, bytes, id,
                    session);
    } else {
        ++destination.pendingFrames;
        destination.pendingBytes += bytes;
        _pendingDeliveries.insert(key, {frame, id, session, bytes, outputId, complete});
    }
    return true;
}

void GPSCorrectionLedger::shutdown(quint64 excludedDeliveryId)
{
    for (auto& stats : _statistics) {
        stats.active = false;
    }
    _outputs.clear();
    _invalidatePending([&](const PendingDelivery& delivery) {
        return !excludedDeliveryId || delivery.frame.deliveryId != excludedDeliveryId;
    });
}
