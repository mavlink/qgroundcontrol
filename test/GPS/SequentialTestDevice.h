#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include <algorithm>
#include <deque>

#include "ReadTimestamp.h"
#include "RuntimeScheduler.h"

class SequentialTestDevice : public QIODevice, public ReadTimestamp
{
public:
    explicit SequentialTestDevice(RuntimeScheduler* scheduler = nullptr) : _scheduler(scheduler)
    {
        open(ReadOnly | Unbuffered);
    }

    quint64 receivedAtUs = 0;

    quint64 lastReadTimestampUs() const override { return _lastReadTimestampUs; }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return _size + QIODevice::bytesAvailable(); }

    bool canReadLine() const override
    {
        return QIODevice::canReadLine() || std::any_of(_chunks.cbegin(), _chunks.cend(), [](const Chunk& chunk) {
                   return chunk.bytes.indexOf('\n', chunk.offset) >= 0;
               });
    }

    void feed(const QByteArray& data, bool notify = true)
    {
        if (!data.isEmpty()) {
            const auto timestamp =
                receivedAtUs ? receivedAtUs : (_scheduler ? _scheduler->nowUs() : ReadTimestamp::nowUs());
            _chunks.push_back({data, 0, timestamp});
            _size += data.size();
        }
        if (notify) {
            emit readyRead();
        }
    }

    void close() override
    {
        _chunks.clear();
        _size = 0;
        _lastReadTimestampUs = 0;
        QIODevice::close();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        if (_chunks.empty() || maxSize <= 0)
            return 0;
        auto& chunk = _chunks.front();
        const auto count = std::min<qint64>(maxSize, chunk.bytes.size() - chunk.offset);
        std::copy_n(chunk.bytes.constData() + chunk.offset, count, data);
        _lastReadTimestampUs = chunk.timestampUs;
        chunk.offset += count;
        _size -= count;
        if (chunk.offset == chunk.bytes.size())
            _chunks.pop_front();
        return count;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    struct Chunk
    {
        QByteArray bytes;
        qsizetype offset;
        quint64 timestampUs;
    };

    QPointer<RuntimeScheduler> _scheduler;
    std::deque<Chunk> _chunks;
    qint64 _size = 0;
    quint64 _lastReadTimestampUs = 0;
};
