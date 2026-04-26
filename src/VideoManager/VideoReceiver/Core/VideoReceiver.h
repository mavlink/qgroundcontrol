#pragma once

#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QVideoSink>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoFrameDelivery.h"

class QIODevice;
namespace VideoSourceResolver {
struct VideoSource;
}

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
        CapRecording = 0x04,          ///< Can record to file
        CapLocalCamera = 0x10,        ///< Supports local camera capture (UVC)
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)
    Q_FLAG(Capabilities)

    /// Coarse session-state vocabulary surfaced to QML. VideoStream derives this
    /// from the FSM through VideoStreamLifecyclePolicy; VideoReceiver itself no
    /// longer tracks state — primitive `receiverStarted/Stopped/...` signals are
    /// its only lifecycle output. `VideoStream::SessionState` aliases this so
    /// existing QML/test code compiles unchanged.
    enum class ReceiverState : quint8
    {
        Stopped,   ///< No receiver running
        Starting,  ///< Receiver requested, not yet confirmed started
        Running,   ///< Receiver started and decoding
        Stopping,  ///< Stop requested, teardown in progress
    };
    Q_ENUM(ReceiverState)

    enum class SinkChangeAction : quint8
    {
        NoAction,
        RestartRequired,
    };
    Q_ENUM(SinkChangeAction)

    struct PlaybackPolicy
    {
        bool lowLatencyStreaming = false;
        int probeSizeBytes = 0;
    };

    [[nodiscard]] virtual Capabilities capabilities() const = 0;

    /// Returns true if this receiver provides latency measurements via frame delivery.
    [[nodiscard]] virtual bool latencySupported() const { return true; }

    /// Configure receiver-specific typed source metadata. URI/device playback
    /// input is still applied separately through setUri()/setSourceDevice().
    virtual void configureSource(const VideoSourceResolver::VideoSource& source);

    /// Active decoder info. QtMultimedia populates `activeDecoderName` from
    /// QMediaPlayer codec metadata. `hwDecoding` remains false where the
    /// receiver exposes no HW-vs-SW accessor.
    [[nodiscard]] bool hwDecoding() const { return _hwDecoding; }

    [[nodiscard]] const QString& activeDecoderName() const { return _activeDecoderName; }

    [[nodiscard]] QString uri() const { return _uri; }
    [[nodiscard]] QIODevice* sourceDevice() const { return _sourceDevice; }
    [[nodiscard]] QUrl sourceDeviceUrl() const { return _sourceDeviceUrl; }
    [[nodiscard]] PlaybackPolicy playbackPolicy() const { return _playbackPolicy; }

    [[nodiscard]] bool started() const { return _started; }

    [[nodiscard]] bool lowLatency() const { return _lowLatency; }

    /// Canonical state accessors. Each receiver overrides these to report its
    /// authoritative source (pipeline state / QMediaPlayer state / QCamera state).
    /// Default returns false for receivers that do not track lifecycle internally.
    [[nodiscard]] virtual bool isStreaming() const { return false; }

    [[nodiscard]] virtual bool isDecoding() const { return false; }

    void setUri(const QString& uri)
    {
        if (uri != _uri) {
            _uri = uri;
            emit uriChanged(_uri);
        }
    }

    virtual void setSourceDevice(QIODevice* device, const QUrl& sourceUrl = {})
    {
        _sourceDevice = device;
        _sourceDeviceUrl = sourceUrl;
    }

    virtual void setPlaybackPolicy(const PlaybackPolicy& policy)
    {
        _playbackPolicy = policy;
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

    /// Notify the receiver that the video sink is about to change.
    /// Called by VideoStream when routing a new QVideoSink through frame delivery.
    virtual void onSinkAboutToChange() {}

    /// Notify the receiver that the video sink has changed.
    /// Called by VideoStream after updating the frame-delivery sink pointer.
    [[nodiscard]] virtual SinkChangeAction onSinkChanged(QVideoSink* newSink)
    {
        Q_UNUSED(newSink);
        return SinkChangeAction::NoAction;
    }

    /// Unified error classification across ingest session and Qt Multimedia receivers.
    enum class ErrorCategory : quint8
    {
        Transient,         ///< Recoverable — stream controller may retry (socket hiccup)
        Fatal,             ///< Unrecoverable — pipeline/playback must stop
        MissingPlugin,     ///< GStreamer plug-in unavailable for codec/container
    };
    Q_ENUM(ErrorCategory)

signals:
    void timeout();
    void streamingChanged(bool active);
    void decodingChanged(bool active);

    /// Unified error channel. Receivers emit here; VideoStream forwards to QML.
    void receiverError(VideoReceiver::ErrorCategory category, const QString& message);

    void uriChanged(const QString& uri);
    void startedChanged(bool started);
    void lowLatencyChanged(bool lowLatency);

    /// Primitive lifecycle signals — the FSM's sole input vocabulary.
    /// Receivers emit these on genuine lifecycle transitions; `receiverError`
    /// with `ErrorCategory::Fatal` is the terminal-failure primitive.
    void receiverStarted();
    void receiverStopped();
    void receiverPaused();
    void receiverResumed();
    void receiverFirstFrame();

    /// Emitted when stream metadata (codec, resolution, framerate) is updated.
    /// QtMultimedia forwards QMediaPlayer::metaDataChanged. VideoStream exposes this as
    /// a Q_PROPERTY for QML and tooling use.
    void receiverMetaDataChanged(const QMediaMetaData& metaData);

public slots:
    virtual void start(uint32_t timeout) = 0;
    virtual void stop() = 0;
    virtual void startDecoding() = 0;
    virtual void stopDecoding() = 0;

    /// Optional pause/resume for reconnect cycles (Phase 4).
    /// Default implementations are no-ops; receivers override as needed.
    virtual void pause() {}
    virtual void resume() {}

protected:
    bool validateFrameDeliveryForDecoding();
    void resetFrameDeliveryStats();

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
    QString _uri;
    QIODevice* _sourceDevice = nullptr;
    QUrl _sourceDeviceUrl;
    PlaybackPolicy _playbackPolicy;
    QString _activeDecoderName;
    bool _started = false;
    bool _lowLatency = false;
    bool _hwDecoding = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VideoReceiver::Capabilities)
