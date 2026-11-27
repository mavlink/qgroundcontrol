#include "GstQgcSinkStats.h"

#include <numeric>
#include <utility>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>

#include "GstQgcFrameMap.h"

Q_DECLARE_LOGGING_CATEGORY(GstQgcQVideoSinkLog)

namespace GstQgc {

quint64 SinkStats::displayed() const
{
    const quint64 frames = std::accumulate(pathFrames.begin(), pathFrames.end(), quint64(0));
    return (frames > superseded) ? (frames - superseded) : 0;
}

SinkStats SinkStats::operator-(const SinkStats& since) const
{
    const auto grown = [](quint64 now, quint64 before) -> quint64 { return (now > before) ? (now - before) : 0; };
    SinkStats growth{.input = grown(input, since.input),
                     .dropped = grown(dropped, since.dropped),
                     .delivered = grown(delivered, since.delivered),
                     .gpuFallbacks = grown(gpuFallbacks, since.gpuFallbacks),
                     .streamDemotions = grown(streamDemotions, since.streamDemotions),
                     .tiledDrops = grown(tiledDrops, since.tiledDrops),
                     .superseded = grown(superseded, since.superseded)};
    for (std::size_t i = 0; i < pathFrames.size(); ++i) {
        growth.pathFrames[i] = grown(pathFrames[i], since.pathFrames[i]);
    }
    return growth;
}

PathStats formatSinkStats(const SinkStats& stats)
{
    PathStats out;
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        const auto path = HwVideoBufferPath(i);
        if (const quint64 frames = stats.frames(path); frames > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(GstHw::toString(path))).arg(frames);
            if (path != HwVideoBufferPath::None) {
                out.totalDelivered += frames;
            }
        }
    }
    const std::pair<const char*, quint64> counts[] = {{"gpu-fallbacks", stats.gpuFallbacks},
                                                      {"stream-demotions", stats.streamDemotions},
                                                      {"tiled-drops", stats.tiledDrops}};
    for (const auto& [name, count] : counts) {
        if (count > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(name)).arg(count);
        }
    }
    return out;
}

quint64 SinkCounters::noteDelivered(HwVideoBufferPath path, bool superseded)
{
    {
        const QMutexLocker locker(&_deliveryMutex);
        (void) _pathFrames[static_cast<std::size_t>(path)].fetch_add(1, std::memory_order_relaxed);
        if (superseded) {
            (void) _superseded.fetch_add(1, std::memory_order_relaxed);
        }
    }
    return _delivered.fetch_add(1, std::memory_order_relaxed) + 1;
}

void SinkCounters::noteGpuFallback(bool firstOfEpoch)
{
    (void) _gpuFallbacks.fetch_add(1, std::memory_order_relaxed);
    if (firstOfEpoch) {
        (void) _streamDemotions.fetch_add(1, std::memory_order_relaxed);
    }
}

SinkStats SinkCounters::snapshot() const
{
    SinkStats stats{.input = input(),
                    .dropped = dropped(),
                    .delivered = delivered(),
                    .gpuFallbacks = _gpuFallbacks.load(std::memory_order_relaxed),
                    .streamDemotions = _streamDemotions.load(std::memory_order_relaxed),
                    .tiledDrops = _tiledDrops.load(std::memory_order_relaxed)};
    const QMutexLocker locker(&_deliveryMutex);
    for (std::size_t i = 0; i < stats.pathFrames.size(); ++i) {
        stats.pathFrames[i] = _pathFrames[i].load(std::memory_order_relaxed);
    }
    stats.superseded = _superseded.load(std::memory_order_relaxed);
    return stats;
}

void PathSession::startEpoch()
{
    _pathCache = HwResolvedPathCache{};
    _demotionRecorded = false;
    _tiledDropLogged = false;
}

void PathSession::stop()
{
    startEpoch();
    _consecutiveMapFailures = 0;
}

PathSession::Verdict PathSession::onMapFailed(const MappedFrame& mapped, SinkCounters& counters)
{
    const quint64 failures = ++_consecutiveMapFailures;
    if (mapped.noCpuLayout) {
        // Until upstream renegotiates (the sink asked for it when the DMABuf import stopped), or the run escalates.
        counters.noteTiledDrop();
        if (!std::exchange(_tiledDropLogged, true)) {
            qCWarning(GstQgcQVideoSinkLog) << "tiled DMA_DRM frame not imported and has no CPU layout — dropped";
        }
    } else if ((failures & 0x3F) == 1) {
        qCWarning(GstQgcQVideoSinkLog) << "mapping failed, consecutive=" << failures;
    }
    if (failures >= kMaxConsecutiveMapFailures) {
        qCWarning(GstQgcQVideoSinkLog) << failures << "consecutive map failures — erroring out";
        return Verdict::Error;
    }
    return Verdict::Drop;
}

void PathSession::onDelivered(const MappedFrame& mapped, SinkCounters& counters)
{
    if (mapped.demoted) {
        counters.noteGpuFallback(!std::exchange(_demotionRecorded, true));
    }
}

}  // namespace GstQgc
