#include "GstRtpPacketProbe.h"

#include <atomic>
#include <memory>

#include "RtpPacketCounter.h"

namespace {

constexpr char kRtpPacketProbeKey[] = "qgc-rtp-packet-probe";
// RFC 3550, section 5.1: the version, the sequence number, the timestamp and the SSRC are in the first 12 bytes.
constexpr gsize kRtpFixedHeaderSize = 12;
constexpr guint8 kRtpVersion = 2;
// RFC 5761, section 4: a sender may put its RTCP packets, version 2 as well, on the RTP port. Their second byte is a
// packet type in this range, which RTP payload types leave free for it.
constexpr guint8 kRtcpPacketTypeFirst = 192;
constexpr guint8 kRtcpPacketTypeLast = 223;

struct RtpPacketProbe
{
    explicit RtpPacketProbe(int clockRate)
        : counter(clockRate)
    {}

    void count(GstBuffer* buffer)
    {
        // Copied out rather than mapped: mapping a buffer of several memories merges them into a new one.
        guint8 header[kRtpFixedHeaderSize];
        if ((gst_buffer_extract(buffer, 0, header, sizeof(header)) != sizeof(header)) ||
            ((header[0] >> 6) != kRtpVersion) ||
            ((header[1] >= kRtcpPacketTypeFirst) && (header[1] <= kRtcpPacketTypeLast))) {
            return;
        }
        const GstClockTime arrival = GST_BUFFER_DTS_OR_PTS(buffer);
        counter.add(GST_READ_UINT16_BE(header + 2), GST_READ_UINT32_BE(header + 4), GST_READ_UINT32_BE(header + 8),
                    GST_CLOCK_TIME_IS_VALID(arrival) ? std::optional(static_cast<qint64>(arrival)) : std::nullopt);
    }

    void publish()
    {
        packetsReceived.store(counter.packetsReceived(), std::memory_order_relaxed);
        packetsLost.store(counter.packetsLost(), std::memory_order_relaxed);
        jitterNs.store(counter.jitterNs().value_or(-1), std::memory_order_relaxed);
    }

    /// Only the pad's streaming thread uses it.
    RtpPacketCounter counter;
    std::atomic<quint64> packetsReceived = 0;
    std::atomic<quint64> packetsLost = 0;
    std::atomic<qint64> jitterNs = -1;
};

// The pad's probe and the bin each hold one, so neither is left with a probe the other one freed.
using SharedRtpPacketProbe = std::shared_ptr<RtpPacketProbe>;

void freeRtpPacketProbe(gpointer data)
{
    delete static_cast<SharedRtpPacketProbe*>(data);
}

GstPadProbeReturn onRtpPackets([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    RtpPacketProbe& probe = **static_cast<SharedRtpPacketProbe*>(data);
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER_LIST) {
        GstBufferList* const list = GST_PAD_PROBE_INFO_BUFFER_LIST(info);
        const guint packets = list ? gst_buffer_list_length(list) : 0;
        for (guint i = 0; i < packets; ++i) {
            probe.count(gst_buffer_list_get(list, i));
        }
    } else if (GstBuffer* const buffer = GST_PAD_PROBE_INFO_BUFFER(info)) {
        probe.count(buffer);
    }
    probe.publish();
    return GST_PAD_PROBE_OK;
}

}  // namespace

namespace GStreamer {

bool countRtpPackets(GstElement* bin, GstPad* pad, int clockRate)
{
    if (!bin || !pad) {
        return false;
    }
    auto* const padProbe = new SharedRtpPacketProbe(std::make_shared<RtpPacketProbe>(clockRate));
    auto* const binProbe = new SharedRtpPacketProbe(*padProbe);
    if (gst_pad_add_probe(pad, static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST),
                          onRtpPackets, padProbe, freeRtpPacketProbe) == 0) {
        delete padProbe;
        delete binProbe;
        return false;
    }
    g_object_set_data_full(G_OBJECT(bin), kRtpPacketProbeKey, binProbe, freeRtpPacketProbe);
    return true;
}

std::optional<RtpPacketCounts> rtpPacketCounts(GstElement* bin)
{
    const auto* const shared =
        bin ? static_cast<const SharedRtpPacketProbe*>(g_object_get_data(G_OBJECT(bin), kRtpPacketProbeKey)) : nullptr;
    if (!shared) {
        return std::nullopt;
    }
    const RtpPacketProbe& probe = **shared;
    const qint64 jitterNs = probe.jitterNs.load(std::memory_order_relaxed);
    return RtpPacketCounts{
        .packetsReceived = probe.packetsReceived.load(std::memory_order_relaxed),
        .packetsLost = probe.packetsLost.load(std::memory_order_relaxed),
        .jitterMs = (jitterNs >= 0) ? (static_cast<double>(jitterNs) / GST_MSECOND) : -1.0,
    };
}

}  // namespace GStreamer
