#include "GstH26xSetup.h"

#include "GStreamerHelpers.h"
#include "GstScoped.h"

namespace GStreamer::SourceFactory {

namespace {

// NAL unit types (ITU-T H.264 and H.265, table 7-1). Slices are the types up to the last one named here.
constexpr guint kH264NalSliceFirst = 1;
constexpr guint kH264NalSlicePartitionA = 2;
constexpr guint kH264NalSliceIdr = 5;
constexpr guint kH264NalSei = 6;
constexpr guint kH264NalSps = 7;
constexpr guint kH264NalPps = 8;
constexpr guint kH264NalDelimiter = 9;
constexpr guint kH265NalSliceLast = 31;
constexpr guint kH265NalVps = 32;
constexpr guint kH265NalSps = 33;
constexpr guint kH265NalPps = 34;
constexpr guint kH265NalDelimiter = 35;
constexpr guint kH265NalPrefixSei = 39;

bool isH26xRtpDepayloader(const char* factoryName)
{
    return (g_strcmp0(factoryName, "rtph264depay") == 0) || (g_strcmp0(factoryName, "rtph265depay") == 0);
}

// Joining a stream mid-GOP, slices before the next parameter sets can't be decoded. GStreamer 1.28's h264parse warns
// for every such slice and, when converting byte-stream to avc for the decoder, fails the pipeline with "No caps set"
// if it ever pushes an access unit before it has both SPS and PPS, e.g. an SPS whose PPS a jitter buffer dropped.
// The depayloader's DELTA_UNIT flag isn't enough either: a sliced encoder's IDR can be joined after its SPS/PPS.
struct NalSummary
{
    bool vps = false;
    bool sps = false;
    bool pps = false;
    bool slice = false;
};

NalSummary summarizeNals(GstBuffer* buffer, bool h265)
{
    NalSummary summary;
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        return summary;
    }
    const guint8* const data = map.data;
    for (gsize i = 0; (i + 3) < map.size; ++i) {
        if ((data[i] != 0) || (data[i + 1] != 0) || (data[i + 2] != 1)) {
            continue;
        }
        const guint8 header = data[i + 3];
        if (h265) {
            const guint nalType = (header >> 1) & 0x3FU;
            summary.vps |= (nalType == kH265NalVps);
            summary.sps |= (nalType == kH265NalSps);
            summary.pps |= (nalType == kH265NalPps);
            summary.slice |= (nalType <= kH265NalSliceLast);
        } else {
            const guint nalType = header & 0x1FU;
            summary.sps |= (nalType == kH264NalSps);
            summary.pps |= (nalType == kH264NalPps);
            summary.slice |= ((nalType >= kH264NalSliceFirst) && (nalType <= kH264NalSliceIdr));
        }
        i += 2;
    }
    gst_buffer_unmap(buffer, &map);
    return summary;
}

bool outputsByteStream(GstPad* pad)
{
    const GstCapsPtr caps = adoptCaps(gst_pad_get_current_caps(pad));
    const GstStructure* structure =
        (caps && !gst_caps_is_empty(caps.get())) ? gst_caps_get_structure(caps.get(), 0) : nullptr;
    return structure && (g_strcmp0(gst_structure_get_string(structure, "stream-format"), "byte-stream") == 0);
}

/// The latest parameter-set buffers seen while the gate is closed.
struct ParameterSetGate
{
    explicit ParameterSetGate(bool isH265)
        : h265(isH265)
    {}

    ~ParameterSetGate()
    {
        gst_clear_buffer(&vps);
        gst_clear_buffer(&sps);
        gst_clear_buffer(&pps);
    }

    ParameterSetGate(const ParameterSetGate&) = delete;
    ParameterSetGate& operator=(const ParameterSetGate&) = delete;

    const bool h265;
    GstBuffer* vps = nullptr;
    GstBuffer* sps = nullptr;
    GstBuffer* pps = nullptr;
};

/// Prepends @p prefix's memories to @p buffer, keeping @p buffer's timestamps and flags.
GstBuffer* prependMemories(GstBuffer* buffer, GstBuffer* prefix)
{
    buffer = gst_buffer_make_writable(buffer);
    for (guint i = gst_buffer_n_memory(prefix); i > 0; --i) {
        gst_buffer_prepend_memory(buffer, gst_buffer_get_memory(prefix, i - 1));
    }
    return buffer;
}

GstPadProbeReturn holdUntilParameterSets(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) {
        return GST_PAD_PROBE_OK;
    }
    // Length-prefixed avc/hvc1 output carries its parameter sets in the caps' codec_data.
    if (!outputsByteStream(pad)) {
        return GST_PAD_PROBE_REMOVE;
    }

    auto* gate = static_cast<ParameterSetGate*>(data);
    const NalSummary nals = summarizeNals(buffer, gate->h265);
    if (!nals.slice) {
        if (nals.vps) {
            gst_buffer_replace(&gate->vps, buffer);
        }
        if (nals.sps) {
            gst_buffer_replace(&gate->sps, buffer);
        }
        if (nals.pps) {
            gst_buffer_replace(&gate->pps, buffer);
        }
        return GST_PAD_PROBE_DROP;
    }

    const bool complete = (nals.sps || gate->sps) && (nals.pps || gate->pps) && (!gate->h265 || nals.vps || gate->vps);
    if (!complete) {
        return GST_PAD_PROBE_DROP;
    }

    // Prepend, in VPS/SPS/PPS order, the stored sets this buffer lacks; one buffer may hold several of them.
    GstBuffer* const stored[] = {(gate->h265 && !nals.vps) ? gate->vps : nullptr, nals.sps ? nullptr : gate->sps,
                                 nals.pps ? nullptr : gate->pps};
    for (int i = 2; i >= 0; --i) {
        const bool duplicate = (i > 0 && stored[i] == stored[i - 1]) || (i > 1 && stored[i] == stored[i - 2]);
        if (stored[i] && !duplicate) {
            buffer = prependMemories(buffer, stored[i]);
        }
    }
    GST_PAD_PROBE_INFO_DATA(info) = buffer;
    return GST_PAD_PROBE_REMOVE;
}

}  // namespace

bool startsAccessUnit(GstBuffer* buffer, bool h265)
{
    // Leading zero bytes, the start code, the NAL unit's header and the first byte after it.
    guint8 head[16];
    const gsize size = gst_buffer_extract(buffer, 0, head, sizeof(head));
    gsize i = 0;
    while ((i < size) && (head[i] == 0)) {
        ++i;
    }
    const gsize headerSize = h265 ? 2 : 1;
    if ((i < 2) || ((i + 1 + headerSize) >= size) || (head[i] != 1)) {
        return false;
    }
    const guint8 header = head[i + 1];
    // first_mb_in_slice is 0, or first_slice_segment_in_pic_flag is set: both read as a leading 1 bit.
    const bool firstSliceOfPicture = (head[i + 1 + headerSize] & 0x80U) != 0;
    // What may precede a picture's first slice in its access unit (H.264 7.4.1.2.3, H.265 7.4.2.4.4).
    if (h265) {
        const guint nalType = (header >> 1) & 0x3FU;
        if (nalType <= kH265NalSliceLast) {
            return firstSliceOfPicture;
        }
        return ((nalType >= kH265NalVps) && (nalType <= kH265NalDelimiter)) || (nalType == kH265NalPrefixSei) ||
               ((nalType >= 41) && (nalType <= 44)) || ((nalType >= 48) && (nalType <= 55));
    }
    const guint nalType = header & 0x1FU;
    if ((nalType == kH264NalSliceFirst) || (nalType == kH264NalSlicePartitionA) || (nalType == kH264NalSliceIdr)) {
        return firstSliceOfPicture;
    }
    return ((nalType >= kH264NalSei) && (nalType <= kH264NalDelimiter)) || ((nalType >= 14) && (nalType <= 18));
}

void configureH26xParser(GstElement* element)
{
    const char* factoryName = factoryNameOf(GST_OBJECT_CAST(element));
    if ((g_strcmp0(factoryName, "h264parse") == 0) || (g_strcmp0(factoryName, "h265parse") == 0)) {
        // Recording can begin long after RTSP setup. Repeat codec parameter sets at every IDR so
        // the late-opened recording branch can construct MP4/MOV codec_data without relying on
        // the camera to resend its startup-only VPS/SPS/PPS or SPS/PPS.
        g_object_set(element, "config-interval", -1, nullptr);
    }
}

void configureH26xRtpDepayloader(GstElement* element, bool requestKeyframes)
{
    const char* factoryName = factoryNameOf(GST_OBJECT_CAST(element));
    if (!isH26xRtpDepayloader(factoryName)) {
        return;
    }
    // After packet loss, ask the camera for a keyframe rather than wait out the GOP: rtph264depay can since GStreamer
    // 1.20, rtph265depay since 1.26.
    if (requestKeyframes && g_object_class_find_property(G_OBJECT_GET_CLASS(element), "request-keyframe")) {
        g_object_set(element, "request-keyframe", TRUE, nullptr);
    }
    if (const GstPadPtr srcPad = adoptPad(gst_element_get_static_pad(element, "src"))) {
        gateOnParameterSets(srcPad.get(), g_strcmp0(factoryName, "rtph265depay") == 0);
    }
}

void gateOnParameterSets(GstPad* depayloaderSrc, bool h265)
{
    (void) gst_pad_add_probe(depayloaderSrc, GST_PAD_PROBE_TYPE_BUFFER, holdUntilParameterSets,
                             new ParameterSetGate(h265),
                             [](gpointer gate) { delete static_cast<ParameterSetGate*>(gate); });
}

}  // namespace GStreamer::SourceFactory
