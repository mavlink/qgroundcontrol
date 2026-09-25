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

void GPSCorrectionLedger::sampleReceivedByteRates(qint64 nowMs)
{
    const qint64 elapsedMs = nowMs - _rateSampleMs;
    for (auto& stats : _statistics) {
        const quint64 received = stats.receivedBytes - stats.sampledReceivedBytes;
        stats.receivedBytesPerSecond =
            _rateSampleMs > 0 && elapsedMs > 0 ? static_cast<quint64>(qRound64(received * 1000.0 / elapsedMs)) : 0;
        stats.sampledReceivedBytes = stats.receivedBytes;
    }
    _rateSampleMs = nowMs;
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
        ++stats->messageCounts[frame.messageId];
        stats->lastValidMs = (std::max) (stats->lastValidMs, frame.receivedAtMs);
    }
    recordEvent(frame, GPSCorrectionStage::Validated, GPSCorrectionReason::None, frame.data.size());
}

void GPSCorrectionLedger::selected(const GPSCorrectionFrame& frame)
{
    if (auto* stats = _currentStatistics(frame)) {
        ++stats->selectedFrames;
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

void GPSCorrectionLedger::registerOutput(const QString& id)
{
    _outputDestinations.insert(id, {});
    auto& destination = _destinations[id];
    destination.id = id;
}

void GPSCorrectionLedger::updateOutputDestinations(const QString& id, const QSet<QString>& destinations)
{
    if (const auto it = _outputDestinations.find(id); it != _outputDestinations.end() && *it != destinations) {
        *it = destinations;
    }
}

void GPSCorrectionLedger::pruneDestinationHistory()
{
    if (_destinations.size() <= MAX_DESTINATION_HISTORY) {
        return;
    }
    QSet<QString> live;
    for (auto it = _outputDestinations.cbegin(); it != _outputDestinations.cend(); ++it) {
        live.insert(it.key());
        live.unite(it.value());
    }
    qsizetype historyCount = 0;
    for (auto it = _destinations.cbegin(); it != _destinations.cend(); ++it) {
        if (!live.contains(it.key())) {
            ++historyCount;
        }
    }
    while (historyCount > MAX_DESTINATION_HISTORY) {
        auto oldest = _destinations.end();
        for (auto it = _destinations.begin(); it != _destinations.end(); ++it) {
            if (!live.contains(it.key()) &&
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
    _events.append({++_nextEvent, _clock(), frame.source, frame.sourceInstance, frame.session, destination,
                    destinationSession, stage, reason, bytes});
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

void GPSCorrectionLedger::removeOutput(const QString& id)
{
    _outputDestinations.remove(id);
    pruneDestinationHistory();
}

bool GPSCorrectionLedger::admitted(const GPSCorrectionFrame& frame, const QString& id, quint64 session, quint64 bytes,
                                   bool complete)
{
    auto& destination = _destinations[id];
    destination.id = id;
    destination.session = session;
    destination.lastActivityMs = _clock();
    if (!bytes) {
        return true;
    }
    destination.queuedFrames += complete ? 1 : 0;
    destination.queuedBytes += bytes;
    recordEvent(frame, GPSCorrectionStage::Queued, GPSCorrectionReason::None, bytes, id, session);
    return true;
}

void GPSCorrectionLedger::shutdown()
{
    for (auto& stats : _statistics) {
        stats.active = false;
        stats.receivedBytesPerSecond = 0;
    }
    _outputDestinations.clear();
    pruneDestinationHistory();
}
