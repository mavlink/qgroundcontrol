#include "LineBuffer.h"

#include <QtCore/private/qringbuffer_p.h>

LineBuffer::LineBuffer()
    : _buf(new QRingBuffer())
{
}

LineBuffer::~LineBuffer()
{
    delete _buf;
}

LineBuffer::LineBuffer(LineBuffer &&other) noexcept
    : _buf(other._buf)
    , _lastDropped(other._lastDropped)
{
    other._buf = nullptr;
    other._lastDropped = 0;
}

LineBuffer &LineBuffer::operator=(LineBuffer &&other) noexcept
{
    if (this != &other) {
        delete _buf;
        _buf = other._buf;
        _lastDropped = other._lastDropped;
        other._buf = nullptr;
        other._lastDropped = 0;
    }
    return *this;
}

qsizetype LineBuffer::append(const char *data, qint64 len, qsizetype maxSize)
{
    _lastDropped = 0;
    if (maxSize > 0 && _buf->size() + len > maxSize) {
        // If len > maxSize, drop is capped at current contents (rare).
        const qint64 want = _buf->size() + len - maxSize;
        const qint64 drop = qMin<qint64>(want, _buf->size());
        _buf->free(drop);
        _lastDropped = static_cast<qsizetype>(drop);
    }
    _buf->append(data, len);
    return _lastDropped;
}

qsizetype LineBuffer::append(const QByteArray &data, qsizetype maxSize)
{
    return append(data.constData(), data.size(), maxSize);
}

bool   LineBuffer::canReadLine() const { return _buf->canReadLine(); }
qint64 LineBuffer::size()        const { return _buf->size(); }
bool   LineBuffer::isEmpty()     const { return _buf->isEmpty(); }
void   LineBuffer::clear()             { _buf->clear(); _lastDropped = 0; }

qint64 LineBuffer::readLine(char *out, qint64 maxSize)
{
    // QIODevice::readLineData contract: no null terminator.
    return _buf->readLineWithoutTerminatingNull(out, maxSize);
}

qint64 LineBuffer::read(char *out, qint64 maxSize)
{
    return _buf->read(out, maxSize);
}
