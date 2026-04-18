#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtGui/QColor>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "VideoReceiver.h"
#include "VideoStream.h"

/// Test double for VideoReceiver — synthesizes the full receiver lifecycle
/// without GStreamer or QMediaPlayer.
///
/// Usage patterns:
///   * **Synchronous (default)** — start/stop/decoding emit their primitive lifecycle
///     signals (receiverStarted/Stopped/…) immediately on the calling stack. Good for FSM transition tests.
///   * **Async** — set `asyncDelayMs > 0` to defer primitive signal emissions via
///     QTimer::singleShot. Lets tests observe transient states (Starting,
///     Stopping) and validates VideoStream's stop-drain logic.
///   * **Failure injection** — `failNext{Start,Stop,StartDecoding,...}` flags
///     are consumed once per matching call.
///   * **Capabilities** — set `capabilityFlags` before constructing.
///
/// Recording is no longer handled by receivers. Tests that exercise recording
/// should verify state via VideoStream::recording() and VideoRecorder signals.
///
/// The class is intentionally header-only-ish (single .cc) so tests can
/// include it directly without pulling extra build deps.
class FakeVideoReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit FakeVideoReceiver(QObject* parent = nullptr);

    /// Convenience constructor — sets CapStreaming and (optionally) BackendKind::GStreamer.
    explicit FakeVideoReceiver(bool gstreamer, QObject* parent = nullptr);

    [[nodiscard]] Capabilities capabilities() const override { return _capabilities; }

    [[nodiscard]] BackendKind kind() const override { return _backendKind; }

    [[nodiscard]] bool isStreaming() const override { return _streamingActive; }

    [[nodiscard]] bool isDecoding() const override { return _decoderActive; }

    // ── Configuration (set BEFORE start/stop calls) ─────────────────────

    /// Override capabilities. Defaults to CapStreaming for the parameterless ctor.
    void setCapabilities(Capabilities caps) { _capabilities = caps; }

    /// Override backend kind. Defaults to BackendKind::QtMultimedia.
    void setBackendKind(BackendKind k) { _backendKind = k; }

    /// Defer *Completed signals by this many ms via QTimer::singleShot.
    /// 0 = synchronous (signals fire on the calling stack).
    void setAsyncDelayMs(int ms) { _asyncDelayMs = ms; }

    /// Make the next start() emit receiverError(Fatal) instead of receiverStarted.
    bool failNextStart = false;
    bool failNextStop = false;
    bool failNextStartDecoding = false;
    bool failNextStopDecoding = false;

    // ── Test introspection ─────────────────────────────────────────────

    int startCallCount = 0;
    int stopCallCount = 0;
    int startDecodingCallCount = 0;
    int stopDecodingCallCount = 0;

    // ── Test-side drivers ───────────────────────────────────────────────

    /// Synthesize a receiver-level error (network drop, decoder failure, etc.).
    void emitReceiverError(ErrorCategory category, const QString& message);

    /// Drive the receiver into a "decoding active" state without going through
    /// startDecoding() — useful for testing downstream consumers.
    void forceDecoding(bool decoding)
    {
        if (_decoderActive == decoding)
            return;
        _decoderActive = decoding;
        emit decodingChanged(decoding);
    }

    /// Drive the receiver into a "streaming active" state.
    void forceStreaming(bool streaming)
    {
        if (_streamingActive == streaming)
            return;
        _streamingActive = streaming;
        emit streamingChanged(streaming);
    }

    /// Synthesize the first-frame primitive (VideoStreamStateMachine uses this
    /// to transition Connected → Streaming).
    void emitFirstFrame() { emit receiverFirstFrame(); }
    void emitReceiverStopped() { emit receiverStopped(); }

    // ── Headless frame delivery (drives VideoFrameDelivery in CI) ─────────

    /// Build a valid QVideoFrame of the given size filled with a solid colour.
    /// Zero-copy-ish: allocates one QImage, wraps it in a QVideoFrame. Exposed
    /// so tests can compose custom sequences (e.g. varying sizes to exercise
    /// videoSize-changed paths).
    static QVideoFrame makeSyntheticFrame(QSize size,
                                          QVideoFrameFormat::PixelFormat fmt = QVideoFrameFormat::Format_RGBA8888,
                                          QColor fill = Qt::black);

    /// Announce a frame format on the owning VideoStream's bridge. Lets tests
    /// exercise the pre-first-frame videoSize path (`announceFormat` updates
    /// `bridge->videoSize()` without any frame having been delivered).
    /// No-op if frame delivery hasn't been wired yet.
    void announceFormat(QSize size,
                        QVideoFrameFormat::PixelFormat fmt = QVideoFrameFormat::Format_RGBA8888);

    /// Synthesize and push one frame through `frameDelivery()->deliverFrame()`.
    /// Returns true if delivery accepted the frame (delivery wired + frame
    /// valid), false otherwise.
    bool deliverSyntheticFrame(QSize size = QSize(1280, 720));

    /// Burst-deliver `count` frames back-to-back from the current thread.
    /// Useful for exercising delivery backpressure / drop semantics. Returns
    /// the number of `deliverFrame` calls issued (not the number that
    /// survived the backpressure gate).
    int deliverSyntheticFrames(int count, QSize size = QSize(1280, 720));

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding() override;
    void stopDecoding() override;
    void pause() override;
    void resume() override;

private:
    /// Emit a *Completed signal — synchronously if asyncDelayMs == 0,
    /// otherwise via QTimer::singleShot.
    template <typename Emitter>
    void _emitMaybeAsync(Emitter&& emitter);

    /// Pop a "fail next" flag — returns true if the flag was set.
    static bool _consumeFlag(bool& flag);

    Capabilities _capabilities;
    BackendKind _backendKind = BackendKind::QtMultimedia;
    int _asyncDelayMs = 0;
    bool _streamingActive = false;
    bool _decoderActive = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers — small, stateless conveniences for tests using FakeVideoReceiver.
// ─────────────────────────────────────────────────────────────────────────────

namespace FakeReceiverHelpers {

/// Build a VideoStream::ReceiverFactory that creates FakeVideoReceiver
/// instances. The most-recently-created receiver is captured via outReceiver
/// (raw pointer; lifetime owned by VideoStream).
///
/// configurer: optional callback invoked on each new receiver before it is
/// returned, so tests can preset asyncDelayMs / failure flags / capabilities.
inline VideoStream::ReceiverFactory makeFactory(FakeVideoReceiver** outReceiver = nullptr, bool gstreamer = false,
                                                std::function<void(FakeVideoReceiver*)> configurer = {})
{
    return [outReceiver, gstreamer, configurer = std::move(configurer)](
               const VideoSourceResolver::VideoSource& /*source*/, bool /*thermal*/, QObject* parent) -> VideoReceiver* {
        auto* r = new FakeVideoReceiver(gstreamer, parent);
        if (configurer)
            configurer(r);
        if (outReceiver)
            *outReceiver = r;
        return r;
    };
}

}  // namespace FakeReceiverHelpers
