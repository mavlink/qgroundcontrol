#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtMultimedia/QVideoSink>
#include <functional>

#include "VideoReceiver.h"

class VideoFrameDelivery;

/// Owns stream sink routing and the stable frame-delivery endpoint.
class VideoSinkRouter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoSinkRouter)

public:
    using ReceiverInvoker = std::function<void(const std::function<void()>&)>;

    struct SinkResult
    {
        bool frameDeliveryChanged = false;
        bool restartRequested = false;
    };

    explicit VideoSinkRouter(QString streamName, QObject* parent = nullptr);

    [[nodiscard]] VideoFrameDelivery* frameDelivery() const { return _delivery; }
    [[nodiscard]] QVideoSink* videoSink() const;

    [[nodiscard]] bool attachReceiver(VideoReceiver* receiver, const ReceiverInvoker& invokeOnReceiverThread);
    void detachReceiver(VideoReceiver* receiver, const ReceiverInvoker& invokeOnReceiverThread);

    [[nodiscard]] SinkResult registerVideoSink(VideoReceiver* receiver,
                                               QVideoSink* sink,
                                               const ReceiverInvoker& invokeOnReceiverThread);

private:
    struct SinkChange
    {
        QVideoSink* oldSink = nullptr;
        QVideoSink* newSink = nullptr;
        bool hasReceiver = false;
        bool replacingLiveSink = false;
    };

    [[nodiscard]] SinkChange _prepareSinkChange(VideoReceiver* receiver, QVideoSink* sink) const;
    [[nodiscard]] VideoReceiver::SinkChangeAction _commitSinkChange(VideoReceiver* receiver,
                                                                    const SinkChange& change,
                                                                    const ReceiverInvoker& invokeOnReceiverThread);
    [[nodiscard]] SinkResult _finishSinkChange(VideoReceiver* receiver,
                                               const SinkChange& change,
                                               VideoReceiver::SinkChangeAction action,
                                               const ReceiverInvoker& invokeOnReceiverThread);

    QString _streamName;
    VideoFrameDelivery* _delivery = nullptr;
};
