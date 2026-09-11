#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QMutex>

#include <deque>
#include <memory>

#include "ReadTimestamp.h"
#include "TimestampedByteBuffer.h"

/// Main-thread read-only stream; the worker only touches the shared byte buffer.
class GPSByteStream : public QIODevice, public ReadTimestamp
{
    Q_OBJECT

public:
    explicit GPSByteStream(QObject* parent = nullptr);
    ~GPSByteStream() override;

    std::shared_ptr<TimestampedByteBuffer> buffer() const { return _buffer; }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override;

    quint64 lastReadTimestampUs() const override { return _lastReadTimestampUs; }

public slots:
    void notifyReadyRead();

protected:
    qint64 readData(char* data, qint64 length) override;

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    std::shared_ptr<TimestampedByteBuffer> _buffer;
    quint64 _lastReadTimestampUs = 0;
};
