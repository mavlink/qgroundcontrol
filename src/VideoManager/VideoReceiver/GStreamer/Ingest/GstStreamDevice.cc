#include "GstStreamDevice.h"

#include <algorithm>
#include <cstring>

GstStreamDevice::GstStreamDevice(QObject* parent)
    : QIODevice(parent)
{
    open(QIODevice::ReadOnly);
}

GstStreamDevice::~GstStreamDevice()
{
    finishStream();
}

bool GstStreamDevice::atEnd() const
{
    QMutexLocker locker(&_mutex);
    return _finished && _buffer.isEmpty();
}

qint64 GstStreamDevice::bytesAvailable() const
{
    QMutexLocker locker(&_mutex);
    return static_cast<qint64>(_buffer.size()) + QIODevice::bytesAvailable();
}

void GstStreamDevice::close()
{
    // This device is handed to QtMultimedia/FFmpeg, which may be blocked in a
    // read on its worker thread. Calling QIODevice::close() concurrently with
    // that read mutates QIODevice's internal buffer state; for this sequential
    // EOF path we only need to wake readers and make future reads return 0.
    finishStream();
}

void GstStreamDevice::resetStream()
{
    QMutexLocker locker(&_mutex);
    _buffer.clear();
    _finished = false;
    if (!isOpen())
        open(QIODevice::ReadOnly);
    _hasSpace.wakeAll();
}

void GstStreamDevice::finishStream()
{
    QMutexLocker locker(&_mutex);
    _finished = true;
    _hasData.wakeAll();
    _hasSpace.wakeAll();
}

bool GstStreamDevice::append(const char* data, qint64 size)
{
    if (!data || size <= 0)
        return true;

    QMutexLocker locker(&_mutex);
    while (!_finished && _buffer.size() >= kMaxBufferedBytes)
        _hasSpace.wait(&_mutex);

    if (_finished)
        return false;

    _buffer.append(data, size);
    _hasData.wakeAll();
    return true;
}

qint64 GstStreamDevice::readData(char* data, qint64 maxSize)
{
    if (!data || maxSize <= 0)
        return 0;

    QMutexLocker locker(&_mutex);
    while (!_finished && _buffer.isEmpty())
        _hasData.wait(&_mutex);

    if (_buffer.isEmpty())
        return 0;

    const qint64 bytesToRead = std::min<qint64>(maxSize, _buffer.size());
    std::memcpy(data, _buffer.constData(), static_cast<size_t>(bytesToRead));
    _buffer.remove(0, static_cast<qsizetype>(bytesToRead));
    _hasSpace.wakeAll();
    return bytesToRead;
}

qint64 GstStreamDevice::writeData(const char* data, qint64 maxSize)
{
    return append(data, maxSize) ? maxSize : -1;
}
