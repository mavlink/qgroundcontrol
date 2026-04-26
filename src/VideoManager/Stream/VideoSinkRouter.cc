#include "VideoSinkRouter.h"

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"
#include "VideoReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(VideoReceiverSessionLog)

VideoSinkRouter::VideoSinkRouter(QString streamName, QObject* parent)
    : QObject(parent)
    , _streamName(std::move(streamName))
    , _delivery(new VideoFrameDelivery(this))
{
}

QVideoSink* VideoSinkRouter::videoSink() const
{
    return _delivery ? _delivery->videoSink() : nullptr;
}

bool VideoSinkRouter::attachReceiver(VideoReceiver* receiver, const ReceiverInvoker& invokeOnReceiverThread)
{
    if (!receiver)
        return false;

    receiver->setFrameDelivery(_delivery);
    const bool hadPendingSink = _delivery && _delivery->videoSink();
    if (hadPendingSink) {
        QVideoSink* sink = _delivery->videoSink();
        invokeOnReceiverThread([receiver, sink]() {
            [[maybe_unused]] const VideoReceiver::SinkChangeAction action = receiver->onSinkChanged(sink);
        });
    }
    return hadPendingSink;
}

void VideoSinkRouter::detachReceiver(VideoReceiver* receiver, const ReceiverInvoker& invokeOnReceiverThread)
{
    if (!receiver)
        return;

    invokeOnReceiverThread([receiver]() { receiver->onSinkAboutToChange(); });
    receiver->setFrameDelivery(nullptr);
}

VideoSinkRouter::SinkResult VideoSinkRouter::registerVideoSink(VideoReceiver* receiver,
                                                               QVideoSink* sink,
                                                               const ReceiverInvoker& invokeOnReceiverThread)
{
    SinkResult result;
    if (!_delivery)
        return result;

    if (_delivery->videoSink() == sink)
        return result;

    const SinkChange change = _prepareSinkChange(receiver, sink);
    const VideoReceiver::SinkChangeAction action = _commitSinkChange(receiver, change, invokeOnReceiverThread);
    return _finishSinkChange(receiver, change, action, invokeOnReceiverThread);
}

VideoSinkRouter::SinkChange VideoSinkRouter::_prepareSinkChange(VideoReceiver* receiver, QVideoSink* sink) const
{
    SinkChange change;
    change.oldSink = _delivery ? _delivery->videoSink() : nullptr;
    change.newSink = sink;
    change.hasReceiver = receiver != nullptr;
    change.replacingLiveSink = sink && change.oldSink && change.oldSink != sink;
    return change;
}

VideoReceiver::SinkChangeAction VideoSinkRouter::_commitSinkChange(VideoReceiver* receiver,
                                                                   const SinkChange& change,
                                                                   const ReceiverInvoker& invokeOnReceiverThread)
{
    if (change.replacingLiveSink) {
        qCWarning(VideoReceiverSessionLog) << _streamName << "replacing live QVideoSink (" << change.oldSink << "->"
                                           << change.newSink << ") - check for QML binding leaks";
    }

    VideoReceiver::SinkChangeAction action = VideoReceiver::SinkChangeAction::NoAction;
    if (receiver)
        invokeOnReceiverThread([receiver]() { receiver->onSinkAboutToChange(); });
    _delivery->setVideoSink(change.newSink);
    if (receiver) {
        QVideoSink* sink = change.newSink;
        invokeOnReceiverThread([receiver, sink, &action]() { action = receiver->onSinkChanged(sink); });
    }
    return action;
}

VideoSinkRouter::SinkResult VideoSinkRouter::_finishSinkChange(VideoReceiver* receiver,
                                                               const SinkChange& change,
                                                               VideoReceiver::SinkChangeAction action,
                                                               const ReceiverInvoker& invokeOnReceiverThread)
{
    SinkResult result;
    if (!change.hasReceiver || !receiver)
        return result;

    if (action == VideoReceiver::SinkChangeAction::RestartRequired) {
        result.restartRequested = true;
    } else if (change.newSink && receiver->started() && !receiver->isDecoding()) {
        invokeOnReceiverThread([receiver]() { receiver->startDecoding(); });
    }

    result.frameDeliveryChanged = true;
    return result;
}
