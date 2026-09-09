#include "GPSReceiverMailbox.h"

#include <QtCore/QMutexLocker>

#include <utility>

#include "QGCLoggingCategory.h"
#include "RTCMParser.h"

QGC_LOGGING_CATEGORY(GPSReceiverMailboxLog, "GPS.Receiver.GPSReceiverMailbox")

GPSReceiverMailbox::GPSReceiverMailbox()
{
    qCDebug(GPSReceiverMailboxLog) << this;
}

GPSReceiverMailbox::~GPSReceiverMailbox()
{
    qCDebug(GPSReceiverMailboxLog) << this;
}

namespace {
bool validCorrection(const QByteArray& data)
{
    if (data.size() < 8 || data.size() > GPSReceiverMailbox::MAX_FRAME_BYTES) {
        return false;
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    const qsizetype payloadSize = ((bytes[1] & 3) << 8) | bytes[2];
    if (bytes[0] != RTCMParser::kPreamble || (bytes[1] & 0xfc) != 0 || data.size() != payloadSize + 6) {
        return false;
    }
    const auto offset = data.size() - 3;
    const uint32_t crc = (uint32_t(bytes[offset]) << 16) | (uint32_t(bytes[offset + 1]) << 8) | bytes[offset + 2];
    return RTCMParser::crc24q(bytes, offset) == crc;
}
}  // namespace

bool GPSReceiverMailbox::_schedule()
{
    return !std::exchange(_scheduled, true);
}

bool GPSReceiverMailbox::_fresh(qint64 receivedAtMs, qint64 nowMs)
{
    return receivedAtMs > 0 && receivedAtMs <= nowMs && nowMs - receivedAtMs < MAX_AGE_MS;
}

bool GPSReceiverMailbox::publish(const GPSObservation& observation)
{
    const QMutexLocker lock(&_mutex);
    if (_closed) {
        return false;
    }
    _stats.coalescedSnapshots += _pending.position.has_value();
    _pending.position = observation;
    return _schedule();
}

bool GPSReceiverMailbox::publish(const GPSSatelliteObservation& observation)
{
    const QMutexLocker lock(&_mutex);
    if (_closed) {
        return false;
    }
    _stats.coalescedSnapshots += _pending.satellites.has_value();
    _pending.satellites = observation;
    return _schedule();
}

bool GPSReceiverMailbox::publish(const GPSRelativeObservation& observation)
{
    const QMutexLocker lock(&_mutex);
    if (_closed) {
        return false;
    }
    _stats.coalescedSnapshots += _pending.relativePosition.has_value();
    _pending.relativePosition = observation;
    return _schedule();
}

bool GPSReceiverMailbox::publish(const GPSSurveyInStatus& status)
{
    const QMutexLocker lock(&_mutex);
    if (_closed) {
        return false;
    }
    _stats.coalescedSnapshots += _pending.survey.has_value();
    _pending.survey = status;
    _surveyReceivedAtMs = static_cast<qint64>(GPSObservation::monotonicNowUs() / 1000);
    return _schedule();
}

bool GPSReceiverMailbox::publishCorrection(const QByteArray& data, qint64 receivedAtMs)
{
    const QMutexLocker lock(&_mutex);
    if (_closed) {
        return false;
    }
    if (data.isEmpty() || data.size() > MAX_FRAME_BYTES) {
        ++_stats.droppedCorrections;
        return false;
    }
    if (_pending.corrections.size() >= MAX_CORRECTIONS) {
        _pending.corrections.pop_front();
        ++_stats.droppedCorrections;
    }
    _pending.corrections.push_back({data, receivedAtMs});
    return _schedule();
}

GPSReceiverMailbox::Batch GPSReceiverMailbox::take(qint64 nowMs)
{
    const QMutexLocker lock(&_mutex);
    Batch batch;
    batch.position = std::exchange(_pending.position, {});
    batch.satellites = std::exchange(_pending.satellites, {});
    batch.relativePosition = std::exchange(_pending.relativePosition, {});
    batch.survey = std::exchange(_pending.survey, {});
    if (batch.position && !_fresh(batch.position->monotonicTimestampUs / 1000, nowMs)) {
        batch.position.reset();
    }
    if (batch.satellites && !_fresh(batch.satellites->monotonicTimestampUs / 1000, nowMs)) {
        batch.satellites.reset();
    }
    if (batch.relativePosition && !_fresh(batch.relativePosition->monotonicTimestampUs / 1000, nowMs)) {
        batch.relativePosition.reset();
    }
    if (batch.survey && !_fresh(_surveyReceivedAtMs, nowMs)) {
        batch.survey.reset();
    }
    while (!_pending.corrections.empty() && batch.corrections.size() < FRAMES_PER_DRAIN) {
        Correction frame = std::move(_pending.corrections.front());
        _pending.corrections.pop_front();
        if (_fresh(frame.receivedAtMs, nowMs)) {
            batch.corrections.push_back(std::move(frame));
        } else {
            ++_stats.droppedCorrections;
        }
    }
    while (!_deliveries.empty() && batch.deliveries.size() < FRAMES_PER_DRAIN) {
        batch.deliveries.append(std::move(_deliveries.front()));
        _deliveries.pop_front();
    }
    batch.more = !_pending.corrections.empty() || !_deliveries.empty();
    _scheduled = batch.more;
    return batch;
}

bool GPSReceiverMailbox::setCorrectionsEnabled(bool enabled)
{
    const QMutexLocker lock(&_mutex);
    _correctionsEnabled = enabled && !_closed;
    if (!_correctionsEnabled) {
        _clearCommands(GPSCorrectionOutcome::NotReady);
    }
    return !_closed && !_deliveries.empty() && _schedule();
}

bool GPSReceiverMailbox::submitCorrection(const QByteArray& data, qint64 receivedAtMs, qint64 nowMs)
{
    GPSCorrectionFrame frame;
    frame.data = data;
    frame.receivedAtMs = receivedAtMs;
    return submitCorrection(frame, 0, nowMs).accepted;
}

GPSCorrectionSubmitResult GPSReceiverMailbox::submitCorrection(const GPSCorrectionFrame& frame,
                                                               quint64 destinationSession, qint64 nowMs)
{
    const QMutexLocker lock(&_mutex);
    std::optional<GPSCorrectionOutcome> rejected;
    if (_closed) {
        rejected = GPSCorrectionOutcome::Cancelled;
    } else if (!_correctionsEnabled) {
        rejected = GPSCorrectionOutcome::NotReady;
    } else if (!validCorrection(frame.data) || frame.receivedAtMs <= 0 || frame.receivedAtMs > nowMs) {
        rejected = GPSCorrectionOutcome::InvalidData;
    } else if (!_fresh(frame.receivedAtMs, nowMs)) {
        rejected = GPSCorrectionOutcome::Expired;
    } else if (_commands.size() >= MAX_COMMANDS ||
               _commands.size() + _deliveries.size() + _inFlight.has_value() >= MAX_DELIVERIES) {
        // Reserve a terminal report for every admitted command, even while the GUI is stalled.
        rejected = GPSCorrectionOutcome::Overflow;
    }
    if (rejected) {
        ++_stats.rejectedCommands;
        return {false, *rejected};
    }
    GPSCorrectionDelivery delivery;
    delivery.deliveryId = frame.deliveryId;
    delivery.source = frame.source;
    delivery.sourceInstance = frame.sourceInstance;
    delivery.sourceSession = frame.session;
    delivery.destinationId = QStringLiteral("localReceiver");
    delivery.destinationSession = destinationSession;
    delivery.requestedBytes = frame.data.size();
    _commands.push_back({frame.data, frame.receivedAtMs, std::move(delivery), ++_nextCommandId});
    _stats.queuedCommandBytes += frame.data.size();
    return {true, {}};
}

std::optional<GPSReceiverMailbox::Correction> GPSReceiverMailbox::takeCommand(qint64 nowMs)
{
    const QMutexLocker lock(&_mutex);
    if (_inFlight) {
        return {};
    }
    while (!_commands.empty()) {
        Correction frame = std::move(_commands.front());
        _commands.pop_front();
        if (_fresh(frame.receivedAtMs, nowMs)) {
            _inFlight = frame;
            return frame;
        }
        ++_stats.expiredCommands;
        _finish(frame, GPSCorrectionOutcome::Expired);
    }
    return {};
}

void GPSReceiverMailbox::_finish(const Correction& command, GPSCorrectionOutcome outcome, quint64 writtenBytes)
{
    GPSCorrectionDelivery delivery = command.delivery;
    delivery.writtenBytes = qMin(writtenBytes, delivery.requestedBytes);
    delivery.outcome = outcome;
    if (outcome == GPSCorrectionOutcome::Written && delivery.writtenBytes != delivery.requestedBytes) {
        delivery.outcome = GPSCorrectionOutcome::WriteFailed;
    }
    _stats.writtenCommandBytes += delivery.writtenBytes;
    if (delivery.outcome == GPSCorrectionOutcome::WriteFailed) {
        _stats.failedCommandBytes += delivery.requestedBytes - delivery.writtenBytes;
    } else if (delivery.outcome != GPSCorrectionOutcome::Written) {
        _stats.droppedCommandBytes += delivery.requestedBytes - delivery.writtenBytes;
    }
    _deliveries.push_back(std::move(delivery));
}

bool GPSReceiverMailbox::completeCommand(const Correction& command, GPSCorrectionOutcome outcome, quint64 writtenBytes)
{
    const QMutexLocker lock(&_mutex);
    if (!_inFlight || _inFlight->commandId != command.commandId) {
        return false;
    }
    _inFlight.reset();
    if (_closed) {
        return false;
    }
    _finish(command, outcome, writtenBytes);
    return _schedule();
}

bool GPSReceiverMailbox::scheduleDeliveryNotification()
{
    const QMutexLocker lock(&_mutex);
    return !_closed && !_deliveries.empty() && _schedule();
}

QList<GPSCorrectionDelivery> GPSReceiverMailbox::takeDeliveries()
{
    const QMutexLocker lock(&_mutex);
    QList<GPSCorrectionDelivery> result;
    result.reserve(_deliveries.size());
    while (!_deliveries.empty()) {
        result.append(std::move(_deliveries.front()));
        _deliveries.pop_front();
    }
    return result;
}

void GPSReceiverMailbox::_clearCommands(GPSCorrectionOutcome outcome)
{
    while (!_commands.empty()) {
        _finish(_commands.front(), outcome);
        _commands.pop_front();
    }
}

bool GPSReceiverMailbox::clearCommands()
{
    const QMutexLocker lock(&_mutex);
    _clearCommands(GPSCorrectionOutcome::Cleared);
    return !_closed && !_deliveries.empty() && _schedule();
}

void GPSReceiverMailbox::close()
{
    const QMutexLocker lock(&_mutex);
    _closed = true;
    _correctionsEnabled = false;
    _clearCommands(GPSCorrectionOutcome::Cancelled);
    _pending = {};
    _scheduled = false;
}

GPSReceiverMailbox::Stats GPSReceiverMailbox::stats() const
{
    const QMutexLocker lock(&_mutex);
    Stats result = _stats;
    result.pendingCorrections = _pending.corrections.size();
    result.pendingCommands = _commands.size();
    result.pendingDeliveries = _deliveries.size();
    return result;
}
