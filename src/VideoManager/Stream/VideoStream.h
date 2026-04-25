#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoSink>
#include <QtQmlIntegration/QtQmlIntegration>
#include <functional>
#include <memory>
#include <optional>

#include "VideoDiagnostics.h"
#include "VideoRecorder.h"
#include "VideoSourceResolver.h"
#include "VideoStreamFsmState.h"
#include "VideoStreamStats.h"

Q_DECLARE_LOGGING_CATEGORY(VideoStreamLog)

class QGCVideoStreamInfo;
class VideoFrameDelivery;
class VideoStreamSession;
class VideoStreamLifecycleController;
class VideoStreamStateMachine;
#include "VideoReceiver.h"
#include "VideoPlaybackInput.h"

/// QML-facing identity and control surface for one video stream.
class VideoStream : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Role role READ role CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(VideoReceiver::ErrorCategory lastErrorCategory READ lastErrorCategory NOTIFY lastErrorChanged)
    Q_PROPERTY(VideoDiagnostics* diagnostics READ diagnostics CONSTANT)
    Q_PROPERTY(SessionState sessionState READ sessionState NOTIFY sessionStateChanged)
    Q_PROPERTY(VideoStreamFsm::State fsmState READ fsmState NOTIFY fsmStateChanged)
    Q_PROPERTY(QObject* frameDelivery READ frameDeliveryAsObject NOTIFY frameDeliveryChanged)
    Q_PROPERTY(VideoStreamStats* stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged)
    Q_PROPERTY(bool firstFrameReady READ firstFrameReady NOTIFY firstFrameReadyChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QGCVideoStreamInfo* videoStreamInfo READ videoStreamInfo NOTIFY videoStreamInfoChanged)
    Q_PROPERTY(qreal sourceAspectRatio READ sourceAspectRatio NOTIFY streamInfoUpdated)
    Q_PROPERTY(bool sourceIsThermal READ sourceIsThermal NOTIFY streamInfoUpdated)
    Q_PROPERTY(quint16 sourceHfov READ sourceHfov NOTIFY streamInfoUpdated)
    Q_PROPERTY(bool hwDecoding READ hwDecoding NOTIFY hwDecodingChanged)
    Q_PROPERTY(QString activeDecoderName READ activeDecoderName NOTIFY activeDecoderNameChanged)
    Q_PROPERTY(VideoRecorder* recorder READ recorder NOTIFY recorderChanged)
    Q_PROPERTY(bool latencySupported READ latencySupported NOTIFY receiverChanged)
    QML_UNCREATABLE("")

public:
    /// Stream identity — compile-time safe dispatch, replaces string comparisons.
    enum class Role : quint8
    {
        Primary,
        Thermal,
        UVC,
        Dynamic,  ///< Vehicle-reported stream (MAVLink VIDEO_STREAM_INFORMATION)
    };
    Q_ENUM(Role)
    static constexpr size_t RoleCount = static_cast<size_t>(Role::Dynamic) + 1;

    /// Lifecycle state — alias of VideoReceiver::ReceiverState so existing QML
    /// and test code referencing VideoStream::SessionState continues to compile.
    using SessionState = VideoReceiver::ReceiverState;

    /// Factory function type for creating receivers.
    using ReceiverFactory = std::function<VideoReceiver*(const VideoSourceResolver::VideoSource& source,
                                                         bool thermal,
                                                         QObject* parent)>;

    explicit VideoStream(Role role, ReceiverFactory factory, QObject* parent = nullptr);
    VideoStream(Role role, const QString& name, ReceiverFactory factory, QObject* parent = nullptr);
    ~VideoStream() override;

    // ── Identity ─────────────────────────────────────────────────────
    [[nodiscard]] Role role() const { return _role; }

    [[nodiscard]] QString name() const { return _name; }

    [[nodiscard]] std::optional<quint8> metadataStreamId() const { return _metadataStreamId; }
    void setMetadataStreamId(std::optional<quint8> streamId);
    [[nodiscard]] std::optional<VideoSourceResolver::StreamInfo> sourceMetadata() const { return _sourceMetadata; }
    void setSourceMetadata(const VideoSourceResolver::StreamInfo& metadata);
    void clearSourceMetadata();
    [[nodiscard]] qreal sourceAspectRatio() const;
    [[nodiscard]] bool sourceIsThermal() const;
    [[nodiscard]] quint16 sourceHfov() const;

    [[nodiscard]] bool isThermal() const { return _role == Role::Thermal; }

    Q_INVOKABLE static QString nameForRole(Role role);
    static std::optional<Role> roleForName(const QString& name);

    static constexpr int roleCount() { return static_cast<int>(RoleCount); }

    [[nodiscard]] VideoStreamSession* session() const { return _session; }
    [[nodiscard]] VideoStreamLifecycleController* lifecycle() const;

    // ── Receiver access ──────────────────────────────────────────────
    [[nodiscard]] VideoReceiver* receiver() const;

    // ── Recorder access ──────────────────────────────────────────────
    [[nodiscard]] VideoRecorder* recorder() const;

    /// Transfer ownership of the current recorder out of this stream and
    /// immediately rebuild a fresh recorder so `recorder()` stays non-null
    /// for QML bindings.
    [[nodiscard]] std::unique_ptr<VideoRecorder> releaseRecorder();

    [[nodiscard]] VideoFrameDelivery* frameDelivery() const;
    [[nodiscard]] QObject* frameDeliveryAsObject() const;

    // ── State queries ────────────────────────────────────────────────
    /// Derives the session state from the FSM's current state using the
    /// canonical mapping:
    ///   FSM {Idle, Failed} → Stopped
    ///   FSM {Starting}     → Starting
    ///   FSM {Connected, Streaming, Paused, Reconnecting} → Running
    ///   FSM {Stopping}     → Stopping
    /// Returns Stopped when no FSM exists (pre-receiver, post-teardown).
    [[nodiscard]] SessionState sessionState() const;

    /// QML-friendly projection of the FSM's current state. Returns `Idle` when
    /// no FSM exists (pre-receiver, post-teardown). Use the `fsmStateChanged`
    /// NOTIFY for bindings — `fsm` itself is intentionally not a NOTIFY
    /// property so QML can't accidentally hold a dangling pointer across
    /// receiver swaps.
    [[nodiscard]] VideoStreamFsm::State fsmState() const;

    /// Raw FSM accessor (for tests).
    [[nodiscard]] VideoStreamStateMachine* fsm() const;

    [[nodiscard]] bool started() const;
    [[nodiscard]] bool streaming() const;
    [[nodiscard]] bool decoding() const;
    [[nodiscard]] bool firstFrameReady() const;
    [[nodiscard]] bool recording() const;

    [[nodiscard]] bool latencySupported() const;

    [[nodiscard]] QString uri() const;
    [[nodiscard]] const VideoSourceResolver::VideoSource& sourceDescriptor() const;

    [[nodiscard]] QSize videoSize() const;

    // ── Stream info (MAVLink auto-stream) ────────────────────────────
    [[nodiscard]] QGCVideoStreamInfo* videoStreamInfo() const;

    void setVideoStreamInfo(QGCVideoStreamInfo* info);

    // ── Sink management ──────────────────────────────────────────────
    /// Q_INVOKABLE for tests and QML helpers that own sink binding lifecycle.
    Q_INVOKABLE void registerVideoSink(QVideoSink* sink);
    [[nodiscard]] QVideoSink* videoSink() const;

    // ── URI + settings ───────────────────────────────────────────────
    bool setUri(const QString& uri);
    bool setSourceDescriptor(const VideoSourceResolver::VideoSource& source);
    bool setLowLatency(bool lowLatency);

    // ── Lifecycle ────────────────────────────────────────────────────
    void start(uint32_t timeout);
    void stop();
    void restart();

    // ── Test support ─────────────────────────────────────────────────
    void setRecorderForTest(VideoRecorder* recorder);

    using RecorderFactory = std::function<VideoRecorder*(VideoStream*)>;
    void setRecorderFactoryForTest(RecorderFactory factory);

    using PlaybackInput = VideoPlaybackInput;

    using PlaybackInputResolver = std::function<PlaybackInput(const VideoSourceResolver::VideoSource& source)>;
    void setPlaybackInputResolverForTest(PlaybackInputResolver resolver);

    // ── Stats ─────────────────────────────────────────────────────────
    [[nodiscard]] VideoStreamStats* stats() const;

    [[nodiscard]] bool hwDecoding() const;
    [[nodiscard]] QString activeDecoderName() const;

    // ── Error reporting ──────────────────────────────────────────────
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] VideoReceiver::ErrorCategory lastErrorCategory() const;
    [[nodiscard]] VideoDiagnostics* diagnostics() const;

signals:
    void sessionStateChanged(SessionState newState);
    /// Fired whenever the stream lifecycle FSM transitions.
    void fsmStateChanged(VideoStreamFsm::State newState);
    void streamingChanged(bool active);
    void decodingChanged(bool active);
    void firstFrameReadyChanged(bool ready);
    void recordingChanged(bool active);
    void recordingStarted(const QString& filename);
    void recordingError(const QString& errorString);
    void videoSizeChanged(QSize size);
    void videoStreamInfoChanged();
    void streamInfoUpdated();
    void uriChanged(const QString& uri);
    void frameDeliveryChanged();
    void recorderChanged();
    void receiverChanged();
    void lastErrorChanged(const QString& lastError);
    void statsChanged();
    void hwDecodingChanged();
    void activeDecoderNameChanged();

private:
    void _initSession();
    void _wireSessionSignals();

    Role _role;
    QString _name;
    std::optional<quint8> _metadataStreamId;
    ReceiverFactory _factory;

    QPointer<QGCVideoStreamInfo> _videoStreamInfo;
    QMetaObject::Connection _videoStreamInfoConnection;
    std::optional<VideoSourceResolver::StreamInfo> _sourceMetadata;
    VideoStreamSession* _session = nullptr;
};
