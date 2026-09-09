#include "GPSReplayTransport.h"

#include <QtCore/QFile>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReplayTransportLog, "GPS.Test.ReplayTransport")

bool GPSReplayTrace::load(const QString& filename, GPSReplayTrace& result, QString& error, quint64 streamId)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    constexpr qint64 maxTraceBytes = GPSRecordingDocument::MAX_BYTES;
    if (file.size() > maxTraceBytes) {
        error = QStringLiteral("Replay trace exceeds 4 MiB");
        return false;
    }
    return fromJson(file.read(maxTraceBytes + 1), result, error, streamId);
}

bool GPSReplayTrace::fromJson(const QByteArray& json, GPSReplayTrace& result, QString& error, quint64 streamId)
{
    GPSRecordingDocument document;
    GPSReplayTrace parsed;
    if (!GPSRecordingDocument::decode(json, document, error) ||
        !document.selectStream(streamId, parsed.recordedEvents, parsed.profile, parsed.streamId, error)) {
        return false;
    }
    parsed.limitReached = document.limitReached;
    for (const auto& event : parsed.recordedEvents) {
        using K = GPSRecordingEvent::Kind;
        if (event.kind != K::Session && event.kind != K::Close && event.kind != K::ConfigurationStarted &&
            event.kind != K::ConfigurationFinished) {
            parsed.events.append(event);
        }
    }
    result = std::move(parsed);
    error.clear();
    return true;
}

GPSReplayTransport::GPSReplayTransport(GPSReplayClock& clock, std::atomic_bool& requestStop, GPSReplayTrace trace,
                                       int maximumRead)
    : GPSTransport(requestStop),
      _clock(clock),
      _stop(requestStop),
      _trace(std::move(trace)),
      _maximumRead(qMax(maximumRead, 1))
{
    qCDebug(GPSReplayTransportLog) << this;
    _baudrate = _trace.profile ? _trace.profile->initialBaud : 0;
    for (const auto& event : _trace.events) {
        if (event.receivedAtUs && *event.receivedAtUs <= 0) {
            _originUs = qMax(_originUs, static_cast<quint64>(1 - *event.receivedAtUs));
        }
    }
    _clock.advanceTo(_originUs + 1);
}

GPSReplayTransport::~GPSReplayTransport()
{
    qCDebug(GPSReplayTransportLog) << this;
}

int GPSReplayTransport::_fail(const QString& message)
{
    if (_failure.isEmpty()) {
        _failure = QStringLiteral("Event %1 at %2 us: %3").arg(_index).arg(_clock.nowUs()).arg(message);
    }
    _fatal = true;
    return -EIO;
}

GPSTransport::OpenResult GPSReplayTransport::open()
{
    if (isCancelled()) {
        return {.status = OpenStatus::Cancelled};
    }
    if (_index >= _trace.events.size() || (_trace.events[_index].kind != GPSReplayEvent::Kind::Open &&
                                           _trace.events[_index].kind != GPSReplayEvent::Kind::OpenError)) {
        _fail(QStringLiteral("Unexpected open"));
        return {.status = OpenStatus::Error, .detail = _failure};
    }
    const auto& event = _trace.events[_index++];
    _clock.advanceTo(_eventTime(event.atUs));
    const auto status =
        event.openStatus.value_or(event.kind == GPSReplayEvent::Kind::Open ? OpenStatus::Opened : OpenStatus::Error);
    _opened = status == OpenStatus::Opened;
    _fatal = !_opened;
    return {.status = status};
}

GPSTransport::ReadResult GPSReplayTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    ++_readCount;
    if (isCancelled()) {
        return {.status = ReadStatus::Cancelled};
    }
    if (!buffer || length <= 0) {
        return {.status = ReadStatus::InvalidData};
    }
    if (!_opened || _fatal) {
        return {.status = ReadStatus::Closed};
    }
    const auto deadline = _clock.nowUs() + quint64(qMax(timeoutMs, 0)) * 1000;
    if (_index >= _trace.events.size() ||
        _eventTime(_trace.events[_index].atUs) >
            deadline + (_trace.events[_index].kind == GPSReplayEvent::Kind::Timeout ? 1 : 0)) {
        _clock.advanceTo(deadline + (timeoutMs > 0 ? 1 : 0));
        return {.status = ReadStatus::TimedOut};
    }
    auto& event = _trace.events[_index];
    _clock.advanceTo(_eventTime(event.atUs));
    if (event.kind == GPSReplayEvent::Kind::Rx) {
        const auto count = std::min({qsizetype(length), qsizetype(_maximumRead), event.bytes.size() - _offset});
        std::memcpy(buffer, event.bytes.constData() + _offset, static_cast<size_t>(count));
        _lastReadTimestampUs = event.receivedAtUs
                                   ? static_cast<quint64>(static_cast<qint64>(_originUs) + *event.receivedAtUs)
                                   : _eventTime(event.atUs);
        _offset += count;
        if (_offset == event.bytes.size()) {
            _offset = 0;
            ++_index;
        }
        return {.status = ReadStatus::Data, .bytesRead = static_cast<int>(count)};
    }
    ReadStatus status;
    switch (event.kind) {
        case GPSReplayEvent::Kind::Timeout:
            status = ReadStatus::TimedOut;
            break;
        case GPSReplayEvent::Kind::Cancel:
            status = ReadStatus::Cancelled;
            break;
        case GPSReplayEvent::Kind::Disconnect:
            status = ReadStatus::Closed;
            break;
        case GPSReplayEvent::Kind::ReadError:
            status = ReadStatus::Error;
            break;
        default:
            _fail(QStringLiteral("Read encountered an expected write, baud change, or open"));
            return {.status = ReadStatus::Error, .detail = _failure};
    }
    status = event.readStatus.value_or(status);
    ++_index;
    if (status == ReadStatus::Cancelled) {
        _stop = true;
    }
    if (status == ReadStatus::Closed || status == ReadStatus::Error || status == ReadStatus::Overflow) {
        _fatal = true;
        _opened = false;
    }
    return {.status = status};
}

GPSTransport::WriteResult GPSReplayTransport::write(const uint8_t* buffer, int length)
{
    if (_index < _trace.events.size() && _trace.events[_index].kind == GPSReplayEvent::Kind::BoundedWrite) {
        return writeBounded(buffer, length, QDeadlineTimer(configurationWriteTimeout()));
    }
    const int result = _writeLegacy(buffer, length);
    if (result == length && result >= 0) {
        return {.status = WriteStatus::Completed, .acceptedBytes = result, .writtenBytes = result};
    }
    return {.status = isCancelled() ? WriteStatus::Cancelled : WriteStatus::Error,
            .acceptedBytes = qMax(result, 0),
            .writtenBytes = qMax(result, 0)};
}

int GPSReplayTransport::_writeLegacy(const uint8_t* buffer, int length)
{
    if (isCancelled()) {
        return -ECANCELED;
    }
    if (!_opened || _fatal || !buffer || length <= 0 || _index >= _trace.events.size()) {
        return _fail(QStringLiteral("Unexpected write"));
    }
    if (_trace.events[_index].kind == GPSReplayEvent::Kind::WriteError) {
        const auto event = _trace.events[_index++];
        if (!event.bytes.isEmpty() && event.bytes != QByteArrayView(reinterpret_cast<const char*>(buffer), length)) {
            return _fail(QStringLiteral("Failed TX bytes differ from trace"));
        }
        _clock.advanceTo(_eventTime(event.atUs));
        _fatal = true;
        return event.value < length ? event.value : -EIO;
    }
    int consumed = 0;
    while (consumed < length) {
        if (_index >= _trace.events.size() || _trace.events[_index].kind != GPSReplayEvent::Kind::Tx) {
            return _fail(QStringLiteral("Write exceeded expected TX bytes"));
        }
        const auto& event = _trace.events[_index];
        const auto count = std::min(qsizetype(length - consumed), event.bytes.size() - _offset);
        if (std::memcmp(buffer + consumed, event.bytes.constData() + _offset, static_cast<size_t>(count)) != 0) {
            return _fail(QStringLiteral("TX bytes differ from trace"));
        }
        _clock.advanceTo(_eventTime(event.atUs));
        consumed += static_cast<int>(count);
        _offset += count;
        if (_offset == event.bytes.size()) {
            _offset = 0;
            ++_index;
        }
    }
    return consumed;
}

bool GPSReplayTransport::setBaudrate(unsigned baudrate)
{
    if (isCancelled()) {
        return false;
    }
    if (_index < _trace.events.size() && _trace.events[_index].kind == GPSReplayEvent::Kind::BaudError &&
        _trace.events[_index].value == static_cast<int>(baudrate)) {
        _clock.advanceTo(_eventTime(_trace.events[_index++].atUs));
        return false;
    }
    if (_index >= _trace.events.size() || _trace.events[_index].kind != GPSReplayEvent::Kind::Baud ||
        _trace.events[_index].value != static_cast<int>(baudrate)) {
        _fail(QStringLiteral("Unexpected baud rate %1").arg(baudrate));
        return false;
    }
    _clock.advanceTo(_eventTime(_trace.events[_index++].atUs));
    _baudrate = baudrate;
    return true;
}

GPSTransport::WriteResult GPSReplayTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {.status = WriteStatus::Cancelled};
    }
    if (!_opened || _fatal || (!buffer && length > 0) || length < 0 || _index >= _trace.events.size()) {
        _fail(QStringLiteral("Unexpected bounded write"));
        return {.status = WriteStatus::Error};
    }
    const auto& event = _trace.events[_index];
    if (event.kind != GPSReplayEvent::Kind::BoundedWrite || !event.writeResult ||
        event.bytes != QByteArrayView(reinterpret_cast<const char*>(buffer), length)) {
        _fail(QStringLiteral("Bounded write or bytes differ from trace"));
        return {.status = WriteStatus::Error};
    }
    const quint64 elapsed = _eventTime(event.atUs) > _clock.nowUs() ? _eventTime(event.atUs) - _clock.nowUs() : 0;
    if (!deadline.isForever() && elapsed > quint64(qMax<qint64>(deadline.remainingTime(), 0)) * 1000) {
        // A capture only proves progress at completion. Do not invent partial delivery at an earlier deadline.
        _fail(QStringLiteral("Replay deadline is shorter than recorded write duration"));
        return {.status = WriteStatus::Error};
    }
    _clock.advanceTo(_eventTime(event.atUs));
    const auto result = *event.writeResult;
    ++_index;
    if (result.status == WriteStatus::Cancelled) {
        _stop = true;
    }
    _fatal = event.fatal;
    return result;
}

std::chrono::milliseconds GPSReplayTransport::correctionWriteTimeout(int length) const
{
    if (_trace.profile && _trace.profile->transport == GPSRecordingMetadata::Transport::Serial) {
        return serialCorrectionWriteTimeout(length, _baudrate);
    }
    return GPSTransport::correctionWriteTimeout(length);
}
