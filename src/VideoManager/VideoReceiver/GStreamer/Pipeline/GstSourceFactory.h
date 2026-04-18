#pragma once

#include <QtCore/QString>

#include <gst/gstelement.h>

#include "VideoSourceResolver.h"

/// Per-scheme source element construction for the GStreamer pipeline.
/// `GstPipelineFactory::build()` consumes the element returned here and wires
/// it into the rest of the topology (tee → decoder/recorder valves).
///
/// Kept separate from the pipeline assembler because protocol dispatch
/// (rtspsrc / tcpclientsrc / udpsrc / whepsrc / srtsrc / uridecodebin3 /
/// gst_parse_launch) is its own concern with ~15 helpers; folding all of it
/// into GstPipelineFactory.cc buries the topology logic.
namespace GstSourceFactory {

/// Build a source element (possibly wrapped in a bin with tsdemux + parsebin)
/// for the given URI. Transport is classified via `VideoSourceResolver`.
/// `bufferMs`: RTP jitter buffer in ms (-1 = none, 0 = default 60, >0 = explicit).
/// `forceSwDecoders`: only meaningful for adaptive (HLS/DASH) sources that
/// internally instantiate uridecodebin3.
/// Returns nullptr on failure.
GstElement* createFromUri(const QString& uri, int bufferMs, bool forceSwDecoders);
GstElement* createFromSource(const VideoSourceResolver::SourceDescriptor& source, int bufferMs, bool forceSwDecoders);

}  // namespace GstSourceFactory
