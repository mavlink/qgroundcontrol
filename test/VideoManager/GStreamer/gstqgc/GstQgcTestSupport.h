#pragma once

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <gst/allocators/gstfdmemory.h>
#endif

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtTypes>
#include <QtGui/QColor>
#include <QtMultimedia/QVideoFrameFormat>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "gstqgc/gstqgcqvideosink.h"
#include "gstqgc/gstqgcvideosinkbin.h"

class QVideoSink;

/// qgcqvideosink helpers shared by the test classes that drive it.
namespace GstQgcTest {

/// What reached the QVideoSink in runPipelineThroughQVideoSink().
struct PipelineRunResult
{
    int frameCount = 0;
    QVideoFrameFormat lastFormat;
    qint64 lastStartTime = -1;
    QColor centerPixel;
    bool centerPixelValid = false;
    bool eos = false;
    QString errorMessage;
};

/// Plays "videotestsrc @p sourceProperties ! @p capsLine ! qgcvideosinkbin" to EOS with the bin bound to @p videoSink,
/// as VideoManager binds it, and records the frames that reach the QVideoSink.
PipelineRunResult runPipelineThroughQVideoSink(QVideoSink& videoSink, const char* capsLine, int numBuffers = 5,
                                               bool gpuZerocopy = false, const char* sourceProperties = "",
                                               bool captureCenterPixel = false);

/// The qgcqvideosink inside the qgcvideosinkbin @p sinkBin.
inline GStreamer::GstElementPtr qvideosink(GstElement* sinkBin)
{
    return GStreamer::adoptElement(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(sinkBin)));
}

inline guint64 framesDelivered(GstElement* qvideosink)
{
    guint64 delivered = 0;
    g_object_get(qvideosink, "frames-delivered", &delivered, nullptr);
    return delivered;
}

/// A guint64 field of the sink's "frame-stats" structure; -1 if the field is missing.
inline qint64 sinkStat(GstElement* qvideosink, const char* field)
{
    const GStreamer::GstStructurePtr stats = GstQgc::frameStats(qvideosink);
    guint64 value = 0;
    const bool found = stats && gst_structure_get_uint64(stats.get(), field, &value);
    return found ? static_cast<qint64>(value) : -1;
}

/// Drives a standalone qgcqvideosink from the test thread through a linked source pad. show_frame runs inside
/// pushFrame(), so element counters are exact, and GUI-thread delivery happens only when the test pumps events.
/// Once isReady(), the sink is PLAYING and has had stream-start, BGRA caps (in @p memoryFeature, if given) and a TIME
/// segment, as an upstream element sends before its first buffer.
class SinkHarness
{
public:
    static constexpr int kWidth = 4;
    static constexpr int kHeight = 4;
    static constexpr gsize kFrameBytes = kWidth * kHeight * 4;

    explicit SinkHarness(QVideoSink* target, bool gpuZeroCopy = false, const char* memoryFeature = nullptr)
        : _sink(sinkElement(gpuZeroCopy))
        , _bus(GStreamer::adoptBus(gst_bus_new()))
        , _src(GStreamer::adoptPad(GST_PAD(gst_object_ref_sink(gst_pad_new("src", GST_PAD_SRC)))))
    {
        if (!_sink) {
            return;
        }
        gst_element_set_bus(_sink.get(), _bus.get());
        g_object_set(_sink.get(), "qvideosink", static_cast<gpointer>(target), nullptr);
        const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(_sink.get(), "sink"));
        _ready = sinkPad && (gst_pad_link(_src.get(), sinkPad.get()) == GST_PAD_LINK_OK) &&
                 gst_pad_set_active(_src.get(), TRUE) &&
                 (gst_element_set_state(_sink.get(), GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE) &&
                 start(memoryFeature);
    }

    ~SinkHarness()
    {
        if (_sink) {
            gst_element_set_state(_sink.get(), GST_STATE_NULL);
        }
        gst_pad_set_active(_src.get(), FALSE);
    }

    SinkHarness(const SinkHarness&) = delete;
    SinkHarness& operator=(const SinkHarness&) = delete;

    bool isReady() const { return _ready; }

    GstElement* element() const { return _sink.get(); }

    GstBus* bus() const { return _bus.get(); }

    bool pushSegment()
    {
        GstSegment segment;
        gst_segment_init(&segment, GST_FORMAT_TIME);
        return pushEvent(gst_event_new_segment(&segment));
    }

    bool pushEvent(GstEvent* event) { return gst_pad_push_event(_src.get(), event); }

    /// @p size below kFrameBytes produces a buffer gst_video_frame_map rejects.
    static GstBuffer* makeFrame(GstClockTime pts, gsize size = kFrameBytes)
    {
        GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
        gst_buffer_memset(buffer, 0, 0x80, size);
        GST_BUFFER_PTS(buffer) = pts;
        return buffer;
    }

    GstFlowReturn pushFrame(GstClockTime pts, bool discont = false, gsize size = kFrameBytes)
    {
        GstBuffer* buffer = makeFrame(pts, size);
        if (discont) {
            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DISCONT);
        }
        return pushBuffer(buffer);
    }

    GstFlowReturn pushBuffer(GstBuffer* buffer) { return gst_pad_push(_src.get(), buffer); }

    /// Whether the sink asked upstream to renegotiate since the last call.
    bool takeReconfigure() { return gst_pad_check_reconfigure(_src.get()); }

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    /// A frame in memfd-backed fd memory: not system memory, and no GPU path imports it.
    static GstBuffer* makeFdFrame(GstClockTime pts)
    {
        const int fd = GstTest::makeMemfd(QByteArray(kFrameBytes, char(0x80)));
        if (fd < 0) {
            return nullptr;
        }
        GstAllocator* allocator = gst_fd_allocator_new();
        GstMemory* memory = gst_fd_allocator_alloc(allocator, fd, kFrameBytes, GST_FD_MEMORY_FLAG_NONE);
        gst_object_unref(allocator);
        GstBuffer* buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, memory);
        GST_BUFFER_PTS(buffer) = pts;
        return buffer;
    }
#endif

    guint64 delivered() const { return framesDelivered(_sink.get()); }

private:
    bool start(const char* memoryFeature)
    {
        GstCaps* caps =
            gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRA", "width", G_TYPE_INT, kWidth, "height",
                                G_TYPE_INT, kHeight, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr);
        if (memoryFeature) {
            gst_caps_set_features(caps, 0, gst_caps_features_new(memoryFeature, nullptr));
        }
        const bool ok = pushEvent(gst_event_new_stream_start("qgc-harness")) && pushEvent(gst_event_new_caps(caps)) &&
                        pushSegment();
        gst_caps_unref(caps);
        return ok;
    }

    static GStreamer::GstElementPtr sinkElement(bool gpuZeroCopy)
    {
        GstElement* const sink = gst_element_factory_make_full("qgcqvideosink", "gpu-zerocopy", gpuZeroCopy, nullptr);
        return GStreamer::adoptElement(sink ? GST_ELEMENT(gst_object_ref_sink(sink)) : nullptr);
    }

    GStreamer::GstElementPtr _sink;
    GStreamer::GstBusPtr _bus;
    GStreamer::GstPadPtr _src;
    bool _ready = false;
};

/// A qgcvideosinkbin and its qgcqvideosink.
struct SinkBin
{
    GStreamer::GstElementPtr bin;
    GStreamer::GstElementPtr qvideosink;
};

inline SinkBin makeSinkBin()
{
    GstElement* const bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    if (!bin) {
        return {};
    }
    SinkBin made{GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(bin))), nullptr};
    made.qvideosink = GstQgcTest::qvideosink(bin);
    return made;
}

}  // namespace GstQgcTest

#endif
