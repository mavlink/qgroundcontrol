#include "StreamHealthMonitor.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include "GStreamerHelpers.h"
#include "GstDecodingBranch.h"
#include "GstRecordingBranch.h"  // BranchState
#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(StreamHealthMonitorLog, "Video.StreamHealthMonitor")

StreamHealthMonitor::StreamHealthMonitor(QObject* parent) : QObject(parent) {}

StreamHealthMonitor::~StreamHealthMonitor() = default;

void StreamHealthMonitor::initTimers()
{
    if (_watchdogTimer) {
        return;  // already initialised
    }

    // Timers heap-allocated on the current thread so their affinity matches.
    // Value members would stay on the thread where the enclosing object was
    // constructed, not the thread it was moveToThread'd to.
    _watchdogTimer = new QTimer(this);
    _watchdogTimer->setTimerType(Qt::CoarseTimer);
    (void)connect(_watchdogTimer, &QTimer::timeout, this, &StreamHealthMonitor::_onWatchdogTick);
    _watchdogTimer->start(1000);

    _jitterTimer = new QTimer(this);
    _jitterTimer->setTimerType(Qt::CoarseTimer);
    (void)connect(_jitterTimer, &QTimer::timeout, this, &StreamHealthMonitor::_onJitterTuneTick);
    _jitterTimer->start(5000);

    // Reachability backend is loaded on demand; repeated calls are cheap no-ops.
    // When it's unavailable (headless CI, unsupported platform) instance() returns
    // null and we fall back to reachability-unaware behaviour.
    QNetworkInformation::loadDefaultBackend();
    if (auto* info = QNetworkInformation::instance()) {
        (void)connect(info, &QNetworkInformation::reachabilityChanged,
                      this, &StreamHealthMonitor::_onReachabilityChanged);
    }
}

void StreamHealthMonitor::attach(GstElement* pipeline, GstDecodingBranch* decoding, VideoFrameDelivery* delivery)
{
    _pipeline = pipeline;
    _decoding = decoding;
    _delivery = delivery;
    // Clear cached ref so jitter tuner re-resolves against the new pipeline.
    _cachedJitterElement = {};
}

void StreamHealthMonitor::detach()
{
    attach(nullptr, nullptr, nullptr);
}

void StreamHealthMonitor::noteSourceFrame()
{
    _lastSourceFrameTime.store(QDateTime::currentSecsSinceEpoch(), std::memory_order_relaxed);
}

void StreamHealthMonitor::reset()
{
    _lastSourceFrameTime.store(0, std::memory_order_relaxed);
}

void StreamHealthMonitor::_onWatchdogTick()
{
    if (!_pipeline) {
        return;
    }

    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // Initialise on first tick so the watchdog doesn't fire from a cold start.
    qint64 lastSrc = _lastSourceFrameTime.load(std::memory_order_relaxed);
    if (lastSrc == 0) {
        _lastSourceFrameTime.store(now, std::memory_order_relaxed);
        lastSrc = now;
    }

    const qint64 elapsed = now - lastSrc;
    if (elapsed > static_cast<qint64>(_timeoutSec)) {
        // Suppress the timeout when the OS reports no network path. A reconnect
        // attempt here would just fail immediately; we'll re-evaluate as soon as
        // reachability transitions back (see _onReachabilityChanged forcing a tick).
        // loadDefaultBackend may have produced no instance — treat that as
        // "reachability unknown, don't second-guess the caller".
        const auto* info = QNetworkInformation::instance();
        if (info && info->reachability() == QNetworkInformation::Reachability::Disconnected) {
            qCDebug(StreamHealthMonitorLog)
                << "Source timeout suppressed: network unreachable (" << elapsed << "s elapsed)";
            return;
        }
        qCDebug(StreamHealthMonitorLog) << "Source timeout: no frames for" << elapsed << "s";
        emit sourceTimeout(elapsed);
        // Caller decides whether to pause/resume/restart — monitor does not
        // touch pipeline state directly.
        return;
    }

    const bool decodingActive = _decoding && _isDecoding && _isDecoding() && _decoding->state() == BranchState::Active;
    if (decodingActive) {
        qint64 lastVideo = _decoding->lastVideoFrameTime();
        if (lastVideo == 0) {
            // Prime the timestamp from the watchdog thread without touching
            // streaming-thread-only state. resetVideoFrameTime() stores `now`
            // atomically; noteVideoSinkFrame() would also flip decoder-active
            // state which must only happen on the worker thread.
            _decoding->resetVideoFrameTime(now);
            lastVideo = now;
        }

        const qint64 elapsedVideo = now - lastVideo;
        if (elapsedVideo > (static_cast<qint64>(_timeoutSec) * 2)) {
            qCDebug(StreamHealthMonitorLog) << "Decoder timeout: no decoded frames for" << elapsedVideo << "s";
            emit decoderTimeout(elapsedVideo);
        }
    }
}

void StreamHealthMonitor::_onReachabilityChanged(QNetworkInformation::Reachability reach)
{
    qCDebug(StreamHealthMonitorLog) << "Reachability changed:" << reach;
    // On recovery, short-circuit the 1-Hz periodic cadence. If a timeout was
    // being suppressed because we were offline, firing the tick now lets the
    // receiver restart the pipeline as soon as the OS reports a path.
    if (reach == QNetworkInformation::Reachability::Online) {
        _onWatchdogTick();
    }
}

void StreamHealthMonitor::_onJitterTuneTick()
{
    if (!_pipeline) {
        return;
    }
    GstJitterTuning::tune(_pipeline, _cachedJitterElement, _delivery ? _delivery->latencyMs() : -1.0F);
}

namespace GstJitterTuning {

void tune(GstElement* pipeline, GstObjectPtr<GstElement>& cachedJitter, float latencyMs)
{
    if (!pipeline || latencyMs < 0.0F) {
        return;
    }

    // Resolve on first call only. Subsequent calls reuse the cached ref,
    // avoiding a recursive bin traversal every 5 s — meaningful for
    // pipelines with no rtpjitterbuffer at all (MPEG-TS/UDP, RTMP) where
    // the walk was pure overhead.
    if (!cachedJitter) {
        gstIteratorForEach<GstElement>(gst_bin_iterate_recurse(GST_BIN(pipeline)), [&cachedJitter](GstElement* el) {
            if (cachedJitter) {
                return;  // already found
            }
            GstElementFactory* f = gst_element_get_factory(el);
            if (f && g_strcmp0(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(f)), "rtpjitterbuffer") == 0) {
                cachedJitter = GstObjectPtr<GstElement>(GST_ELEMENT(gst_object_ref(el)));
            }
        });
        if (!cachedJitter) {
            return;  // no jitterbuffer in this pipeline — nothing to tune
        }
    }

    GstElement* jitter = cachedJitter.get();
    guint currentLatency = 0;
    g_object_get(jitter, "latency", &currentLatency, nullptr);

    // Target: 1.5x measured EWMA latency, clamped to [10, 200] ms.
    // Hysteresis: only adjust if the target differs by >20% from current.
    const auto target = static_cast<guint>(qBound(10.0F, latencyMs * 1.5F, 200.0F));
    const float ratio = (currentLatency > 0) ? static_cast<float>(target) / static_cast<float>(currentLatency) : 2.0F;

    if (ratio > 1.2F || ratio < 0.8F) {
        g_object_set(jitter, "latency", target, nullptr);
        qCDebug(StreamHealthMonitorLog) << "Jitter buffer tuned:" << currentLatency << "->" << target << "ms (EWMA"
                                        << latencyMs << "ms)";
    }
}

}  // namespace GstJitterTuning
