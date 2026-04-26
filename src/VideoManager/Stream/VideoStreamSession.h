#pragma once

#include <functional>
#include <memory>

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoSink>

#include "VideoDiagnostics.h"
#include "VideoPlaybackRuntime.h"
#include "VideoReceiver.h"
#include "VideoReceiverSession.h"
#include "VideoRestartPolicy.h"
#include "VideoSourceResolver.h"
#include "VideoStream.h"
#include "VideoStreamFsmState.h"
#include "VideoStreamStats.h"

class VideoFrameDelivery;
class VideoRecorder;
class VideoStreamLifecycleController;
class VideoStreamStateMachine;

Q_DECLARE_LOGGING_CATEGORY(VideoStreamSessionLog)

/// Owns the runtime session for one VideoStream: receiver resources, sink
/// routing, recorder creation, ingest proxying, lifecycle FSM, diagnostics,
/// and restart policy. VideoStream remains the QML-facing identity facade.
class VideoStreamSession : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoStreamSession)

public:
    using SessionState = VideoReceiver::ReceiverState;
    using PlaybackInput = VideoPlaybackRuntime::PlaybackInput;
    using PlaybackInputResolver = VideoPlaybackRuntime::Resolver;
    using ReceiverFactory = VideoStream::ReceiverFactory;
    using RecorderFactory = VideoStream::RecorderFactory;

    VideoStreamSession(QString name,
                       VideoStream::Role role,
                       ReceiverFactory factory,
                       VideoStream* owner,
                       QObject* parent = nullptr);
    ~VideoStreamSession() override;

    [[nodiscard]] VideoReceiverSession* receiverResources() const { return _receiverResources; }
    [[nodiscard]] VideoDiagnostics* diagnostics() const { return _diagnostics; }
    [[nodiscard]] VideoStreamLifecycleController* lifecycle() const { return _lifecycle; }
    [[nodiscard]] VideoStreamStats* stats() const { return _stats; }

    [[nodiscard]] VideoFrameDelivery* frameDelivery() const;
    [[nodiscard]] QObject* frameDeliveryAsObject() const;
    [[nodiscard]] VideoReceiver* receiver() const;
    [[nodiscard]] VideoRecorder* recorder() const;
    [[nodiscard]] QVideoSink* videoSink() const;

    [[nodiscard]] SessionState sessionState() const;
    [[nodiscard]] VideoStreamFsm::State fsmState() const;
    [[nodiscard]] VideoStreamStateMachine* fsm() const;

    [[nodiscard]] bool started() const;
    [[nodiscard]] bool streaming() const;
    [[nodiscard]] bool decoding() const;
    [[nodiscard]] bool firstFrameReady() const;
    [[nodiscard]] bool recording() const;
    [[nodiscard]] bool latencySupported() const;
    [[nodiscard]] bool hwDecoding() const;
    [[nodiscard]] QString activeDecoderName() const;

    [[nodiscard]] const QString& uri() const { return _uri; }
    [[nodiscard]] const VideoSourceResolver::VideoSource& sourceDescriptor() const { return _source; }
    [[nodiscard]] QSize videoSize() const { return _videoSize; }
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] VideoReceiver::ErrorCategory lastErrorCategory() const;

    bool setUri(const QString& uri);
    bool setSourceDescriptor(const VideoSourceResolver::VideoSource& source);
    bool setLowLatency(bool lowLatency);

    void registerVideoSink(QVideoSink* sink);
    void start(uint32_t timeout);
    void stop();
    void restart();

    [[nodiscard]] std::unique_ptr<VideoRecorder> releaseRecorder();
    void setRecorderForTest(VideoRecorder* recorder);
    void setRecorderFactoryForTest(RecorderFactory factory);
    void setPlaybackInputResolverForTest(PlaybackInputResolver resolver);

signals:
    void sessionStateChanged(VideoReceiver::ReceiverState newState);
    void fsmStateChanged(VideoStreamFsm::State newState);
    void streamingChanged(bool active);
    void decodingChanged(bool active);
    void firstFrameReadyChanged(bool ready);
    void recordingChanged(bool active);
    void recordingStarted(const QString& filename);
    void recordingError(const QString& errorString);
    void videoSizeChanged(QSize size);
    void uriChanged(const QString& uri);
    void frameDeliveryChanged();
    void recorderChanged();
    void receiverChanged();
    void lastErrorChanged(const QString& lastError);
    void statsChanged();
    void hwDecodingChanged();
    void activeDecoderNameChanged();

private:
    uint32_t _timeoutForUri() const;
    void _ensureReceiver();
    void _destroyReceiver();
    void _createRecorder();
    void _stopReceiverIfStarted();
    [[nodiscard]] bool _refreshPlaybackInputForStart();
    [[nodiscard]] bool _hasPlaybackInput() const;
    void _applyPlaybackInputToReceiver();
    void _stopIngestSession();
    void _onReceiverTimeout();
    [[nodiscard]] bool _hasReceiver() const { return receiver() != nullptr; }
    [[nodiscard]] bool _hasSource() const { return _source.isValid(); }
    [[nodiscard]] bool _isStopped() const { return sessionState() == SessionState::Stopped; }
    [[nodiscard]] bool _isStoppingOrStopped() const;
    void _connectStats(VideoFrameDelivery* delivery);
    void _wireSignals();

    QString _name;
    VideoStream::Role _role = VideoStream::Role::Primary;
    ReceiverFactory _factory;

    VideoReceiverSession* _receiverResources = nullptr;
    VideoDiagnostics* _diagnostics = nullptr;
    VideoStreamStats* _stats = nullptr;
    VideoPlaybackRuntime* _playbackRuntime = nullptr;
    VideoStreamLifecycleController* _lifecycle = nullptr;

    QString _uri;
    VideoSourceResolver::VideoSource _source;
    QSize _videoSize;
    VideoRestartPolicy _restartPolicy{this};
};
