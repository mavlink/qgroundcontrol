#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <atomic>
#include <cstdint>

#include "GPSTransport.h"

class GPSReplayClock
{
public:
    explicit GPSReplayClock(uint64_t* externalClock = nullptr) : _now(externalClock ? externalClock : &_owned) {}

    quint64 nowUs() const { return *_now; }

    qint64 nowMs() const { return static_cast<qint64>(*_now / 1000); }

    void advanceTo(quint64 timestamp) { *_now = qMax(*_now, timestamp); }

    void advanceBy(quint64 duration) { *_now += duration; }

private:
    uint64_t _owned = 1;
    uint64_t* _now;
};

struct GPSReplayEvent
{
    enum class Kind
    {
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
        Cancel
    };
    quint64 atUs = 0;
    Kind kind = Kind::Rx;
    QByteArray bytes = {};
    int value = 0;
};

struct GPSReplayTrace
{
    QVector<GPSReplayEvent> events;
    QJsonObject profile = {};
    quint64 streamId = 0;
    static bool fromJson(const QByteArray& json, GPSReplayTrace& result, QString& error, quint64 streamId = 0);
    static bool load(const QString& filename, GPSReplayTrace& result, QString& error, quint64 streamId = 0);
};

/// A trace owns byte order and arrival times; reads advance virtual time without sleeping.
class GPSReplayTransport : public GPSTransport
{
public:
    GPSReplayTransport(GPSReplayClock& clock, std::atomic_bool& requestStop, GPSReplayTrace trace,
                       int maximumRead = 4096);
    ~GPSReplayTransport() override;

    bool open() override;

    bool fatalError() const override { return _fatal; }

    int read(uint8_t* buffer, int length, int timeoutMs) override;
    int write(const uint8_t* buffer, int length) override;
    bool setBaudrate(unsigned baudrate) override;

    bool complete() const { return _index == _trace.events.size() && _failure.isEmpty(); }

    QString failure() const { return _failure; }

    qsizetype remainingEvents() const { return _trace.events.size() - _index; }

    quint64 lastReadTimestampUs() const { return _lastReadTimestampUs; }

    quint64 readCount() const { return _readCount; }

private:
    int _fail(const QString& message);
    GPSReplayClock& _clock;
    std::atomic_bool& _stop;
    GPSReplayTrace _trace;
    qsizetype _index = 0;
    qsizetype _offset = 0;
    int _maximumRead = 4096;
    QString _failure;
    quint64 _lastReadTimestampUs = 0;
    quint64 _readCount = 0;
    bool _opened = false;
    bool _fatal = false;
};
