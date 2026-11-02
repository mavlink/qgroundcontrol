#include "GstQgcPathSession.h"

#include <utility>

#include <QtCore/QLoggingCategory>

#include "GstQgcFrameMap.h"
#include "GstQgcSinkCounters.h"

Q_DECLARE_LOGGING_CATEGORY(GstQgcQVideoSinkLog)

namespace GstQgc {

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
