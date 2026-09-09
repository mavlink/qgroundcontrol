#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QVector>

#include <atomic>
#include <functional>
#include <memory>

#include "GPSDriver.h"

/// Allowlisted receiver intent only; addresses, device names and credentials are never metadata.
struct GPSRecordingMetadata
{
    enum class Transport
    {
        Unknown,
        Serial,
        Tcp,
        Udp
    };
    Transport transport = Transport::Unknown;
    GPSReceiverConfig receiver{.role = GPSReceiverConfig::Role::Position,
                               .outputProtocol = GPSReceiverConfig::OutputProtocol::Native,
                               .base = {}};
    int driverType = -1;
    int initialBaud = 0;
    bool configured = false;

    static GPSRecordingMetadata forReceiver(const GPSReceiverConfig& config, GPSType type);
};

/// Receiver threads append bounded value events; no queued payloads or disk I/O cross the thread boundary.
class GPSRecordingBuffer
{
public:
    enum class Kind
    {
        Session,
        Open,
        OpenError,
        Rx,
        Tx,
        Baud,
        BaudError,
        Timeout,
        ReadError,
        WriteError,
        Disconnect,
        Cancel,
        Close,
        ConfigurationStarted,
        ConfigurationFinished
    };

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
                QByteArrayView bytes = {}, int value = 0, quint64 startedAtUs = 0);

    static constexpr qsizetype MAX_EVENTS = 10000;
    static constexpr qsizetype MAX_STORAGE_BYTES = 2 * 1024 * 1024;

private:
    struct Event
    {
        quint64 atUs = 0;
        quint64 startedAtUs = 0;
        quint64 stream = 0;
        Kind kind = Kind::Rx;
        QByteArray bytes;
        int value = 0;
        bool resumed = false;
        GPSRecordingMetadata metadata;
    };

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
    void configurationStarted();
    void configurationFinished(int status);

private:
    std::shared_ptr<GPSRecordingBuffer> _buffer;
    GPSRecordingMetadata _metadata;
    quint64 _id = 0;
    bool _opened = false;
};
