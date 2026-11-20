#pragma once

#include <chrono>

#include "GstSourceFactory.h"
#include "RecordingPlan.h"

/// The intervals and timeouts of a GstVideoReceiver's worker, and of the PipelineSession and RecordingBranch it runs,
/// that a test can't wait out. The app runs these defaults; such a test gives its receiver shorter ones.
struct GstReceiverTimings
{
    /// How often the worker looks at its stream: whether it stalled, what its statistics are, and whether a
    /// recording's part has recorded for long enough.
    std::chrono::milliseconds watchdogInterval = std::chrono::seconds{1};
    /// GStreamer::SourceFactory::Config::rtspTcpTimeout.
    std::chrono::milliseconds rtspTcpTimeout = GStreamer::SourceFactory::Config{}.rtspTcpTimeout;
    /// How long a part of a recording records before the next one takes over.
    std::chrono::milliseconds recordingPartDuration = RecordingPlan::kMaxPartDuration;
    /// How long a recording whose recorder gets the stream's keyframes waits for the first one it can write: one that
    /// comes with its parameter sets. The recording fails after that.
    std::chrono::milliseconds recordingStartTimeout = std::chrono::seconds{5};
    /// How long a file records before its muxer's estimate of the header space it has left counts: until the muxer's
    /// second header update the estimate comes from the file's first frame alone.
    std::chrono::milliseconds recordingHeaderEstimateDelay = std::chrono::seconds{10};
};
