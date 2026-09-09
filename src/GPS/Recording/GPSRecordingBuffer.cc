#include "GPSRecordingBuffer.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingBufferLog, "GPS.Recording.GPSRecordingBuffer")
QGC_LOGGING_CATEGORY(GPSRecordingStreamLog, "GPS.Recording.GPSRecordingStream")

namespace {
QString kindName(GPSRecordingBuffer::Kind kind)
{
    using K = GPSRecordingBuffer::Kind;
    switch (kind) {
        case K::Session:
            return QStringLiteral("session");
        case K::Open:
            return QStringLiteral("open");
        case K::OpenError:
            return QStringLiteral("open_error");
        case K::Rx:
            return QStringLiteral("rx");
        case K::Tx:
            return QStringLiteral("tx");
        case K::Baud:
            return QStringLiteral("baud");
        case K::BaudError:
            return QStringLiteral("baud_error");
        case K::Timeout:
            return QStringLiteral("timeout");
        case K::ReadError:
            return QStringLiteral("read_error");
        case K::WriteError:
            return QStringLiteral("write_error");
        case K::Disconnect:
            return QStringLiteral("disconnect");
        case K::Cancel:
            return QStringLiteral("cancel");
        case K::Close:
            return QStringLiteral("close");
        case K::ConfigurationStarted:
            return QStringLiteral("configuration_started");
        case K::ConfigurationFinished:
            return QStringLiteral("configuration_finished");
    }
    return {};
}
}  // namespace

GPSRecordingMetadata GPSRecordingMetadata::forReceiver(const GPSReceiverConfig& config, GPSType type)
{
    GPSRecordingMetadata metadata;
    metadata.receiver = config;
    metadata.driverType = static_cast<int>(type);
    metadata.configured = true;
    return metadata;
}

GPSRecordingBuffer::GPSRecordingBuffer(Clock clock) : _clock(clock ? std::move(clock) : GPSObservation::monotonicNowUs)
{
    qCDebug(GPSRecordingBufferLog) << this;
}

GPSRecordingBuffer::~GPSRecordingBuffer()
{
    qCDebug(GPSRecordingBufferLog) << this;
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
                                QByteArrayView bytes, int value, quint64 startedAtUs)
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
    _events.append(std::move(event));
    _storageBytes += overhead + bytes.size() * 2;
    _status.eventCount = _events.size();
    _status.bytesRecorded += bytes.size();
}

QByteArray GPSRecordingBuffer::exportJson() const
{
    QVector<Event> events;
    Status status;
    {
        const QMutexLocker lock(&_mutex);
        if (_status.recording) {
            return {};
        }
        events = _events;
        status = _status;
    }
    QJsonArray output;
    for (const auto& event : events) {
        QJsonObject item{{QStringLiteral("at_us"), static_cast<qint64>(event.atUs)},
                         {QStringLiteral("stream"), static_cast<qint64>(event.stream)},
                         {QStringLiteral("kind"), kindName(event.kind)}};
        if (!event.bytes.isEmpty()) {
            item.insert(QStringLiteral("hex"), QString::fromLatin1(event.bytes.toHex()));
        }
        if (event.startedAtUs) {
            item.insert(QStringLiteral("started_us"), static_cast<qint64>(event.startedAtUs));
        }
        if (event.resumed) {
            item.insert(QStringLiteral("resumed"), true);
        }
        if (event.kind == Kind::Session) {
            const auto& m = event.metadata;
            item.insert(
                QStringLiteral("profile"),
                QJsonObject{{QStringLiteral("transport"), static_cast<int>(m.transport)},
                            {QStringLiteral("protocol"), static_cast<int>(m.receiver.outputProtocol)},
                            {QStringLiteral("role"), static_cast<int>(m.receiver.role)},
                            {QStringLiteral("driver"), m.driverType},
                            {QStringLiteral("baud"), m.initialBaud},
                            {QStringLiteral("configured"), m.configured},
                            {QStringLiteral("constellation_mask"), m.receiver.constellationMask},
                            {QStringLiteral("dynamic_model"), m.receiver.dynamicModel},
                            {QStringLiteral("output_rate_hz"), m.receiver.outputRateHz},
                            {QStringLiteral("heading_offset_deg"), m.receiver.headingOffsetDeg},
                            {QStringLiteral("base"),
                             QJsonObject{{QStringLiteral("fixed"), m.receiver.base.useFixedBase},
                                         {QStringLiteral("survey_accuracy_m"), m.receiver.base.surveyInAccMeters},
                                         {QStringLiteral("survey_duration_s"), m.receiver.base.surveyInDurationSecs},
                                         {QStringLiteral("latitude"), m.receiver.base.fixedBaseLatitude},
                                         {QStringLiteral("longitude"), m.receiver.base.fixedBaseLongitude},
                                         {QStringLiteral("altitude_m"), m.receiver.base.fixedBaseAltitudeMeters},
                                         {QStringLiteral("accuracy_m"), m.receiver.base.fixedBaseAccuracyMeters}}}});
        } else if (event.value || event.kind == Kind::WriteError || event.kind == Kind::ConfigurationFinished) {
            item.insert(QStringLiteral("value"), event.value);
        }
        output.append(item);
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("version"), 1},
                                     {QStringLiteral("description"), QStringLiteral("QGC receiver recording")},
                                     {QStringLiteral("limit_reached"), status.limitReached},
                                     {QStringLiteral("events"), output}})
        .toJson(QJsonDocument::Compact);
}

GPSRecordingStream::GPSRecordingStream(std::shared_ptr<GPSRecordingBuffer> buffer, GPSRecordingMetadata metadata)
    : _buffer(std::move(buffer)), _metadata(metadata), _id(_buffer ? _buffer->allocateStream() : 0)
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

void GPSRecordingStream::record(GPSRecordingBuffer::Kind kind, QByteArrayView bytes, int value, quint64 startedAtUs)
{
    if (_buffer) {
        _buffer->append(_id, _metadata, _opened, kind, bytes, value, startedAtUs);
    }
}

void GPSRecordingStream::configurationStarted()
{
    record(GPSRecordingBuffer::Kind::ConfigurationStarted);
}

void GPSRecordingStream::configurationFinished(int status)
{
    record(GPSRecordingBuffer::Kind::ConfigurationFinished, {}, status);
}
