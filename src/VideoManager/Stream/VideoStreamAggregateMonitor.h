#pragma once

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>

class VideoStream;

/// Tracks aggregate state for non-thermal streams.
class VideoStreamAggregateMonitor : public QObject
{
    Q_OBJECT

public:
    explicit VideoStreamAggregateMonitor(QObject* parent = nullptr);

    void watch(VideoStream* stream);
    void unwatch(VideoStream* stream);

    [[nodiscard]] bool decoding() const { return _state.decoding; }
    [[nodiscard]] bool recording() const { return _state.recording; }

signals:
    void decodingChanged();
    void recordingChanged(bool recording);

private:
    struct AggregateState
    {
        bool decoding = false;
        bool recording = false;
    };

    [[nodiscard]] AggregateState _compute() const;
    void _recompute();

    QList<VideoStream*> _streams;
    QHash<VideoStream*, QList<QMetaObject::Connection>> _connections;
    AggregateState _state;
};
