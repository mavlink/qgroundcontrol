#pragma once

#include <gst/gst.h>
#include <optional>

#include <QtCore/QtTypes>

namespace GStreamer {

/// What countRtpPackets() has counted for a bin since it was installed.
struct RtpPacketCounts
{
    quint64 packetsReceived = 0;
    /// Shrinks when a packet counted as lost arrives late.
    quint64 packetsLost = 0;
    /// The interarrival jitter as a jitter buffer measures it; -1 until measured.
    double jitterMs = -1.0;
};

/// Counts the RTP packets @p pad pushes from now on with an RtpPacketCounter, as @p bin's: for a source without a
/// jitter buffer, which nothing else counts. @p clockRate: of their timestamps, in Hz. False when the pad takes no
/// probe.
///
/// The probe runs on the pad's streaming thread for every packet, in buffers and buffer lists alike: it reads the
/// fixed header of each packet that is RTP version 2 and not an RTCP packet sent to the same port, takes the buffer's
/// DTS (else PTS) as its arrival time, as a jitter buffer does, and neither allocates nor logs. The totals are handed
/// over in atomics.
bool countRtpPackets(GstElement* bin, GstPad* pad, int clockRate);

/// What countRtpPackets() counted for @p bin so far, from any thread; nullopt for a bin without it, or a null one.
std::optional<RtpPacketCounts> rtpPacketCounts(GstElement* bin);

}  // namespace GStreamer
