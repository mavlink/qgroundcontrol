#pragma once

#include <QtCore/QString>
#include <cstdint>
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
    /// Blocking network I/O timeout for source elements that expose one, in seconds.
    uint32_t timeoutS = 8;
};

/// Build a source bin that exposes parsed encoded video for `uri`.
/// Supported schemes: rtsp/rtspt, tcp:// (MPEG-TS), udp:// (H.264 RTP),
/// udp265:// (H.265 RTP), mpegts:// (MPEG-TS over UDP), and http(s)://
/// (multipart MJPEG).
///
/// Ghost pads on RTP/MPEG-TS bins are wired lazily after upstream produces pads.
/// The HTTP MJPEG bin exposes its parsed-JPEG pad immediately.
///
/// Returns the source bin or nullptr on failure.
GstElement* create(const QString& uri, const Config& config);

}  // namespace GStreamer::SourceFactory
