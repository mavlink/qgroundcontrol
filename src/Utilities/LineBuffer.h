#pragma once

#include <QtCore/QByteArray>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE
class QRingBuffer;
QT_END_NAMESPACE

/// Line buffer shared by line-readable QIODevice adapters (UdpIODevice,
/// NmeaPipeDevice). Backed by private QRingBuffer for O(1) drain.
class LineBuffer
{
    Q_DISABLE_COPY(LineBuffer)

public:
    LineBuffer();
    ~LineBuffer();

    LineBuffer(LineBuffer &&) noexcept;
    LineBuffer &operator=(LineBuffer &&) noexcept;

    /// If `maxSize > 0` and the append overflows, drops oldest bytes first.
    /// Returns the number of dropped bytes (0 on no-overflow).
    qsizetype append(const char *data, qint64 len, qsizetype maxSize = 0);
    qsizetype append(const QByteArray &data, qsizetype maxSize = 0);

    bool     canReadLine() const;
    qint64   size() const;
    bool     isEmpty() const;
    void     clear();

    /// QIODevice::readLineData: 0 if no newline, else copies up to '\n' (incl). No null terminator.
    qint64 readLine(char *out, qint64 maxSize);
    /// QIODevice::readData drain.
    qint64 read(char *out, qint64 maxSize);

    qsizetype lastDropped() const { return _lastDropped; }

private:
    qsizetype _trim(qsizetype maxSize);

    QRingBuffer *_buf = nullptr;   // pimpl: keeps qringbuffer_p.h out of header
    qsizetype    _lastDropped = 0;
};
