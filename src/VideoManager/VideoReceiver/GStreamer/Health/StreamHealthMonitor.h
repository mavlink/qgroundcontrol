#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkInformation>
#include <atomic>
#include <cstdint>
#include <functional>
#include <gst/gstelement.h>

#include "GstObjectPtr.h"

class QTimer;
class GstDecodingBranch;
class VideoFrameDelivery;

Q_DECLARE_LOGGING_CATEGORY(StreamHealthMonitorLog)

/// Watchdog + jitter-buffer tuner for a live GStreamer pipeline.
///
/// Owns:
///   - 1-Hz source-frame watchdog (emits `sourceTimeout` if no source frames for > `timeoutSec`).
///   - 5-second jitter-tune timer (updates rtpjitterbuffer latency from EWMA-measured delay).
///
/// Non-owning view of the pipeline + decoding branch + frame delivery. The owning
/// `GstVideoReceiver` guarantees those outlive the monitor across start/stop
/// cycles by calling `attach()` before start and `detach()` before teardown.
///
/// Thread affinity: expected to live on the receiver's worker thread.
/// `noteSourceFrame()` is the only method callable from the GStreamer streaming
/// thread — all others must be called from the owning thread.
class StreamHealthMonitor : public QObject
{
    Q_OBJECT

public:
    explicit StreamHealthMonitor(QObject* parent = nullptr);
    ~StreamHealthMonitor() override;

    /// Create timers on the current thread and start them. Idempotent: calling
    /// twice leaves timers running. Call from the monitor's owning thread.
    void initTimers();

    /// Attach the pipeline + branch + delivery endpoint. Safe to pass nullptrs (monitor
    /// short-circuits its ticks). Clears cached jitter element so it's re-resolved
    /// against the new pipeline on the next tick.
    void attach(GstElement* pipeline, GstDecodingBranch* decoding, VideoFrameDelivery* delivery);

    /// Detach — equivalent to `attach(nullptr, nullptr, nullptr)` but with
    /// explicit intent. Does NOT stop timers (a detached monitor ticks harmlessly).
    void detach();

    void setTimeoutSec(uint32_t s) { _timeoutSec = s; }

    /// Predicate: is decoding currently active?  Monitor only emits
    /// `decoderTimeout` when this returns true.
    void setDecodingPredicate(std::function<bool()> pred) { _isDecoding = std::move(pred); }

    /// Note a source-thread frame arrival (call from the tee-pad probe).
    void noteSourceFrame();

    /// Reset the source-frame timestamp. Called on start/restart so the
    /// watchdog doesn't fire immediately from a stale timestamp.
    void reset();

signals:
    /// Source-side watchdog fired: no source frames for `elapsedSec` seconds.
    void sourceTimeout(qint64 elapsedSec);
    /// Decoder-side watchdog fired: no decoded frames for `elapsedSec` seconds.
    void decoderTimeout(qint64 elapsedSec);

private:
    void _onWatchdogTick();
    void _onJitterTuneTick();
    /// Invoked when the platform's network reachability changes. A transition to
    /// Online triggers an immediate watchdog tick so a pending timeout fires
    /// without waiting up to 1 s for the next periodic tick.
    void _onReachabilityChanged(QNetworkInformation::Reachability reach);

    QTimer* _watchdogTimer = nullptr;
    QTimer* _jitterTimer = nullptr;

    // Non-owning views — owner guarantees lifetime via attach/detach.
    GstElement* _pipeline = nullptr;
    GstDecodingBranch* _decoding = nullptr;
    VideoFrameDelivery* _delivery = nullptr;

    // Cached rtpjitterbuffer ref, resolved lazily by tuner.
    GstObjectPtr<GstElement> _cachedJitterElement;

    // atomic — written from the streaming thread (`noteSourceFrame`),
    // read from the worker thread (`_onWatchdogTick`).
    std::atomic<qint64> _lastSourceFrameTime{0};
    uint32_t _timeoutSec = 0;
    std::function<bool()> _isDecoding;
};

namespace GstJitterTuning {
void tune(GstElement* pipeline, GstObjectPtr<GstElement>& cachedJitter, float latencyMs);
}
