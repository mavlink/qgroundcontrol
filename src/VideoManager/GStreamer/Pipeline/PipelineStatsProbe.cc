#include "PipelineStatsProbe.h"

#include <algorithm>
#include <utility>

#include <QtCore/QString>

#include "GstScoped.h"
#include "VideoStreamStats.h"

namespace {

struct JitterBufferTotals
{
    bool found = false;
    quint64 packetsPushed = 0;
    quint64 packetsLost = 0;
    quint64 packetsLate = 0;
    quint64 packetsDuplicate = 0;
    quint64 rtxRequested = 0;
    quint64 rtxRecovered = 0;
    guint64 maxJitterNs = 0;
};

/// The "stats" of every rtpjitterbuffer in @p bin added up, rtspsrc's internal ones included.
JitterBufferTotals jitterBufferTotals(GstElement* bin)
{
    JitterBufferTotals totals;
    if (!bin || !GST_IS_BIN(bin)) {
        return totals;
    }
    const auto addStats = [](const GValue* item, gpointer data) {
        auto* const element = GST_ELEMENT(g_value_get_object(item));
        GstElementFactory* const factory = gst_element_get_factory(element);
        if (!factory || (g_strcmp0(GST_OBJECT_NAME(factory), "rtpjitterbuffer") != 0)) {
            return;
        }
        GstStructure* rawStats = nullptr;
        g_object_get(element, "stats", &rawStats, nullptr);
        const GStreamer::GstStructurePtr stats = GStreamer::adoptStructure(rawStats);
        const auto field = [&stats](const char* name) {
            guint64 value = 0;
            if (stats) {
                (void) gst_structure_get_uint64(stats.get(), name, &value);
            }
            return value;
        };
        auto* const sum = static_cast<JitterBufferTotals*>(data);
        sum->found = true;
        sum->packetsPushed += field("num-pushed");
        sum->packetsLost += field("num-lost");
        sum->packetsLate += field("num-late");
        sum->packetsDuplicate += field("num-duplicates");
        sum->rtxRequested += field("rtx-count");
        sum->rtxRecovered += field("rtx-success-count");
        sum->maxJitterNs = std::max(sum->maxJitterNs, field("avg-jitter"));
    };
    GstIterator* const elements = gst_bin_iterate_recurse(GST_BIN(bin));
    // rtspsrc adds its rtpbin, and rtpbin its jitter buffers, while the stream runs.
    while (gst_iterator_foreach(elements, addStats, &totals) == GST_ITERATOR_RESYNC) {
        gst_iterator_resync(elements);
        totals = {};
    }
    gst_iterator_free(elements);
    return totals;
}

/// The codec of the compressed video @p caps describe: "H.264", "H.265", ...; empty without caps.
QString codecName(const GstCaps* caps)
{
    if (!caps || gst_caps_is_empty(caps) || gst_caps_is_any(caps)) {
        return {};
    }
    const GstStructure* const structure = gst_caps_get_structure(caps, 0);
    static constexpr std::pair<const char*, const char*> kCodecs[] = {
        {"video/x-h264", "H.264"}, {"video/x-h265", "H.265"}, {"video/x-av1", "AV1"},
        {"video/x-vp8", "VP8"},    {"video/x-vp9", "VP9"},    {"image/jpeg", "MJPEG"},
    };
    for (const auto& [mediaType, codec] : kCodecs) {
        if (gst_structure_has_name(structure, mediaType)) {
            return QString::fromLatin1(codec);
        }
    }
    gint mpegVersion = 0;
    if (gst_structure_has_name(structure, "video/mpeg") &&
        gst_structure_get_int(structure, "mpegversion", &mpegVersion)) {
        return QStringLiteral("MPEG-%1").arg(mpegVersion);
    }
    return QString::fromUtf8(gst_structure_get_name(structure));
}

}  // namespace

void PipelineStatsProbe::noteBuffer(GstBuffer* buffer)
{
    (void) _bytesReceived.fetch_add(gst_buffer_get_size(buffer), std::memory_order_relaxed);
    if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
        return;
    }
    const GstClockTime time = GST_BUFFER_PTS_IS_VALID(buffer) ? GST_BUFFER_PTS(buffer) : GST_BUFFER_DTS(buffer);
    if (GST_CLOCK_TIME_IS_VALID(time)) {
        _noteKeyframe(time);
    }
}

void PipelineStatsProbe::_noteKeyframe(GstClockTime time)
{
    if (_keyframeTimeCount > 0) {
        const GstClockTime previous = _keyframeTimes[_keyframeTimeCount - 1];
        // Parameter sets that arrive in a buffer of their own share the keyframe's timestamp.
        if (time == previous) {
            return;
        }
        // Timestamps that went back start over, as after a new segment.
        if (time < previous) {
            _keyframeTimeCount = 0;
        }
    }
    if (_keyframeTimeCount == kKeyframeWindow) {
        std::shift_left(_keyframeTimes.begin(), _keyframeTimes.end(), 1);
        --_keyframeTimeCount;
    }
    _keyframeTimes[_keyframeTimeCount++] = time;
    if (_keyframeTimeCount >= 2) {
        const GstClockTime span = _keyframeTimes[_keyframeTimeCount - 1] - _keyframeTimes[0];
        _keyframeInterval.store(span / (_keyframeTimeCount - 1), std::memory_order_relaxed);
    }
}

void PipelineStatsProbe::sample(GstElement* pipeline, GstElement* source, GstElement* tee,
                                VideoStreamStatsMeter::Counters& counters, VideoStreamStats& stats) const
{
    counters.bytesReceived = _bytesReceived.load(std::memory_order_relaxed);

    const JitterBufferTotals jitterBuffers = jitterBufferTotals(source);
    if (jitterBuffers.found) {
        counters.rtp = VideoStreamStatsMeter::RtpCounters{.packetsReceived = jitterBuffers.packetsPushed,
                                                          .packetsLost = jitterBuffers.packetsLost};
        stats.jitterMs = static_cast<double>(jitterBuffers.maxJitterNs) / GST_MSECOND;
        stats.packetsLate = static_cast<qint64>(jitterBuffers.packetsLate);
        stats.packetsDuplicate = static_cast<qint64>(jitterBuffers.packetsDuplicate);
        stats.rtxRequested = static_cast<qint64>(jitterBuffers.rtxRequested);
        stats.rtxRecovered = static_cast<qint64>(jitterBuffers.rtxRecovered);
    } else {
        counters.rtp.reset();
        stats.jitterMs = -1.0;
        stats.packetsLate = -1;
        stats.packetsDuplicate = -1;
        stats.rtxRequested = -1;
        stats.rtxRecovered = -1;
    }

    const GstClockTime keyframeInterval = _keyframeInterval.load(std::memory_order_relaxed);
    stats.keyframeIntervalMs =
        GST_CLOCK_TIME_IS_VALID(keyframeInterval) ? static_cast<double>(keyframeInterval) / GST_MSECOND : -1.0;

    GstClockTime minLatency = 0;
    GstClockTime maxLatency = 0;
    stats.latencyMs =
        queryLatency(pipeline, minLatency, maxLatency) ? static_cast<double>(minLatency) / GST_MSECOND : -1.0;

    const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(tee ? gst_element_get_static_pad(tee, "sink") : nullptr);
    const GStreamer::GstCapsPtr caps =
        GStreamer::adoptCaps(teeSink ? gst_pad_get_current_caps(teeSink.get()) : nullptr);
    stats.codec = codecName(caps.get());
}

bool PipelineStatsProbe::queryLatency(GstElement* pipeline, GstClockTime& min, GstClockTime& max)
{
    if (!pipeline) {
        return false;
    }
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_latency());
    if (!gst_element_query(pipeline, query.get())) {
        return false;
    }
    gboolean live = FALSE;
    gst_query_parse_latency(query.get(), &live, &min, &max);
    return true;
}
