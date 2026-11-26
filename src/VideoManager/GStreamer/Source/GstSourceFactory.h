#pragma once

#include <chrono>
#include <gst/gstelement.h>
#include <gst/gstpad.h>
#include <optional>

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(GstSourceFactoryLog)

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
    /// RTSP only: how long rtspsrc waits to connect and for the answer to each request (20 s by default). It doesn't
    /// end a stream whose TCP-interleaved data stops: there rtspsrc only sends a keep-alive when the time is up, and
    /// it is the receiver's watchdog that notices the stall.
    std::chrono::milliseconds rtspTcpTimeout{5000};
};

/// Build a source bin for `uri`: `source` [+ `tsdemux`] + `parsebin`, or for RTP over UDP
/// `udpsrc` [+ `rtpjitterbuffer`] + the codec's depayloader and parser, which also take the place of an RTSP bin's
/// `parsebin` once `rtspsrc` tells that its stream is H.264 or H.265. Supported schemes: rtsp/rtspt,
/// tcp:// (MPEG-TS), udp:// (H.264 RTP), udp265:// (H.265 RTP), mpegts:// (MPEG-TS over UDP). A UDP URI with
/// port 0 listens on a free port, and one without a host on every IPv4 address.
///
/// The RTP over UDP bin has its source pad from the start, and counts its RTP packets itself
/// (GStreamer::rtpPacketCounts()). The others' ghost pads are wired lazily; for
/// `rtspsrc`/`tsdemux`/`parsebin` they appear only after upstream produces pads, so callers must connect any
/// downstream `pad-added` handlers before transitioning to PLAYING.
///
/// Returns the source bin or nullptr on failure.
GstElement* create(const QString& uri, const Config& config);

/// rtspsrc waits this long for UDP data before it retries the session over TCP ("timeout").
inline constexpr std::chrono::milliseconds kRtspUdpTimeout{5000};

/// How much longer than the stream timeout the first data for @p uri may take: kRtspUdpTimeout for RTSP, whose UDP to
/// TCP fallback must not be cut short, and zero for a TCP source. nullopt, any time at all, for a UDP source: it only
/// listens on its port, so until somebody sends to it nothing has failed, and a new source would listen no better.
std::optional<std::chrono::milliseconds> connectGracePeriod(const QString& uri);

/// The receive buffer, in bytes, the factory asks of every UDP socket of its sources, rtspsrc's included: 8 MiB, or as
/// much as the kernel lets a socket ask for where that is less (Linux's net.core.rmem_max).
int udpReceiveBufferSize();

/// Has @p jitterBuffer, an rtpjitterbuffer of a source (rtspsrc's included), tell of each packet it drops in a
/// "drop-msg" element message: those it drops to stay within its latency ("drop-on-latency") are in none of its
/// "stats", so PipelineStatsProbe counts them from the bus.
void reportDroppedPackets(GstElement* jitterBuffer);

// The H.264/H.265 setup create() gives the depayloaders and parsers it builds or parsebin plugs
// (GstH26xParameterSetGate.cc).

/// When @p element is h264parse or h265parse, repeats the codec parameter sets at every IDR, so a recording started
/// mid-stream can build its codec_data.
void configureH26xParser(GstElement* element);

/// When @p element is rtph264depay or rtph265depay, gates its output on the parameter sets (gateOnParameterSets()) and,
/// if @p requestKeyframes (the source has a return channel, RTSP's RTCP, that carries a keyframe request upstream),
/// asks for a keyframe after packet loss.
void configureH26xRtpDepayloader(GstElement* element, bool requestKeyframes);

/// Holds an RTP H.264/H.265 depayloader's byte-stream output back until a full parameter set (SPS and PPS, plus VPS
/// for H.265) has arrived, then emits the parameter sets together with the first slice and removes itself. create()
/// applies it to every depayloader it builds or parsebin plugs; length-prefixed output is left alone.
void gateOnParameterSets(GstPad* depayloaderSrc, bool h265);

}  // namespace GStreamer::SourceFactory
