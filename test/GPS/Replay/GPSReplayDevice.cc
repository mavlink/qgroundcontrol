#include "GPSReplayDevice.h"

#include <algorithm>
#include <cstring>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReplayDeviceLog, "GPS.Test.GPSReplayDevice")

GPSReplayDevice::GPSReplayDevice(RuntimeScheduler* scheduler, QObject* parent)
    : QIODevice(parent)
    , _scheduler(scheduler)
{
    qCDebug(GPSReplayDeviceLog) << this;
}

GPSReplayDevice::~GPSReplayDevice()
{
    qCDebug(GPSReplayDeviceLog) << this;
    stop();
}

void GPSReplayDevice::stop()
{
    ++_generation;
    if (_scheduler) {
        for (const auto id : _tasks) {
            _scheduler->cancel(id);
        }
    }
    _tasks.clear();
    _chunks.clear();
    QIODevice::close();
}

void GPSReplayDevice::play(const QVector<GPSRecordingEvent>& events)
{
    const QPointer<GPSReplayDevice> guard(this);
    const auto expectedGeneration = _generation + 1;
    stop();
    if (!guard || !_scheduler || _generation != expectedGeneration) {
        return;
    }
    const auto generation = _generation;
    _lifecycle = {};
    _originUs = _scheduler->nowUs();
    for (const auto& event : events) {
        if (event.receivedAtUs() && *event.receivedAtUs() <= 0) {
            _originUs = qMax(_originUs, static_cast<quint64>(1 - *event.receivedAtUs()));
        }
    }
    const auto delay = _originUs - _scheduler->nowUs();
    for (const auto& event : events) {
        _tasks.append(_scheduler->schedule(this, std::chrono::microseconds(delay + event.atUs),
                                           [this, event, generation]() { _apply(event, generation); }));
    }
    const auto lastEventUs = events.isEmpty() ? 0 : events.last().atUs;
    _tasks.append(
        _scheduler->schedule(this, std::chrono::microseconds(delay + lastEventUs), [this, generation, lastEventUs]() {
            if (_generation == generation && _lifecycle.exhausted(lastEventUs)) {
                _publishTermination(generation);
            }
        }));
}

void GPSReplayDevice::_publishTermination(quint64 generation)
{
    const auto result = *_lifecycle.termination();
    const QPointer<GPSReplayDevice> guard(this);
    if (result.reason != GPSReplayTermination::Reason::CaptureExhausted) {
        _chunks.clear();
        QIODevice::close();
        if (!guard || generation != _generation) {
            return;
        }
        if (result.readStatus != GPSReadStatus::Closed) {
            emit sessionError(result.readStatus);
            if (!guard || generation != _generation) {
                return;
            }
        }
        emit streamClosed();
        if (!guard || generation != _generation) {
            return;
        }
    }
    emit terminated(result);
}

void GPSReplayDevice::_apply(const GPSRecordingEvent& event, quint64 generation)
{
    if (_generation != generation) {
        return;
    }
    using K = GPSRecordingEvent::Kind;
    if (_lifecycle.consume(event)) {
        _publishTermination(generation);
        return;
    }
    if (_lifecycle.termination()) {
        return;
    }
    switch (event.kind()) {
        case K::Open:
            _chunks.clear();
            open(QIODevice::ReadOnly | QIODevice::Unbuffered);
            emit streamOpened();
            return;
        case K::Rx:
            if (!isOpen()) {
                emit sessionError(GPSReadStatus::Closed);
                return;
            }
            _chunks.push_back(
                {event.bytes(), static_cast<quint64>(static_cast<qint64>(_originUs) +
                                                     event.receivedAtUs().value_or(static_cast<qint64>(event.atUs)))});
            emit readyRead();
            return;
        default:
            return;
    }
}

qint64 GPSReplayDevice::bytesAvailable() const
{
    qint64 size = 0;
    for (const auto& chunk : _chunks) {
        size += chunk.bytes.size();
    }
    return size + QIODevice::bytesAvailable();
}

qint64 GPSReplayDevice::readData(char* data, qint64 maximum)
{
    if (_chunks.empty() || maximum <= 0) {
        return 0;
    }
    auto& chunk = _chunks.front();
    const auto count = std::min(maximum, qint64(chunk.bytes.size()));
    std::memcpy(data, chunk.bytes.constData(), static_cast<size_t>(count));
    _lastReceiptUs = chunk.receiptUs;
    chunk.bytes.remove(0, count);
    if (chunk.bytes.isEmpty()) {
        _chunks.pop_front();
    }
    return count;
}
