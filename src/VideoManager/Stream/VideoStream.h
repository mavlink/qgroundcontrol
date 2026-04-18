#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoSink>
#include <QtQmlIntegration/QtQmlIntegration>
#include <functional>
#include <memory>
#include <optional>

#include "VideoRestartPolicy.h"
#include "VideoDiagnostics.h"
#include "VideoRecorder.h"
#include "VideoSourceResolver.h"
#include "VideoStreamFsmState.h"
#include "VideoStreamStats.h"

Q_DECLARE_LOGGING_CATEGORY(VideoStreamLog)

class QGCVideoStreamInfo;
class VideoFrameDelivery;
class VideoReceiverSession;
class VideoSettings;
class VideoStreamStateMachine;
#include "VideoReceiver.h"

/// Per-stream coordinator owning receiver lifecycle, URI resolution,
/// settings binding, and restart logic. VideoManager orchestrates a
/// list of these; each stream is independently testable.
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
    Q_PROPERTY(QObject* bridge READ bridgeAsObject NOTIFY bridgeChanged)
    Q_PROPERTY(QObject* frameDelivery READ frameDeliveryAsObject NOTIFY bridgeChanged)
    Q_PROPERTY(VideoStreamStats* stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged)
    Q_PROPERTY(bool firstFrameReady READ firstFrameReady NOTIFY firstFrameReadyChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QGCVideoStreamInfo* videoStreamInfo READ videoStreamInfo NOTIFY videoStreamInfoChanged)
    Q_PROPERTY(bool hwDecoding READ hwDecoding NOTIFY hwDecodingChanged)
    Q_PROPERTY(QString activeDecoderName READ activeDecoderName NOTIFY activeDecoderNameChanged)
    Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(int streamHealth READ streamHealth NOTIFY streamHealthChanged)
    Q_PROPERTY(float latencyMs READ latencyMs NOTIFY latencyMsChanged)
    Q_PROPERTY(quint64 droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
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

    [[nodiscard]] bool isThermal() const { return _role == Role::Thermal; }

    Q_INVOKABLE static QString nameForRole(Role role);
    static std::optional<Role> roleForName(const QString& name);

    static constexpr int roleCount() { return static_cast<int>(RoleCount); }

    // ── Receiver access ──────────────────────────────────────────────
    [[nodiscard]] VideoReceiver* receiver() const;

    // ── Recorder access ──────────────────────────────────────────────
    [[nodiscard]] VideoRecorder* recorder() const;

    /// Transfer ownership of the current recorder out of this stream. The
    /// stream immediately rebuilds a fresh recorder so `recorder()` stays
    /// non-null for QML bindings. Used by RecordingCoordinator to hand
    /// recorders to RecordingSession; the session destroys them at clean
    /// stop, so each recording cycle gets fresh state. Does not touch any
    /// recording intent — the coordinator owns recording policy.
    [[nodiscard]] std::unique_ptr<VideoRecorder> releaseRecorder();

    [[nodiscard]] VideoFrameDelivery* bridge() const;
    [[nodiscard]] VideoFrameDelivery* frameDelivery() const { return bridge(); }
    [[nodiscard]] QObject* bridgeAsObject() const;
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
    [[nodiscard]] VideoStreamStateMachine* fsm() const { return _fsm.get(); }

    [[nodiscard]] bool started() const;
    [[nodiscard]] bool streaming() const;
    [[nodiscard]] bool decoding() const;
    [[nodiscard]] bool firstFrameReady() const;
    [[nodiscard]] bool recording() const;

    [[nodiscard]] bool latencySupported() const;

    [[nodiscard]] QString uri() const { return _uri; }
    [[nodiscard]] const VideoSourceResolver::VideoSource& sourceDescriptor() const { return _source; }

    [[nodiscard]] QSize videoSize() const { return _videoSize; }

    // ── Stream info (MAVLink auto-stream) ────────────────────────────
    [[nodiscard]] QGCVideoStreamInfo* videoStreamInfo() const { return _streamInfo; }

    void setVideoStreamInfo(QGCVideoStreamInfo* info);

    // ── Sink management ──────────────────────────────────────────────
    /// Q_INVOKABLE so QGCVideoOutput.qml can call this directly to register
    /// its QVideoSink when the QML item is created / reparented.
    Q_INVOKABLE void registerVideoSink(QVideoSink* sink);
    /// Returns the QVideoSink currently registered on the stream's bridge,
    /// or nullptr if no sink has been registered yet. Preserved for test API.
    [[nodiscard]] QVideoSink* pendingSink() const;

    // ── URI + settings ───────────────────────────────────────────────
    bool updateFromSettings(VideoSettings* settings);
    bool setUri(const QString& uri);
    bool setSourceDescriptor(const VideoSourceResolver::VideoSource& source);

    // ── Lifecycle ────────────────────────────────────────────────────
    void start(uint32_t timeout);
    void stop();
    void restart();

    // ── Test support ─────────────────────────────────────────────────
    void setRecorderForTest(VideoRecorder* recorder);

    using RecorderFactory = std::function<VideoRecorder*(VideoStream*)>;
    void setRecorderFactoryForTest(RecorderFactory factory);

    // ── Stats ─────────────────────────────────────────────────────────
    [[nodiscard]] VideoStreamStats* stats() const { return _stats; }

    [[nodiscard]] float fps() const;
    [[nodiscard]] int streamHealth() const;
    [[nodiscard]] float latencyMs() const;
    [[nodiscard]] quint64 droppedFrames() const;
    [[nodiscard]] bool hwDecoding() const;
    [[nodiscard]] QString activeDecoderName() const;

    // ── Error reporting ──────────────────────────────────────────────
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] VideoReceiver::ErrorCategory lastErrorCategory() const;
    [[nodiscard]] VideoDiagnostics* diagnostics() const { return _diagnostics; }

signals:
    void sessionStateChanged(SessionState newState);
    /// Fired whenever the FSM transitions. Bound to
    /// VideoStreamStateMachine::stateChanged in `_ensureReceiver()`.
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
    void bridgeChanged();
    void recorderChanged();
    void receiverChanged();
    void fpsChanged(float fps);
    void streamHealthChanged(int health);
    void latencyMsChanged(float latencyMs);
    void droppedFramesChanged(quint64 dropped);
    void lastErrorChanged(const QString& lastError);
    void statsChanged();
    void hwDecodingChanged();
    void activeDecoderNameChanged();

private:
    uint32_t _timeoutForUri() const;
    void _ensureReceiver();
    void _destroyReceiver();
    void _createRecorder();
    void _destroyRecorder();
    void _stopReceiverIfStarted();

    /// Called when the receiver emits timeout(). Decides whether to do a
    /// RTSP PAUSED→PLAYING reconnect or a full restart, then delegates.
    void _onReceiverTimeout();

    bool _updateAutoStream(QString* outSource = nullptr);

    /// Map an FSM state to the coarser SessionState vocabulary.
    static SessionState _mapFsmState(VideoStreamFsm::State fsmState);

    void _connectStats(VideoFrameDelivery* delivery);
    void _wireSessionSignals();

    Role _role;
    QString _name;
    ReceiverFactory _factory;

    QGCVideoStreamInfo* _streamInfo = nullptr;
    VideoReceiverSession* _session = nullptr;
    VideoDiagnostics* _diagnostics = nullptr;

    VideoStreamStats* _stats = nullptr;

    QString _uri;
    VideoSourceResolver::VideoSource _source;
    QSize _videoSize;

    /// Reconnect/restart intent, backoff, and stale-callback filtering.
    VideoRestartPolicy _restartPolicy{this};

    /// Authoritative FSM. One-per-receiver lifetime: created in `_ensureReceiver()`,
    /// destroyed in `_destroyReceiver()`. `sessionState()` derives from its state.
    std::unique_ptr<VideoStreamStateMachine> _fsm;

    /// Last FSM-mapped session state — used to suppress duplicate sessionStateChanged
    /// emissions when the FSM transitions between states that map to the same
    /// SessionState (e.g. Connected → Streaming both map to Running).
    mutable SessionState _lastMappedState = SessionState::Stopped;
};
