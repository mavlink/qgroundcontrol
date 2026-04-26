#include "VideoStreamAggregateMonitor.h"

#include "VideoStream.h"

VideoStreamAggregateMonitor::VideoStreamAggregateMonitor(QObject* parent)
    : QObject(parent)
{
}

void VideoStreamAggregateMonitor::watch(VideoStream* stream)
{
    if (!stream || _streams.contains(stream))
        return;

    _streams.append(stream);
    _connections.insert(stream, {
        connect(stream, &VideoStream::decodingChanged, this, [this](bool) { _recompute(); }),
        connect(stream, &VideoStream::recordingChanged, this, [this](bool) { _recompute(); }),
        connect(stream, &QObject::destroyed, this, [this, stream]() { unwatch(stream); }),
    });

    _recompute();
}

void VideoStreamAggregateMonitor::unwatch(VideoStream* stream)
{
    if (!stream)
        return;

    const auto connections = _connections.take(stream);
    for (const auto& connection : connections)
        disconnect(connection);
    _streams.removeAll(stream);

    _recompute();
}

VideoStreamAggregateMonitor::AggregateState VideoStreamAggregateMonitor::_compute() const
{
    AggregateState state;
    for (const auto* stream : _streams) {
        if (!stream || stream->isThermal())
            continue;
        state.decoding |= stream->decoding();
        state.recording |= stream->recording();
    }
    return state;
}

void VideoStreamAggregateMonitor::_recompute()
{
    const AggregateState state = _compute();

    if (state.decoding != _state.decoding) {
        _state.decoding = state.decoding;
        emit decodingChanged();
    }
    if (state.recording != _state.recording) {
        _state.recording = state.recording;
        emit recordingChanged(state.recording);
    }
}
