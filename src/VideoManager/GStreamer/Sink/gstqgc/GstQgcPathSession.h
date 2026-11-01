#pragma once

#include <QtCore/QtTypes>

#include "HwBuffers/common/GstHwVideoBufferFactory.h"

struct MappedFrame;

namespace GstQgc {

class SinkCounters;

/// \brief What qgcqvideosink keeps about the path its frames take through one caps epoch, from set_caps to the next
/// set_caps or stop: the GPU path it resolved, and what is once per epoch, a stream demotion and the tiled-drop
/// warning. Also decides what becomes of a frame that fails to map: dropped, or after a sustained run of failures, an
/// error that ends the stream.
///
/// Threading: the sink's streaming thread; GstBaseSink serializes set_caps, show_frame and stop.
class PathSession
{
public:
    /// A run of map failures this long is a broken import, not a hiccup: the stream fails so it is set up anew.
    static constexpr quint64 kMaxConsecutiveMapFailures = 120;

    enum class Verdict
    {
        Drop,   ///< Drop the frame and go on.
        Error,  ///< Fail the stream.
    };

    /// New caps: the path is resolved anew, and the once-per-epoch bookkeeping starts over.
    void startEpoch();
    /// The sink stopped: as startEpoch(), and a run of map failures ends.
    void stop();

    /// For mapSampleToFrame(), which caches the path it resolves in the epoch.
    HwResolvedPathCache* pathCache() { return &_pathCache; }

    /// @p mapped, which has no frame, failed to map. Counts and logs it, and returns Error once the run of failures
    /// reaches kMaxConsecutiveMapFailures.
    Verdict onMapFailed(const MappedFrame& mapped, SinkCounters& counters);

    /// The frame mapped: a run of failures ends.
    void onMapped() { _consecutiveMapFailures = 0; }

    quint64 consecutiveMapFailures() const { return _consecutiveMapFailures; }

    /// @p mapped reached the QVideoSink. A frame a GPU path was tried for but the CPU delivered counts as a GPU
    /// fallback, the epoch's first also as a stream demotion.
    void onDelivered(const MappedFrame& mapped, SinkCounters& counters);

private:
    HwResolvedPathCache _pathCache = {};
    bool _demotionRecorded = false;
    bool _tiledDropLogged = false;
    quint64 _consecutiveMapFailures = 0;
};

}  // namespace GstQgc
