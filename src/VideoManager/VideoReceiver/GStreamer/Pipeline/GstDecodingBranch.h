#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <atomic>
#include <functional>
#include <gst/gstelement.h>
#include <gst/gstpad.h>

#include <QtMultimedia/QMediaMetaData>

#include "GstObjectPtr.h"
#include "GstTeeBranch.h"
#include "VideoReceiver.h"

class GstAppsinkBridge;
class VideoFrameDelivery;

Q_DECLARE_LOGGING_CATEGORY(GstDecodingBranchLog)

/// Manages the decoder tee branch (decodebin3 → appsink → VideoFrameDelivery).
class GstDecodingBranch : public GstTeeBranch
{
public:
    GstDecodingBranch() = default;
    ~GstDecodingBranch() override;

    // ── Sink setup ───────────────────────────────────────────────────

    void setupSink(VideoFrameDelivery* delivery, QObject* parent);

    void teardownSink();

    [[nodiscard]] GstElement* sinkElement() const { return _videoSink.get(); }

    [[nodiscard]] bool hasSink() const { return !!_videoSink; }

    [[nodiscard]] GstAppsinkBridge* appsinkBridge() const { return _appsinkBridge; }

    // ── Deferred decoding ────────────────────────────────────────────

    void setDecodingPending(bool pending) { _decodingPending.store(pending, std::memory_order_relaxed); }
    [[nodiscard]] bool isDecodingPending() const { return _decodingPending.load(std::memory_order_relaxed); }

    // ── Pipeline integration ─────────────────────────────────────────

    void ensureSinkInPipeline(GstElement* pipeline);

    bool addDecoder(GstElement* pipeline, GstElement* src, std::function<void(GstPad*)> onNewPad, GCallback padAddedCb,
                    gpointer cbData);

    bool addVideoSink(GstElement* pipeline, GstPad* pad, GstElement* decoderValve);

    /// Log and record the selected codec. Returns a QMediaMetaData populated
    /// with VideoCodec (from decoder factory name) and any other available info.
    /// Caller should also populate Resolution/VideoFrameRate from pad caps.
    [[nodiscard]] QMediaMetaData logSelectedCodec();

    // ── Branch teardown ──────────────────────────────────────────────

    void shutdown(GstElement* pipeline);

    // ── State ────────────────────────────────────────────────────────
    // `state()`/`setState()` and `setPendingStop()`/`takePendingStop()` are
    // inherited from GstTeeBranch.

    [[nodiscard]] GstElement* decoder() const { return _decoder.get(); }

    void setVideoSinkProbeGuard(GstPadProbeGuard guard) { _videoSinkProbeGuard = std::move(guard); }

    // ── HW decoder fallback ──────────────────────────────────────────

    [[nodiscard]] bool hwDecoderFailed() const { return _hwDecoderFailed; }
    void setHwDecoderFailed(bool failed) { _hwDecoderFailed = failed; }

    [[nodiscard]] bool isHwDecoding() const { return _isHwDecoding; }
    [[nodiscard]] const QString& activeDecoderName() const { return _activeDecoderName; }

    bool isHwDecoderError(GstMessage* msg) const;

    // ── Frame timing ─────────────────────────────────────────────────

    void noteVideoSinkFrame()
    {
        _lastVideoFrameTime.store(QDateTime::currentSecsSinceEpoch(), std::memory_order_relaxed);
    }

    [[nodiscard]] qint64 lastVideoFrameTime() const { return _lastVideoFrameTime.load(std::memory_order_relaxed); }
    /// Reset the frame timestamp to zero (call from worker thread to mark no frames seen yet).
    void resetVideoFrameTime() { _lastVideoFrameTime.store(0, std::memory_order_relaxed); }
    /// Atomically prime the frame timestamp to `now` (safe from any thread — avoids the
    /// decoder-state mutation in noteVideoSinkFrame(), which is worker-thread-only).
    void resetVideoFrameTime(qint64 now) { _lastVideoFrameTime.store(now, std::memory_order_relaxed); }

    [[nodiscard]] QSize lastVideoSize() const { return _lastVideoSize; }

private:
    // `_state` and `_pendingStop` live on the GstTeeBranch base.
    std::atomic<bool> _decodingPending{false};
    bool _hwDecoderFailed = false;
    bool _isHwDecoding = false;
    QString _activeDecoderName;

    GstAppsinkBridge* _appsinkBridge = nullptr;
    GstObjectPtr<GstElement> _decoder;
    GstObjectPtr<GstElement> _videoSink;
    GstPadProbeGuard _videoSinkProbeGuard;
    std::atomic<qint64> _lastVideoFrameTime{0};
    QSize _lastVideoSize;
};
