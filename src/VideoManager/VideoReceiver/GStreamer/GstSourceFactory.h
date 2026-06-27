#pragma once

#include <QtCore/QString>
#include <gst/gstelement.h>

namespace GStreamer::SourceFactory {

/// RTP jitter-buffer policy for sources that produce `application/x-rtp` caps.
/// Ignored for non-RTP sources (e.g. raw MPEG-TS over TCP).
enum class JitterBuffer
{
    None,           ///< No `rtpjitterbuffer` element; lowest latency, no reordering.
    DropOnLatency,  ///< `rtpjitterbuffer` with `drop-on-latency=TRUE`.
    Buffered,       ///< `rtpjitterbuffer` with `drop-on-latency=FALSE`.
};

/// Source-bin construction parameters. Defaults match the drone-GCS profile: 80 ms playout
/// latency with RFC 4588 retransmission, leaving ~3× the 20 ms rtx-delay for packet recovery.
/// Callers wanting sub-frame latency should use JitterBuffer::None.
struct Config
{
    JitterBuffer jitterBuffer = JitterBuffer::DropOnLatency;
    int latencyMs = 80;
    bool doRetransmission = true;
};

/// Build a source bin (`source` [+ `tsdemux`] [+ `rtpjitterbuffer`] + `parsebin`)
/// for `uri`. Supported schemes: rtsp/rtspt, tcp:// (MPEG-TS), udp:// (H.264 RTP),
/// udp265:// (H.265 RTP), mpegts:// (MPEG-TS over UDP).
///
/// Ghost pads on the returned bin are wired lazily; for `rtspsrc`/`tsdemux`/`parsebin`
/// they appear only after upstream produces pads, so callers must connect any
/// downstream `pad-added` handlers before transitioning to PLAYING.
///
/// Returns the source bin or nullptr on failure.
GstElement* create(const QString& uri, const Config& config);

}  // namespace GStreamer::SourceFactory
