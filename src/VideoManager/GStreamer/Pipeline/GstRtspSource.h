#pragma once

#include <gst/gst.h>

#include <QtCore/QString>

#include "GstScoped.h"
#include "GstSourceFactory.h"

class QUrl;

/// The RTSP source GStreamer::SourceFactory::create() builds, with QGC's transport, latency and retransmission policy.
namespace GStreamer::SourceFactory {

/// SDP-derived caps carry the stream's media type; audio and metadata streams must never reach the video parser.
bool isVideoMedia(const GstStructure* structure);

/// A configured rtspsrc for @p uri, with @p sourceUrl's user info as its credentials, or null after logging why.
/// @p latencyMs: the playout latency, unless @p config asks for no jitter buffer.
GstElementPtr buildRtspSource(const QString& uri, const QUrl& sourceUrl, const Config& config, guint latencyMs);

}  // namespace GStreamer::SourceFactory
