#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <atomic>
#include <cstdint>

#include "GPSRecordingFormat.h"
#include "GPSReplayLifecycle.h"
#include "GPSTransport.h"

class GPSReplayClock
{
public:
    explicit GPSReplayClock(uint64_t* externalClock = nullptr)
        : _now(externalClock ? externalClock : &_owned)
    {}

    quint64 nowUs() const { return *_now; }

    qint64 nowMs() const { return static_cast<qint64>(*_now / 1000); }

    void advanceTo(quint64 timestamp) { *_now = qMax(*_now, timestamp); }

    void advanceBy(quint64 duration) { *_now += duration; }

private:
    uint64_t _owned = 1;
    uint64_t* _now;
};

using GPSReplayEvent = GPSRecordingEvent;

struct GPSReplayTrace
{
    QVector<GPSReplayEvent> events;
    std::optional<GPSRecordingMetadata> profile = {};
    QVector<GPSRecordingEvent> recordedEvents = {};
    bool limitReached = false;
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

    OpenResult open() override;

    bool fatalError() const override { return _fatal; }

    ReadResult read(uint8_t* buffer, int length, int timeoutMs) override;
    WriteResult write(const uint8_t* buffer, int length) override;
    WriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;
    WriteResult writeUntil(const uint8_t* buffer, int length, GPSDeadline deadline,
                           const GPSExecutionContext& context) override;
    GPSExecutionContext executionContext();
    std::chrono::milliseconds correctionWriteTimeout(int length) const override;

    unsigned fixedBaudrate() const override { return _trace.profile ? _trace.profile->fixedBaud : 0; }

    GPSReplayClock& clock() { return _clock; }

    const GPSReplayTrace& trace() const { return _trace; }

    bool setBaudrate(unsigned baudrate) override;

    bool complete() const { return _index == _trace.events.size() && _failure.isEmpty(); }

    QString failure() const { return _failure; }

    qsizetype remainingEvents() const { return _trace.events.size() - _index; }

    quint64 lastReadTimestampUs() const { return _lastReadTimestampUs; }

    quint64 readCount() const { return _readCount; }

    const std::optional<GPSReplayTermination>& termination() const { return _lifecycle.termination(); }

    quint64 terminationCount() const { return _lifecycle.terminationCount(); }

private:
    int _writeLegacy(const uint8_t* buffer, int length);

    quint64 _eventTime(quint64 atUs) const { return _originUs + atUs; }

    quint64 _originUs = 0;
    int _fail(const QString& message);
    GPSReplayClock& _clock;
    std::atomic_bool& _stop;
    GPSReplayTrace _trace;
    GPSReplayLifecycle _lifecycle;
    qsizetype _index = 0;
    qsizetype _offset = 0;
    int _maximumRead = 4096;
    unsigned _baudrate = 0;
    QString _failure;
    quint64 _lastReadTimestampUs = 0;
    quint64 _readCount = 0;
    bool _opened = false;
    bool _fatal = false;
};
