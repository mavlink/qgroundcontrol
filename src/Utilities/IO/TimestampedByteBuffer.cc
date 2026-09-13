#include "TimestampedByteBuffer.h"

#include <QtCore/QMutexLocker>

#include <algorithm>
#include <cstring>

bool TimestampedByteBuffer::append(const QByteArray& bytes, quint64 receivedAtUs)
{
    if (bytes.isEmpty()) {
        return false;
    }
    const QMutexLocker lock(&_mutex);
    const bool notify = _chunks.empty() && !_gap;
    while (!_chunks.empty() && (_size + bytes.size() > kCapacity || _chunks.size() >= kMaxChunks)) {
        _size -= _chunks.front().remaining();
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

TimestampedByteBuffer::ReadResult TimestampedByteBuffer::read(char* data, qint64 length, quint64 nowUs,
                                                              quint64 maximumAgeUs)
{
    if (!data || length <= 0) {
        return {};
    }
    const QMutexLocker lock(&_mutex);
    const quint64 now = nowUs;
    while (!_chunks.empty() &&
           (_chunks.front().receivedAtUs > now || now - _chunks.front().receivedAtUs >= maximumAgeUs)) {
        _size -= _chunks.front().remaining();
        _chunks.pop_front();
        _gap = true;
    }
    if (_gap) {
        _gap = false;
        return {0, now, true};
    }
    if (_chunks.empty()) {
        return {};
    }
    auto& chunk = _chunks.front();
    const qint64 count = std::min(length, static_cast<qint64>(chunk.remaining()));
    std::memcpy(data, chunk.bytes.constData() + chunk.offset, static_cast<size_t>(count));
    const auto receivedAtUs = chunk.receivedAtUs;
    chunk.offset += count;
    _size -= count;
    if (chunk.remaining() == 0) {
        _chunks.pop_front();
    }
    return {count, receivedAtUs, false};
}

qint64 TimestampedByteBuffer::size() const
{
    const QMutexLocker lock(&_mutex);
    return _size;
}

bool TimestampedByteBuffer::gapPending() const
{
    const QMutexLocker lock(&_mutex);
    return _gap;
}
