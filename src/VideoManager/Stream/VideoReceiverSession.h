#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtMultimedia/QVideoSink>
#include <functional>
#include <memory>

#include "VideoReceiver.h"
#include "VideoSourceResolver.h"

class VideoFrameDelivery;
class VideoRecorder;
class VideoRecorderSessionController;
class VideoStream;
class VideoSinkRouter;

/// Owns the concrete receiver session for one VideoStream.
///
/// VideoStream remains the public/QML-facing coordinator and owns policy state
/// such as restart intent and the FSM. This object owns the lower-level session
/// resources: stream bridge, active receiver, draining receivers, sink routing,
/// and recorder instance.
class VideoReceiverSession : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoReceiverSession)

public:
    using ReceiverFactory = std::function<VideoReceiver*(const VideoSourceResolver::VideoSource& source,
                                                         bool thermal,
                                                         QObject* parent)>;
    using RecorderFactory = std::function<VideoRecorder*(VideoStream*)>;

    struct EnsureResult
    {
        bool created = false;
        bool hadPendingSink = false;
    };

    struct SinkResult
    {
        bool bridgeChanged = false;
        bool restartRequested = false;
    };

    VideoReceiverSession(QString name, bool thermal, ReceiverFactory factory, VideoStream* owner);
    ~VideoReceiverSession() override;

    [[nodiscard]] VideoReceiver* receiver() const { return _receiver; }
    [[nodiscard]] VideoFrameDelivery* bridge() const;
    [[nodiscard]] VideoRecorder* recorder() const;
    [[nodiscard]] QVideoSink* videoSink() const;

    [[nodiscard]] bool requiresReceiverRecreate(const VideoSourceResolver::VideoSource& source) const;
    [[nodiscard]] EnsureResult ensureReceiver(const VideoSourceResolver::VideoSource& source);
    [[nodiscard]] bool destroyReceiver();
    [[nodiscard]] SinkResult registerVideoSink(QVideoSink* sink);

    void createRecorder();
    void destroyRecorder();
    [[nodiscard]] std::unique_ptr<VideoRecorder> releaseRecorder();
    void setRecorderForTest(VideoRecorder* recorder);
    void setRecorderFactoryForTest(RecorderFactory factory);

signals:
    void streamingChanged(bool active);
    void decodingChanged(bool active);
    void receiverError(VideoReceiver::ErrorCategory category, const QString& message);
    void timeout();
    void lateSinkRestartRequested();

    void recordingStarted(const QString& path);
    void recordingChanged(bool active);
    void recordingError(const QString& message);
    void recorderChanged();

private:
    void _wireReceiverSignals();
    void _disconnectReceiver();
    void _detachReceiverBridge();
    void _invokeOnReceiverThread(const std::function<void()>& fn);

    QString _name;
    bool _thermal = false;
    ReceiverFactory _factory;
    VideoStream* _owner = nullptr;
    VideoSourceResolver::VideoSource _source;

    VideoReceiver* _receiver = nullptr;
    VideoSinkRouter* _sinkRouter = nullptr;
    VideoRecorderSessionController* _recorderController = nullptr;
    QList<VideoReceiver*> _drainingReceivers;
    QList<QMetaObject::Connection> _receiverConns;
};
