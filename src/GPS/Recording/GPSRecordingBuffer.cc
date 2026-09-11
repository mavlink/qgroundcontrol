#include "GPSRecordingBuffer.h"

#include <QtCore/QMutexLocker>

#include <utility>

#include "GPSConfigurationReport.h"
#include "GPSObservation.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingBufferLog, "GPS.Recording.GPSRecordingBuffer")
QGC_LOGGING_CATEGORY(GPSRecordingStreamLog, "GPS.Recording.GPSRecordingStream")

GPSRecordingBuffer::GPSRecordingBuffer(Clock clock)
    : _clock(clock ? std::move(clock) : GPSObservation::monotonicNowUs)
{
    qCDebug(GPSRecordingBufferLog) << this;
}

GPSRecordingBuffer::~GPSRecordingBuffer()
{
    qCDebug(GPSRecordingBufferLog) << this;
}

bool GPSRecordingBuffer::setProvenance(const GPSRecordingProvenance& provenance)
{
    const QMutexLocker lock(&_mutex);
    if (_status.recording || !provenance.valid())
        return false;
    _provenance = provenance;
    return true;
}

bool GPSRecordingBuffer::start()
{
    const QMutexLocker lock(&_mutex);
    if (_status.recording) {
        return false;
    }
    _events.clear();
    _seenStreams.clear();
    _status = {.recording = true};
    _storageBytes = 0;
    _originUs = nowUs();
    _recording = true;
    return true;
}

void GPSRecordingBuffer::stop()
{
    const QMutexLocker lock(&_mutex);
    _recording = false;
    _status.recording = false;
}

GPSRecordingBuffer::Status GPSRecordingBuffer::status() const
{
    const QMutexLocker lock(&_mutex);
    return _status;
}

quint64 GPSRecordingBuffer::nowUs() const
{
    return _clock();
}

quint64 GPSRecordingBuffer::allocateStream()
{
    return _nextStream.fetch_add(1);
}

void GPSRecordingBuffer::append(quint64 stream, const GPSRecordingMetadata& metadata, bool alreadyOpen, Kind kind,
                                QByteArrayView bytes, int value, quint64 startedAtUs,
                                std::optional<GPSWriteResult> writeResult, bool fatal, quint64 receivedAtUs,
                                std::optional<GPSOpenStatus> openStatus, std::optional<GPSReadStatus> readStatus)
{
    if (!_recording.load(std::memory_order_relaxed)) {
        return;
    }
    const QMutexLocker lock(&_mutex);
    if (!_status.recording) {
        return;
    }
    const bool newStream = !_seenStreams.contains(stream);
    const bool resumed = newStream && alreadyOpen && kind != Kind::Open;
    const qsizetype additionalEvents = 1 + newStream + resumed;
    // The fixed allowance covers JSON keys, numeric fields and the allowlisted session metadata.
    const qsizetype overhead = additionalEvents * 384 + (newStream ? 1024 : 0);
    if (bytes.size() > (MAX_STORAGE_BYTES - _storageBytes - overhead) / 2 ||
        _storageBytes + overhead > MAX_STORAGE_BYTES || _events.size() + additionalEvents > MAX_EVENTS) {
        _status.recording = false;
        _status.limitReached = true;
        _recording = false;
        return;
    }
    const quint64 now = nowUs();
    const quint64 atUs =
        qMax(_events.isEmpty() ? quint64(1) : _events.last().atUs, now >= _originUs ? now - _originUs + 1 : quint64(1));
    if (newStream) {
        _seenStreams.insert(stream);
        Event session;
        session.atUs = atUs;
        session.stream = stream;
        session.kind = Kind::Session;
        session.metadata = metadata;
        if (!_provenance.producer.isEmpty())
            session.metadata.provenance = _provenance;
        _events.append(std::move(session));
    }
    if (resumed) {
        Event open;
        open.atUs = atUs;
        open.stream = stream;
        open.kind = Kind::Open;
        open.resumed = true;
        _events.append(std::move(open));
    }
    Event event;
    event.atUs = atUs;
    event.startedAtUs = startedAtUs >= _originUs ? qMin(startedAtUs - _originUs + 1, atUs) : 0;
    event.stream = stream;
    event.kind = kind;
    event.bytes = QByteArray(bytes.data(), bytes.size());
    event.value = value;
    event.writeResult = writeResult;
    event.fatal = fatal;
    event.openStatus = openStatus;
    event.readStatus = readStatus;
    if (receivedAtUs) {
        event.receivedAtUs = static_cast<qint64>(receivedAtUs) - static_cast<qint64>(_originUs) + 1;
    }
    _events.append(std::move(event));
    _storageBytes += overhead + bytes.size() * 2;
    _status.eventCount = _events.size();
    _status.bytesRecorded += bytes.size();
}

std::optional<GPSRecordingDocument> GPSRecordingBuffer::snapshot() const
{
    const QMutexLocker lock(&_mutex);
    if (_status.recording) {
        return std::nullopt;
    }
    return GPSRecordingDocument{.events = _events, .limitReached = _status.limitReached};
}

QByteArray GPSRecordingBuffer::exportJson() const
{
    const auto document = snapshot();
    return document ? document->encode() : QByteArray();
}

GPSRecordingStream::GPSRecordingStream(std::shared_ptr<GPSRecordingBuffer> buffer, GPSRecordingMetadata metadata)
    : _buffer(std::move(buffer))
    , _metadata(metadata)
    , _id(_buffer ? _buffer->allocateStream() : 0)
{
    qCDebug(GPSRecordingStreamLog) << this;
}

GPSRecordingStream::~GPSRecordingStream()
{
    qCDebug(GPSRecordingStreamLog) << this;
    closed();
}

quint64 GPSRecordingStream::nowUs() const
{
    return _buffer ? _buffer->nowUs() : 0;
}

void GPSRecordingStream::opened(bool success, quint64 startedAtUs)
{
    record(success ? GPSRecordingBuffer::Kind::Open : GPSRecordingBuffer::Kind::OpenError, {}, 0, startedAtUs);
    _opened = success;
}

void GPSRecordingStream::closed(int reason)
{
    if (_opened) {
        record(GPSRecordingBuffer::Kind::Close, {}, reason);
        _opened = false;
    }
}

void GPSRecordingStream::record(GPSRecordingBuffer::Kind kind, QByteArrayView bytes, int value, quint64 startedAtUs,
                                quint64 receivedAtUs)
{
    if (_buffer) {
        _buffer->append(_id, _metadata, _opened, kind, bytes, value, startedAtUs, {}, false, receivedAtUs);
    }
}

void GPSRecordingStream::configurationStarted()
{
    record(GPSRecordingBuffer::Kind::ConfigurationStarted);
}

void GPSRecordingStream::configurationFinished(int status)
{
    // The format's frozen status values do not depend on the driver's enum declaration order.
    using S = GPSConfigurationStatus;
    int recordedStatus = -1;
    switch (static_cast<S>(status)) {
        case S::NotConfigured:
            recordedStatus = 0;
            break;
        case S::Ready:
            recordedStatus = 1;
            break;
        case S::Unsupported:
            recordedStatus = 2;
            break;
        case S::Cancelled:
            recordedStatus = 3;
            break;
        case S::TransportError:
            recordedStatus = 4;
            break;
        case S::Failed:
            recordedStatus = 5;
            break;
    }
    record(GPSRecordingBuffer::Kind::ConfigurationFinished, {}, recordedStatus);
}

void GPSRecordingStream::recordWrite(QByteArrayView bytes, GPSWriteResult result, quint64 startedAtUs, bool fatal)
{
    result.detail.clear();
    if (_buffer) {
        _buffer->append(_id, _metadata, _opened, GPSRecordingBuffer::Kind::BoundedWrite, bytes, 0, startedAtUs, result,
                        fatal);
    }
}

void GPSRecordingStream::opened(GPSOpenResult result, quint64 startedAtUs)
{
    if (_buffer) {
        _buffer->append(_id, _metadata, _opened,
                        result.status == GPSOpenStatus::Opened ? GPSRecordingBuffer::Kind::Open
                                                               : GPSRecordingBuffer::Kind::OpenError,
                        {}, 0, startedAtUs, {}, false, 0, result.status);
    }
    _opened = result.status == GPSOpenStatus::Opened;
}

void GPSRecordingStream::recordRead(QByteArrayView bytes, GPSReadResult result, quint64 startedAtUs,
                                    quint64 receivedAtUs)
{
    using K = GPSRecordingBuffer::Kind;
    K kind = K::ReadError;
    switch (result.status) {
        case GPSReadStatus::Data:
            kind = K::Rx;
            break;
        case GPSReadStatus::TimedOut:
            kind = K::Timeout;
            break;
        case GPSReadStatus::Cancelled:
            kind = K::Cancel;
            break;
        case GPSReadStatus::Closed:
            kind = K::Disconnect;
            break;
        case GPSReadStatus::Error:
        case GPSReadStatus::Overflow:
        case GPSReadStatus::InvalidData:
            break;
    }
    if (_buffer) {
        _buffer->append(_id, _metadata, _opened, kind, bytes, 0, startedAtUs, {}, false, receivedAtUs, {},
                        result.status);
    }
}
