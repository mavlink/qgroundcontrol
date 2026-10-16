#pragma once

#include <gst/gstelement.h>
#include <gst/gstpad.h>

#include <QtCore/QString>

namespace GStreamer::SourceFactory {

/// RTP jitter-buffer policy for sources that produce `application/x-rtp` caps.
/// Ignored for non-RTP sources (e.g. raw MPEG-TS over TCP).
enum class JitterBuffer
{
    None,           ///< No playout buffering; omits an explicit buffer or sets `rtspsrc` latency to zero.
    DropOnLatency,  ///< `rtpjitterbuffer` with `drop-on-latency=TRUE`.
};

/// Source-bin construction parameters. Defaults match the drone-GCS profile: 80 ms playout
/// latency with RFC 4588 retransmission on RTSP, leaving ~3× the 25 ms rtx-delay for packet recovery.
/// Callers wanting sub-frame latency should use JitterBuffer::None.
struct Config
{
    JitterBuffer jitterBuffer = JitterBuffer::DropOnLatency;
    int latencyMs = 80;
    bool doRetransmission = true;  ///< RTSP only: plain UDP/TCP sources have no RTCP back-channel for NACKs.
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

/// Holds an RTP H.264/H.265 depayloader's byte-stream output back until a full parameter set (SPS and PPS, plus VPS
/// for H.265) has arrived, then emits the parameter sets together with the first slice and removes itself. create()
/// applies it to every depayloader it builds or parsebin plugs; length-prefixed output is left alone.
void gateOnParameterSets(GstPad* depayloaderSrc, bool h265);

}  // namespace GStreamer::SourceFactory
