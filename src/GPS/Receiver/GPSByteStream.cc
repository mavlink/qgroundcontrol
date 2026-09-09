#include "GPSByteStream.h"

#include <QtCore/QMutexLocker>

#include <algorithm>
#include <cstring>

#include "GPSSourceHealth.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSByteStreamLog, "GPS.Receiver.GPSByteStream")

bool GPSByteBuffer::append(const QByteArray& bytes, quint64 receivedAtUs)
{
    if (bytes.isEmpty()) {
        return false;
    }
    const QMutexLocker lock(&_mutex);
    const bool notify = _chunks.empty() && !_gap;
    while (!_chunks.empty() && (_size + bytes.size() > kCapacity || _chunks.size() >= kMaxChunks)) {
        _size -= _chunks.front().bytes.size();
        _chunks.pop_front();
        _gap = true;
    }
    if (bytes.size() > kCapacity) {
        _gap = true;
    }
    _chunks.push_back({bytes.right(kCapacity), receivedAtUs});
    _size += _chunks.back().bytes.size();
    return notify;
}

qint64 GPSByteBuffer::read(char* data, qint64 length, quint64& receivedAtUs)
{
    if (length <= 0) {
        return 0;
    }
    const QMutexLocker lock(&_mutex);
    const quint64 now = GPSObservation::monotonicNowUs();
    while (!_chunks.empty() && (_chunks.front().receivedAtUs > now ||
                                now - _chunks.front().receivedAtUs >= GPSSourceHealth::FRESHNESS_TIMEOUT_MS * 1000u)) {
        _size -= _chunks.front().bytes.size();
        _chunks.pop_front();
        _gap = true;
    }
    if (_gap) {
        // The NMEA splitter discards a partial sentence when it encounters this boundary.
        data[0] = '\0';
        receivedAtUs = now;
        _gap = false;
        return 1;
    }
    if (_chunks.empty()) {
        return 0;
    }
    auto& chunk = _chunks.front();
    const qint64 count = std::min(length, static_cast<qint64>(chunk.bytes.size()));
    std::memcpy(data, chunk.bytes.constData(), static_cast<size_t>(count));
    receivedAtUs = chunk.receivedAtUs;
    chunk.bytes.remove(0, count);
    _size -= count;
    if (chunk.bytes.isEmpty()) {
        _chunks.pop_front();
    }
    return count;
}

qint64 GPSByteBuffer::size() const
{
    const QMutexLocker lock(&_mutex);
    return _size + (_gap ? 1 : 0);
}

GPSByteStream::GPSByteStream(QObject* parent)
    : QIODevice(parent)
    , _buffer(std::make_shared<GPSByteBuffer>())
{
    qCDebug(GPSByteStreamLog) << this;
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

GPSByteStream::~GPSByteStream()
{
    qCDebug(GPSByteStreamLog) << this;
}

qint64 GPSByteStream::bytesAvailable() const
{
    return _buffer->size() + QIODevice::bytesAvailable();
}

qint64 GPSByteStream::readData(char* data, qint64 length)
{
    return _buffer->read(data, length, _lastReadTimestampUs);
}

void GPSByteStream::notifyReadyRead()
{
    if (bytesAvailable() > 0) {
        emit readyRead();
    }
}
