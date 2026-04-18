#pragma once

#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QThread>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QVideoSink>
#include <QtQmlIntegration/QtQmlIntegration>
#include <utility>

#include "VideoFrameDelivery.h"

class QGCVideoStreamInfo;
class VideoRecorder;

class VideoReceiver : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    explicit VideoReceiver(QObject* parent = nullptr) : QObject(parent) {}

    /// Receiver capabilities — used to query feature support at runtime.
    enum Capability
    {
        CapStreaming = 0x01,          ///< Can receive network streams (RTSP, UDP, TCP)
        CapHWDecode = 0x02,           ///< Supports hardware-accelerated decoding
        CapRecording = 0x04,          ///< Can record to file
        CapLowLatency = 0x08,         ///< Supports low-latency mode
        CapLocalCamera = 0x10,        ///< Supports local camera capture (UVC)
        CapVulkanDecode = 0x20,       ///< Supports Vulkan Video decode (VK_KHR_video_decode_queue)
        CapRecordingLossless = 0x40,  ///< Supports lossless (no re-encode) recording via GStreamer tee
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)
    Q_FLAG(Capabilities)

    /// Backend identity — use instead of CapGStreamer flag for backend discrimination.
    /// Override `kind()` in each backend subclass.
    enum class BackendKind : quint8
    {
        GStreamer,     ///< GStreamer pipeline backend
        QtMultimedia,  ///< Qt Multimedia (FFmpeg) backend
        UVC,           ///< Local camera via QMediaCaptureSession
    };
    Q_ENUM(BackendKind)

    /// Coarse session-state vocabulary surfaced to QML. VideoStream derives this
    /// from the FSM via `_mapFsmState`; VideoReceiver itself no longer tracks
    /// state — primitive `receiverStarted/Stopped/...` signals are its only
    /// lifecycle output. `VideoStream::SessionState` aliases this so existing
    /// QML/test code compiles unchanged.
    enum class ReceiverState : quint8
    {
        Stopped,   ///< No receiver running
        Starting,  ///< Receiver requested, not yet confirmed started
        Running,   ///< Receiver started and decoding
        Stopping,  ///< Stop requested, teardown in progress
    };
    Q_ENUM(ReceiverState)

    [[nodiscard]] virtual Capabilities capabilities() const = 0;

    /// Returns the backend kind for type discrimination without capability flags.
    [[nodiscard]] virtual BackendKind kind() const = 0;

    /// Returns true if this receiver provides latency measurements via the bridge.
    [[nodiscard]] virtual bool latencySupported() const { return true; }

    /// Create the recorder implementation best suited for this backend.
    /// Default is QMediaRecorder fed from VideoFrameDelivery; backends with a
    /// better native path (for example GStreamer tee/remux) override this.
    [[nodiscard]] virtual VideoRecorder* createRecorder(VideoFrameDelivery* delivery, QObject* parent);

    /// Video stream statistics - delegated to frame delivery.
    [[nodiscard]] quint64 totalFrames() const { return _delivery ? _delivery->frameCount() : 0; }

    [[nodiscard]] quint64 droppedFrames() const { return _delivery ? _delivery->droppedFrames() : 0; }

    /// Active decoder info. Both backends populate `activeDecoderName` (GStreamer
    /// from decoder-pad caps; QtMultimedia from QMediaPlayer codec metadata).
    /// `hwDecoding` is meaningful only for GStreamer — QMediaPlayer exposes no
    /// HW-vs-SW accessor, so QtMultimediaReceiver always reports false.
    [[nodiscard]] bool hwDecoding() const { return _hwDecoding; }

    [[nodiscard]] const QString& activeDecoderName() const { return _activeDecoderName; }

    bool isThermal() const { return _thermal; }

    void setThermal(bool thermal) { _thermal = thermal; }

    [[nodiscard]] QString name() const { return _name; }

    [[nodiscard]] QString uri() const { return _uri; }

    [[nodiscard]] bool started() const { return _started; }

    [[nodiscard]] bool lowLatency() const { return _lowLatency; }

    /// Canonical state accessors. Each backend overrides these to report its
    /// authoritative source (pipeline state / QMediaPlayer state / QCamera state).
    /// Default returns false for backends that do not track pipeline lifecycle.
    [[nodiscard]] virtual bool isStreaming() const { return false; }

    [[nodiscard]] virtual bool isDecoding() const { return false; }

    void setName(const QString& name)
    {
        if (name != _name) {
            _name = name;
            emit nameChanged(_name);
        }
    }

    void setUri(const QString& uri)
    {
        if (uri != _uri) {
            _uri = uri;
            emit uriChanged(_uri);
        }
    }

    void setStarted(bool started)
    {
        if (started != _started) {
            _started = started;
            emit startedChanged(_started);
        }
    }

    virtual void setLowLatency(bool lowLatency)
    {
        if (lowLatency != _lowLatency) {
            _lowLatency = lowLatency;
            emit lowLatencyChanged(_lowLatency);
        }
    }

    /// Attach (or detach) the stream-owned VideoFrameDelivery to this receiver.
    /// Must be called before start(). The pointer is non-owning — lifetime is
    /// managed by VideoStream. Safe to call from the main thread before the
    /// worker thread starts processing (happens-before the first QMetaObject
    /// dispatch inside start()).
    void setFrameDelivery(VideoFrameDelivery* delivery) { _delivery = delivery; }

    /// Access frame delivery (non-owning; set by VideoStream via setFrameDelivery).
    [[nodiscard]] VideoFrameDelivery* frameDelivery() const { return _delivery; }

    /// Notify the backend that the video sink is about to change.
    /// Called by VideoStream when routing a new QVideoSink through the bridge.
    virtual void onSinkAboutToChange() {}

    /// Notify the backend that the video sink has changed.
    /// Called by VideoStream after updating the bridge's sink pointer.
    virtual void onSinkChanged(QVideoSink* newSink) { Q_UNUSED(newSink); }

    /// Unified error classification across GStreamer and QtMultimedia backends.
    enum class ErrorCategory : quint8
    {
        Transient,         ///< Recoverable — stream controller may retry (socket hiccup)
        Fatal,             ///< Unrecoverable — pipeline/playback must stop
        MissingPlugin,     ///< GStreamer plug-in unavailable for codec/container
        HardwareFallback,  ///< HW decoder failed; controller attempted SW fallback
    };
    Q_ENUM(ErrorCategory)

    enum STATUS
    {
        STATUS_MIN = 0,
        STATUS_OK = STATUS_MIN,
        STATUS_FAIL,
        STATUS_INVALID_STATE,
        STATUS_INVALID_URL,
        STATUS_NOT_IMPLEMENTED,
        STATUS_MAX = STATUS_NOT_IMPLEMENTED
    };
    Q_ENUM(STATUS)

    static bool isValidStatus(STATUS status) { return ((status >= STATUS_MIN) && (status <= STATUS_MAX)); }

signals:
    void timeout();
    void streamingChanged(bool active);
    void decodingChanged(bool active);

    /// Unified error channel. Backends emit here; VideoStream forwards to QML.
    void receiverError(VideoReceiver::ErrorCategory category, const QString& message);

    void nameChanged(const QString& name);
    void uriChanged(const QString& uri);
    void startedChanged(bool started);
    void lowLatencyChanged(bool lowLatency);

    /// Primitive lifecycle signals — the FSM's sole input vocabulary.
    /// Backends emit these on genuine pipeline transitions; `receiverError`
    /// with `ErrorCategory::Fatal` is the terminal-failure primitive.
    void receiverStarted();
    void receiverStopped();
    void receiverPaused();
    void receiverResumed();
    void receiverFirstFrame();

    /// Emitted when stream metadata (codec, resolution, framerate) is updated.
    /// GStreamer backend emits after caps negotiation; QtMultimedia backend
    /// forwards QMediaPlayer::metaDataChanged. VideoStream exposes this as
    /// a Q_PROPERTY for QML and tooling use.
    void receiverMetaDataChanged(const QMediaMetaData& metaData);

public slots:
    virtual void start(uint32_t timeout) = 0;
    virtual void stop() = 0;
    virtual void startDecoding() = 0;
    virtual void stopDecoding() = 0;

    /// Optional pause/resume for reconnect cycles (Phase 4).
    /// Default implementations are no-ops; backends override as needed.
    virtual void pause() {}
    virtual void resume() {}

protected:


    /// Re-invoke `fn` on this QObject's thread if the caller isn't there
    /// already. Returns true when a hop was dispatched (the caller should
    /// return immediately); false when the caller may proceed inline.
    /// Kept in the header so the templated lambda inlines cleanly.
    template <typename Fn>
    bool _hopToOwningThread(Fn&& fn)
    {
        if (thread() == QThread::currentThread())
            return false;
        QMetaObject::invokeMethod(this, std::forward<Fn>(fn), Qt::QueuedConnection);
        return true;
    }

    bool validateBridgeForDecoding();
    void resetBridgeStats();

    /// Update decoder identity and (optionally) publish a full QMediaMetaData update.
    /// Callers that have the richer metadata handy pass it here to collapse the
    /// two-step "mutate state + emit metaData" dance into a single call.
    void setDecoderInfo(bool hwDecoding, const QString& decoderName, const QMediaMetaData& metaData = {})
    {
        _hwDecoding = hwDecoding;
        _activeDecoderName = decoderName;
        if (!metaData.isEmpty())
            emit receiverMetaDataChanged(metaData);
    }

    VideoFrameDelivery* _delivery = nullptr;
    QString _name;
    QString _uri;
    QString _activeDecoderName;
    bool _started = false;
    bool _lowLatency = false;
    bool _thermal = false;
    bool _hwDecoding = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VideoReceiver::Capabilities)
