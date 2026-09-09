#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QVector>

#include <atomic>
#include <functional>
#include <memory>

#include "GPSRecordingFormat.h"

/// Receiver threads append bounded value events; no queued payloads or disk I/O cross the thread boundary.
class GPSRecordingBuffer
{
public:
    using Kind = GPSRecordingEvent::Kind;

    struct Status
    {
        bool recording = false;
        bool limitReached = false;
        qsizetype eventCount = 0;
        qint64 bytesRecorded = 0;
        bool operator==(const Status&) const = default;
    };

    using Clock = std::function<quint64()>;

    explicit GPSRecordingBuffer(Clock clock = {});
    ~GPSRecordingBuffer();
    bool start();
    void stop();
    Status status() const;
    QByteArray exportJson() const;
    quint64 nowUs() const;
    quint64 allocateStream();
    void append(quint64 stream, const GPSRecordingMetadata& metadata, bool alreadyOpen, Kind kind,
                QByteArrayView bytes = {}, int value = 0, quint64 startedAtUs = 0,
                std::optional<GPSTransport::WriteResult> writeResult = {}, bool fatal = false);

    static constexpr qsizetype MAX_EVENTS = 10000;
    static constexpr qsizetype MAX_STORAGE_BYTES = 2 * 1024 * 1024;

private:
    using Event = GPSRecordingEvent;

    Clock _clock;
    mutable QMutex _mutex;
    std::atomic_bool _recording = false;
    std::atomic<quint64> _nextStream = 1;
    QVector<Event> _events;
    QSet<quint64> _seenStreams;
    Status _status;
    quint64 _originUs = 0;
    qsizetype _storageBytes = 0;
};

/// Confined to one receiver I/O thread; the shared buffer outlives the UI controller if a worker retires late.
class GPSRecordingStream
{
public:
    GPSRecordingStream(std::shared_ptr<GPSRecordingBuffer> buffer, GPSRecordingMetadata metadata);
    ~GPSRecordingStream();
    quint64 nowUs() const;
    void opened(bool success, quint64 startedAtUs = 0);
    void closed(int reason = 0);

    bool isOpen() const { return _opened; }

    void record(GPSRecordingBuffer::Kind kind, QByteArrayView bytes = {}, int value = 0, quint64 startedAtUs = 0);
    void recordWrite(QByteArrayView bytes, GPSTransport::WriteResult result, quint64 startedAtUs, bool fatal);
    void configurationStarted();
    void configurationFinished(int status);

private:
    std::shared_ptr<GPSRecordingBuffer> _buffer;
    GPSRecordingMetadata _metadata;
    quint64 _id = 0;
    bool _opened = false;
};
