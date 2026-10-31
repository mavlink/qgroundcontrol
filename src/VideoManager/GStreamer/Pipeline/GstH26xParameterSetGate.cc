#include "GstH26xParameterSetGate.h"

#include <gst/gst.h>

#include "GstScoped.h"
#include "GstSourceFactory.h"

namespace GStreamer::SourceFactory {

namespace {

const char* elementFactoryName(GstElement* element)
{
    GstElementFactory* factory = gst_element_get_factory(element);
    return factory ? gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)) : nullptr;
}

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
            summary.vps |= (nalType == 32U);
            summary.sps |= (nalType == 33U);
            summary.pps |= (nalType == 34U);
            summary.slice |= (nalType <= 31U);
        } else {
            const guint nalType = header & 0x1FU;
            summary.sps |= (nalType == 7U);
            summary.pps |= (nalType == 8U);
            summary.slice |= ((nalType >= 1U) && (nalType <= 5U));
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

void configureH26xParser(GstElement* element)
{
    const char* factoryName = elementFactoryName(element);
    if ((g_strcmp0(factoryName, "h264parse") == 0) || (g_strcmp0(factoryName, "h265parse") == 0)) {
        // Recording can begin long after RTSP setup. Repeat codec parameter sets at every IDR so
        // the late-opened recording branch can construct MP4/MOV codec_data without relying on
        // the camera to resend its startup-only VPS/SPS/PPS or SPS/PPS.
        g_object_set(element, "config-interval", -1, nullptr);
    }
}

void configureH26xRtpDepayloader(GstElement* element, bool requestKeyframes)
{
    const char* factoryName = elementFactoryName(element);
    if (!isH26xRtpDepayloader(factoryName)) {
        return;
    }
    // After packet loss, ask the camera for a keyframe rather than wait out the GOP (GStreamer 1.20+).
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
