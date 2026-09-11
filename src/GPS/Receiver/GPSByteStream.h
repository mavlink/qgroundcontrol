#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QMutex>

#include <deque>
#include <memory>

#include "GPSReadTimestamp.h"

/// Bounded mailbox shared by the blocking receiver worker and its Qt decoder stream.
class GPSByteBuffer
{
public:
    bool append(const QByteArray& bytes, quint64 receivedAtUs = GPSObservation::monotonicNowUs());
    qint64 read(char* data, qint64 length, quint64& receivedAtUs);
    qint64 size() const;

private:
    mutable QMutex _mutex;

    struct Chunk
    {
        QByteArray bytes;
        quint64 receivedAtUs = 0;
        qsizetype offset = 0;

        qsizetype remaining() const { return bytes.size() - offset; }
    };

    std::deque<Chunk> _chunks;
    qsizetype _size = 0;
    bool _gap = false;
    static constexpr qsizetype kCapacity = 64 * 1024 - 1;
    static constexpr size_t kMaxChunks = 1024;
};

/// Main-thread read-only stream; the worker only touches the shared byte buffer.
class GPSByteStream : public QIODevice, public GPSReadTimestamp
{
    Q_OBJECT

public:
    explicit GPSByteStream(QObject* parent = nullptr);
    ~GPSByteStream() override;

    std::shared_ptr<GPSByteBuffer> buffer() const { return _buffer; }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override;

    quint64 lastReadTimestampUs() const override { return _lastReadTimestampUs; }

public slots:
    void notifyReadyRead();

protected:
    qint64 readData(char* data, qint64 length) override;

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    std::shared_ptr<GPSByteBuffer> _buffer;
    quint64 _lastReadTimestampUs = 0;
};
