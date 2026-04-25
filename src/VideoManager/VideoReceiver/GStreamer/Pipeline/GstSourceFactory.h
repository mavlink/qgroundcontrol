#pragma once

#include <QtCore/QString>

#include <gst/gstelement.h>

#include "VideoSourceResolver.h"

/// Per-scheme source element construction for the GStreamer pipeline.
/// `GstIngestSession` consumes the element returned here and wires it into the
/// MPEG-TS QIODevice handoff topology.
///
/// Kept separate from the pipeline assembler because protocol dispatch
/// (rtspsrc / tcpclientsrc / udpsrc / whepsrc / srtsrc /
/// gst_parse_launch) is its own concern with ~15 helpers; folding all of it
/// into the ingest-session class buries the transport-specific logic.
namespace GstSourceFactory {

/// Build a source element (possibly wrapped in a bin with tsdemux + parsebin)
/// for the given URI. Transport is classified via `VideoSourceResolver`.
/// `bufferMs`: RTP jitter buffer in ms (-1 = none, 0 = default 60, >0 = explicit).
/// Returns nullptr on failure.
GstElement* createFromUri(const QString& uri, int bufferMs);
GstElement* createFromSource(const VideoSourceResolver::SourceDescriptor& source, int bufferMs);

}  // namespace GstSourceFactory
