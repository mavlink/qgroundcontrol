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
class VideoRecordingService;
class VideoStream;
class VideoSinkRouter;

/// Owns lower-level receiver resources for one stream: frame delivery, active
/// receiver, draining receivers, sink routing, and recorder instance.
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
        bool frameDeliveryChanged = false;
        bool restartRequested = false;
    };

    VideoReceiverSession(QString name,
                         bool thermal,
                         ReceiverFactory factory,
                         VideoStream* owner,
                         QObject* parent = nullptr);
    ~VideoReceiverSession() override;

    [[nodiscard]] VideoReceiver* receiver() const { return _receiver; }
    [[nodiscard]] VideoFrameDelivery* frameDelivery() const;
    [[nodiscard]] VideoRecorder* recorder() const;
    [[nodiscard]] QVideoSink* videoSink() const;

    [[nodiscard]] bool requiresReceiverRecreate(const VideoSourceResolver::VideoSource& source) const;
    [[nodiscard]] EnsureResult ensureReceiver(const VideoSourceResolver::VideoSource& source);
    [[nodiscard]] bool destroyReceiver();
    [[nodiscard]] SinkResult registerVideoSink(QVideoSink* sink);

    void createRecorder(std::unique_ptr<VideoRecorder> preferredRecorder = {});
    void destroyRecorder();
    [[nodiscard]] std::unique_ptr<VideoRecorder> releaseRecorder();
    void setRecorderForTest(VideoRecorder* recorder);
    void setRecorderFactoryForTest(RecorderFactory factory);

signals:
    void streamingChanged(bool active);
    void decodingChanged(bool active);
    void receiverError(VideoReceiver::ErrorCategory category, const QString& message);
    void timeout();

    void recordingStarted(const QString& path);
    void recordingChanged(bool active);
    void recordingError(const QString& message);
    void recorderChanged();

private:
    void _wireReceiverSignals();
    void _disconnectReceiver();
    void _detachReceiverFrameDelivery();
    void _invokeOnReceiverThread(const std::function<void()>& fn);

    QString _name;
    bool _thermal = false;
    ReceiverFactory _factory;
    VideoStream* _owner = nullptr;
    VideoSourceResolver::VideoSource _source;

    VideoReceiver* _receiver = nullptr;
    VideoSinkRouter* _sinkRouter = nullptr;
    VideoRecordingService* _recorderController = nullptr;
    QList<VideoReceiver*> _drainingReceivers;
    QList<QMetaObject::Connection> _receiverConns;
};
