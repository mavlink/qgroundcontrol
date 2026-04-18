#pragma once

#include <QtCore/QString>
#include <gst/gstelement.h>
#include <optional>

#include "GstObjectPtr.h"
#include "VideoSourceResolver.h"

/// Aggregates the structural elements of the standard QGC video pipeline.
///
/// Returned by GstPipelineFactory::build(). The pipeline is left in GST_STATE_NULL.
/// Caller must install pad probes, connect bus/pad-added signals, and transition state.
struct GstPipelineLayout
{
    GstObjectPtr<GstElement> pipeline;
    GstObjectPtr<GstElement> source;
    GstObjectPtr<GstElement> tee;
    GstObjectPtr<GstElement> decoderValve;
    GstObjectPtr<GstElement> recorderValve;
};

namespace GstPipelineFactory {

/// Build the standard QGC video pipeline: source → tee → [decoderQueue → decoderValve | recorderQueue → recorderValve].
///
/// Both valve branches start closed (drop=TRUE). Pipeline is returned in GST_STATE_NULL.
/// Source is selected from VideoSourceResolver::SourceDescriptor.
///
/// @param uri              Stream URI
/// @param bufferMs         Jitter buffer in ms: -1 = none, 0 = default, >0 = explicit.
/// @param forceSwDecoders  When true, sets `force-sw-decoders=TRUE` on any
///                         uridecodebin3 built into the source (adaptive HLS/
///                         DASH paths). Used by GstVideoReceiver to scope a
///                         HW→SW fallback to a single stream without touching
///                         the global GStreamer registry ranks.
std::optional<GstPipelineLayout> build(const QString& uri, int bufferMs, bool forceSwDecoders = false);
std::optional<GstPipelineLayout> build(const VideoSourceResolver::SourceDescriptor& source,
                                       int bufferMs,
                                       bool forceSwDecoders = false);

}  // namespace GstPipelineFactory
