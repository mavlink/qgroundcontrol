#pragma once

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include <deque>

#include "GPSReadTimestamp.h"
#include "GPSRecordingFormat.h"
#include "GPSReplayLifecycle.h"
#include "GPSRuntimeScheduler.h"

/// Event-loop replay preserves chunk receipts while the production session owns parsing and publication.
class GPSReplayDevice : public QIODevice, public GPSReadTimestamp
{
    Q_OBJECT
public:
    explicit GPSReplayDevice(GPSRuntimeScheduler* scheduler, QObject* parent = nullptr);
    ~GPSReplayDevice() override;
    void play(const QVector<GPSRecordingEvent>& events);
    void stop();

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override;

    quint64 lastReadTimestampUs() const override { return _lastReceiptUs; }

    quint64 timeOriginUs() const { return _originUs; }

    const std::optional<GPSReplayTermination>& termination() const { return _lifecycle.termination(); }

signals:
    void streamOpened();
    void streamClosed();
    void sessionError(GPSReadStatus status);
    void terminated(const GPSReplayTermination& result);

protected:
    qint64 readData(char* data, qint64 maximum) override;

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    void _apply(const GPSRecordingEvent& event, quint64 generation);
    void _publishTermination(quint64 generation);

    struct Chunk
    {
        QByteArray bytes;
        quint64 receiptUs = 0;
    };

    GPSReplayLifecycle _lifecycle;
    QPointer<GPSRuntimeScheduler> _scheduler;
    QVector<GPSRuntimeScheduler::TaskId> _tasks;
    std::deque<Chunk> _chunks;
    quint64 _generation = 0;
    quint64 _originUs = 0;
    quint64 _lastReceiptUs = 0;
};
