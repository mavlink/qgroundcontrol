#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <array>
#include <atomic>
#include <cstring>
#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <memory>
#include <vector>
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <gst/allocators/gstfdmemory.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtTest/QSignalSpy>

#include "CpuVideoFramePool.h"
#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "GstQgcFrameMap.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/GstQgcAllocation.h"
#include "gstqgc/GstQgcBinChain.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcVideoFormats.h"
#include "gstqgc/gstqgcqvideosink.h"
#include "gstqgc/gstqgcvideosinkbin.h"

namespace {

guint64 framesDelivered(GstElement* qvideosink)
{
    guint64 delivered = 0;
    g_object_get(qvideosink, "frames-delivered", &delivered, nullptr);
    return delivered;
}

/// A guint64 field of the sink's "frame-stats" structure; -1 if the field is missing.
qint64 sinkStat(GstElement* qvideosink, const char* field)
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
class QgcQVideoSinkHarness
{
public:
    static constexpr int kWidth = 4;
    static constexpr int kHeight = 4;
    static constexpr gsize kFrameBytes = kWidth * kHeight * 4;

    explicit QgcQVideoSinkHarness(QVideoSink* target, bool gpuZeroCopy = false, const char* memoryFeature = nullptr)
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

    ~QgcQVideoSinkHarness()
    {
        if (_sink) {
            gst_element_set_state(_sink.get(), GST_STATE_NULL);
        }
        gst_pad_set_active(_src.get(), FALSE);
    }

    QgcQVideoSinkHarness(const QgcQVideoSinkHarness&) = delete;
    QgcQVideoSinkHarness& operator=(const QgcQVideoSinkHarness&) = delete;

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

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    /// A frame in memfd-backed fd memory: not system memory, and no GPU path imports it.
    static GstBuffer* makeFdFrame(GstClockTime pts)
    {
        const int fd = memfd_create("qgc-sink-harness", MFD_CLOEXEC);
        if (fd < 0) {
            return nullptr;
        }
        if (ftruncate(fd, kFrameBytes) != 0) {
            close(fd);
            return nullptr;
        }
        GstAllocator* allocator = gst_fd_allocator_new();
        GstMemory* memory = gst_fd_allocator_alloc(allocator, fd, kFrameBytes, GST_FD_MEMORY_FLAG_NONE);
        gst_object_unref(allocator);
        GstBuffer* buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, memory);
        gst_buffer_memset(buffer, 0, 0x80, kFrameBytes);
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

SinkBin makeSinkBin()
{
    GstElement* const bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    if (!bin) {
        return {};
    }
    SinkBin made{GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(bin))), nullptr};
    made.qvideosink = GStreamer::adoptElement(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(bin)));
    return made;
}

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

/// Plays "videotestsrc ! @p capsLine ! qgcvideosinkbin" to EOS with the bin bound to @p videoSink, as VideoManager
/// binds it, and records the frames that reach the QVideoSink.
PipelineRunResult runPipelineThroughQVideoSink(QVideoSink& videoSink, const char* capsLine, int numBuffers = 5,
                                               bool gpuZerocopy = false, const char* sourceProperties = "",
                                               bool captureCenterPixel = false)
{
    PipelineRunResult r;
    const auto owner = std::make_unique<QObject>();
    // Declared after the binding's owner, so the pipeline stops first.
    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(
            QStringLiteral("videotestsrc %1 num-buffers=%2 ! %3 ! qgcvideosinkbin name=sink gpu-zerocopy=%4")
                .arg(QString::fromUtf8(sourceProperties))
                .arg(numBuffers)
                .arg(QString::fromUtf8(capsLine))
                .arg(gpuZerocopy ? "true" : "false"))) {
        r.errorMessage = pipeline.error();
        return r;
    }
    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    if (!sinkBin || !GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, owner.get())) {
        r.errorMessage = QStringLiteral("setupQVideoSinkElement() failed");
        return r;
    }
    if (!owner->findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly)) {
        r.errorMessage = QStringLiteral("controller not created by setupQVideoSinkElement()");
        return r;
    }
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, owner.get(), [&](const QVideoFrame& f) {
        ++r.frameCount;
        r.lastFormat = f.surfaceFormat();
        r.lastStartTime = f.startTime();
        if (captureCenterPixel) {
            const QImage image = f.toImage();
            if (!image.isNull()) {
                r.centerPixel = image.pixelColor(image.width() / 2, image.height() / 2);
                r.centerPixelValid = true;
            }
        }
    });

    if (!pipeline.play()) {
        r.errorMessage = QStringLiteral("set_state(PLAYING) failed");
        return r;
    }
    const GStreamer::GstMessagePtr message = pipeline.waitForEosOrError();
    r.eos = message && (GST_MESSAGE_TYPE(message.get()) == GST_MESSAGE_EOS);
    if (!message) {
        r.errorMessage = QStringLiteral("timeout waiting for EOS");
    } else if (!r.eos) {
        r.errorMessage = GstTest::errorText(message.get(), /*withDetails=*/false);
    }
    // Frames reach the QVideoSink through queued calls to this thread, and the counting connection dies with owner
    // on return, so the caller only sees frames delivered before then.
    if (r.eos) {
        (void) UnitTest::waitForCondition([&r] { return r.frameCount > 0; }, TestTimeout::mediumDuration(),
                                          u"frames delivered to the QVideoSink");
    }
    return r;
}

}  // namespace

void GStreamerTest::_testAppsinkFrameDelivery()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(videoSink,
                                                "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                                "videoconvert ! video/x-raw,format=BGRA",
                                                /*numBuffers=*/10);
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.frameSize(), QSize(320, 240));
}

void GStreamerTest::_testAppsinkYuvPassthrough()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(
        videoSink, "video/x-raw,format=I420,width=320,height=240,framerate=30/1", /*numBuffers=*/10);
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_YUV420P);
}

void GStreamerTest::_testAppsinkPtsAndColorimetry()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(
        videoSink, "video/x-raw,format=I420,width=64,height=48,framerate=30/1,colorimetry=(string)bt709",
        /*numBuffers=*/5, /*gpuZerocopy=*/false, "do-timestamp=true");
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QVERIFY2(r.lastStartTime >= 0, "GstBuffer PTS not forwarded to QVideoFrame::startTime");
    QCOMPARE(r.lastFormat.colorSpace(), QVideoFrameFormat::ColorSpace_BT709);
}

void GStreamerTest::_testQgcVideoSinkBinGpuZeroCopyProperty()
{
    const auto makeBin = [](gboolean gpuZeroCopy, gboolean disablePar) {
        GstElement* const bin = gst_element_factory_make_full("qgcvideosinkbin", "gpu-zerocopy", gpuZeroCopy,
                                                              "disable-par", disablePar, nullptr);
        return GStreamer::adoptElement(bin ? GST_ELEMENT(gst_object_ref_sink(bin)) : nullptr);
    };

    {
        const GStreamer::GstElementPtr bin = makeBin(FALSE, FALSE);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (CPU branch)");

        gboolean prop = TRUE;
        g_object_get(bin.get(), "gpu-zerocopy", &prop, NULL);
        QCOMPARE(prop, FALSE);

        const GStreamer::GstElementPtr qvideosink =
            GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(bin.get()), "qgcqvideosink"));
        QVERIFY2(qvideosink, "qgcqvideosink missing from CPU bin");

        gboolean sync = TRUE;
        gboolean qos = TRUE;
        guint64 processingDeadline = 0;
        g_object_get(bin.get(), "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, FALSE);
        QCOMPARE(qos, FALSE);
        QCOMPARE(processingDeadline, G_GUINT64_CONSTANT(20000000));

        g_object_get(qvideosink.get(), "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, FALSE);
        QCOMPARE(qos, FALSE);
        QCOMPARE(processingDeadline, G_GUINT64_CONSTANT(20000000));

        constexpr guint64 kUpdatedDeadline = G_GUINT64_CONSTANT(1234567);
        g_object_set(bin.get(), "sync", TRUE, "qos", TRUE, "processing-deadline", kUpdatedDeadline, NULL);

        g_object_get(bin.get(), "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, TRUE);
        QCOMPARE(qos, TRUE);
        QCOMPARE(processingDeadline, kUpdatedDeadline);

        g_object_get(qvideosink.get(), "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline, NULL);
        QCOMPARE(sync, TRUE);
        QCOMPARE(qos, TRUE);
        QCOMPARE(processingDeadline, kUpdatedDeadline);

        // CPU branch: videoconvert, the PAR=1/1 and format capsfilters, and qgcqvideosink.
        QCOMPARE(GstTest::childFactoryNames(GST_BIN(bin.get())),
                 (QStringList{QStringLiteral("capsfilter"), QStringLiteral("capsfilter"),
                              QStringLiteral("qgcqvideosink"), QStringLiteral("videoconvert")}));
    }

    {
        // disable-par drops the PAR capsfilter.
        const GStreamer::GstElementPtr bin = makeBin(FALSE, TRUE);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (CPU branch, disable-par=TRUE)");
        QCOMPARE(GstTest::childFactoryNames(GST_BIN(bin.get())),
                 (QStringList{QStringLiteral("capsfilter"), QStringLiteral("qgcqvideosink"),
                              QStringLiteral("videoconvert")}));
    }

#if defined(QGC_HAS_ANY_GPU_PATH)
    {
        const GStreamer::GstElementPtr bin = makeBin(TRUE, FALSE);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (GPU branch)");

        gboolean prop = FALSE;
        g_object_get(bin.get(), "gpu-zerocopy", &prop, NULL);
        QCOMPARE(prop, TRUE);
        QVERIFY2(GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(bin.get()), "qgcqvideosink")),
                 "qgcqvideosink missing from GPU bin");
    }
#endif
}

namespace {

/// Memory features of @p caps' structures in order, consecutive duplicates collapsed.
QStringList capsMemoryFeatures(const GstCaps* caps)
{
    QStringList features;
    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        const GstCapsFeatures* f = gst_caps_get_features(caps, i);
        const GStreamer::GCharPtr text = GStreamer::adoptGChar(f ? gst_caps_features_to_string(f) : nullptr);
        const QString feature = QString::fromUtf8(text ? text.get() : GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY);
        if (features.isEmpty() || (features.last() != feature)) {
            features << feature;
        }
    }
    return features;
}

}  // namespace

void GStreamerTest::_testQgcVideoSinkBinGpuBinVariant_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QStringList>("children");
    QTest::addColumn<QStringList>("memoryFeatures");
    QTest::addColumn<bool>("singlePlaneRgb2D");

    const QString systemMemory = QStringLiteral("memory:SystemMemory");
    const QStringList directChildren{QStringLiteral("capsfilter"), QStringLiteral("qgcqvideosink")};
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    // glcolorconvert turns external-OES into texture-target=2D; Qt samples multi-plane GLMemory as green frames.
    QTest::newRow("glupload") << int(HwBuffers::GpuBinMode::GlUpload)
                              << QStringList{QStringLiteral("capsfilter"), QStringLiteral("glcolorconvert"),
                                             QStringLiteral("glupload"), QStringLiteral("qgcqvideosink")}
                              << QStringList{QStringLiteral("memory:GLMemory")} << true;
#endif
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    // DMA_DRM fragments carry memory:DMABuf too; GLMemory stays out so gst-gl can't take the import over.
    QTest::newRow("direct-dmabuf") << int(HwBuffers::GpuBinMode::DirectDmaBuf) << directChildren
                                   << QStringList{QStringLiteral("memory:DMABuf"), systemMemory} << false;
#endif
#if !defined(QGC_GST_BIN_HAS_GLUPLOAD) && !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    // Every compiled native memory in caps order, D3D ahead of GLMemory, then the software-decoder fallback.
    QStringList nativeFeatures;
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    nativeFeatures << QStringLiteral("memory:D3D11Memory");
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    nativeFeatures << QStringLiteral("memory:D3D12Memory");
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    nativeFeatures << QStringLiteral("memory:GLMemory");
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    nativeFeatures << QStringLiteral("memory:AHardwareBuffer");
#endif
    QTest::newRow("passthrough") << int(HwBuffers::GpuBinMode::Passthrough) << directChildren
                                 << (nativeFeatures << systemMemory) << false;
#endif
}

void GStreamerTest::_testQgcVideoSinkBinGpuBinVariant()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else
    QFETCH(int, mode);
    QFETCH(QStringList, children);
    QFETCH(QStringList, memoryFeatures);
    QFETCH(bool, singlePlaneRgb2D);

    const HwBuffers::ScopedGpuBinModeForTest binMode(static_cast<HwBuffers::GpuBinMode>(mode));

    GstElement* element = gst_element_factory_make_full("qgcvideosinkbin", "gpu-zerocopy", TRUE, nullptr);
    QVERIFY2(element, "Failed to create qgcvideosinkbin (GPU branch)");
    const GStreamer::GstElementPtr bin = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(element)));
    QCOMPARE(GstTest::childFactoryNames(GST_BIN(bin.get())), children);

    const GStreamer::GstElementPtr formatFilter =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(bin.get()), "qgc-format-filter"));
    QVERIFY2(formatFilter, "GPU bin missing qgc-format-filter");
    GstCaps* rawCaps = nullptr;
    g_object_get(formatFilter.get(), "caps", &rawCaps, nullptr);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(rawCaps);
    QVERIFY2(caps, "GPU bin format capsfilter has null caps");
    QCOMPARE(capsMemoryFeatures(caps.get()), memoryFeatures);

    if (!singlePlaneRgb2D) {
        return;
    }
    for (guint i = 0; i < gst_caps_get_size(caps.get()); ++i) {
        const GstStructure* structure = gst_caps_get_structure(caps.get(), i);
        QCOMPARE(QString::fromUtf8(gst_structure_get_string(structure, "texture-target")), QStringLiteral("2D"));
        const GValue* formats = gst_structure_get_value(structure, "format");
        QVERIFY2(formats && GST_VALUE_HOLDS_LIST(formats), "GLMemory caps format field is not a list");
        for (guint f = 0; f < gst_value_list_get_size(formats); ++f) {
            const char* name = g_value_get_string(gst_value_list_get_value(formats, f));
            const GstVideoFormatInfo* info = gst_video_format_get_info(gst_video_format_from_string(name));
            QVERIFY2(info && GST_VIDEO_FORMAT_INFO_IS_RGB(info) && (GST_VIDEO_FORMAT_INFO_N_PLANES(info) == 1),
                     qPrintable(QStringLiteral("GLMemory caps offer non-single-plane-RGB format %1").arg(name)));
        }
    }
#endif
}

void GStreamerTest::_testQgcVideoSinkBinRejectsFailedAdopt()
{
    const GStreamer::GstElementPtr target =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_bin_new("qgc-adopt-target"))));
    const GStreamer::GstElementPtr parent =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_bin_new("qgc-adopt-existing-parent"))));
    GstElement* const element = gst_element_factory_make("identity", "preparented");
    QVERIFY(element);
    QVERIFY(gst_bin_add(GST_BIN(parent.get()), element));

    GstElement* adopted = nullptr;
    {
        GstQgc::BinChain chain(GST_BIN(target.get()));
        // adopt() consumes a ref even on failure.
        adopted = chain.adopt(GST_ELEMENT(gst_object_ref(element)));
        chain.commit();
    }

    QVERIFY2(!adopted, "BinChain::adopt must reject elements that gst_bin_add() did not accept");
    const GStreamer::GstObjectPtr currentParent = GStreamer::adoptObject(gst_object_get_parent(GST_OBJECT(element)));
    QCOMPARE(currentParent.get(), GST_OBJECT(parent.get()));
}

void GStreamerTest::_testQgcBinChainFailedAdoptDropsFloatingRefOnce()
{
    const GStreamer::GstElementPtr target =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_bin_new("qgc-adopt-target"))));
    QVERIFY(gst_bin_add(GST_BIN(target.get()), gst_element_factory_make("identity", "taken")));

    // Floating, as from gst_element_factory_make(), plus a plain ref of the test's own.
    GstElement* const duplicate = gst_element_factory_make("identity", "taken");
    QVERIFY(duplicate);
    (void) g_object_ref(duplicate);
    bool destroyed = false;
    g_object_weak_ref(
        G_OBJECT(duplicate), [](gpointer data, GObject*) { *static_cast<bool*>(data) = true; }, &destroyed);

    {
        GstQgc::BinChain chain(GST_BIN(target.get()));
        QVERIFY(!chain.adopt(duplicate));
    }

    QVERIFY2(!destroyed, "a failed BinChain::adopt must drop the floating ref exactly once");
    gst_object_unref(duplicate);
    QVERIFY2(destroyed, "a failed BinChain::adopt must consume the floating ref");
}

void GStreamerTest::_testCapsCacheInvalidation()
{
    QVideoSink videoSink;

    auto r1 = runPipelineThroughQVideoSink(videoSink,
                                           "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                           "videoconvert ! video/x-raw,format=BGRA");
    QVERIFY2(r1.eos, qUtf8Printable(QStringLiteral("Session 1: %1").arg(r1.errorMessage)));
    QVERIFY(r1.frameCount > 0);
    QCOMPARE(r1.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);

    auto r2 = runPipelineThroughQVideoSink(videoSink,
                                           "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                           "videoconvert ! video/x-raw,format=RGBA");
    QVERIFY2(r2.eos, qUtf8Printable(QStringLiteral("Session 2: %1").arg(r2.errorMessage)));
    QVERIFY(r2.frameCount > 0);
    QCOMPARE(r2.lastFormat.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
}

void GStreamerTest::_testGpuZeroCopyFallback_data()
{
    QTest::addColumn<int>("mode");
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    QTest::newRow("glupload") << int(HwBuffers::GpuBinMode::GlUpload);
#endif
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QTest::newRow("direct-dmabuf") << int(HwBuffers::GpuBinMode::DirectDmaBuf);
#endif
#if !defined(QGC_GST_BIN_HAS_GLUPLOAD) && !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QTest::newRow("passthrough") << int(HwBuffers::GpuBinMode::Passthrough);
#endif
}

void GStreamerTest::_testGpuZeroCopyFallback()
{
    QFETCH(int, mode);
    const HwBuffers::ScopedGpuBinModeForTest binMode(static_cast<HwBuffers::GpuBinMode>(mode));

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    const quint64 dmabufBefore =
        GstHwPathTelemetry::peek(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures);
#endif

    QVideoSink videoSink;
    auto r = runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1",
                                          /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    // The glupload bin needs an EGL/GL context, which a headless runner may not have.
    if (!r.eos && (r.errorMessage.contains(QStringLiteral("egl"), Qt::CaseInsensitive) ||
                   r.errorMessage.contains(QStringLiteral("gl context"), Qt::CaseInsensitive))) {
        QSKIP(qPrintable(
            QStringLiteral("gpu-zerocopy pipeline needs a GL context (headless env): %1").arg(r.errorMessage)));
    }
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QCOMPARE(GstHwPathTelemetry::peek(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures),
             dmabufBefore);
#endif
}

void GStreamerTest::_testWindowsGpuSinkPreservesSoftwarePixels()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

    QVideoSink videoSink;
    const auto result =
        runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1",
                                     /*numBuffers*/ 5, /*gpuZerocopy*/ true, "pattern=red",
                                     /*captureCenterPixel*/ true);

    QVERIFY2(result.eos, qUtf8Printable(QStringLiteral("Software-frame pipeline: %1").arg(result.errorMessage)));
    QVERIFY(result.frameCount > 0);
    QCOMPARE(result.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);
    QVERIFY2(result.centerPixelValid, "System-memory fallback frame could not be mapped for pixel verification");
    QVERIFY2((result.centerPixel.red() > 200) && (result.centerPixel.green() < 50) && (result.centerPixel.blue() < 50),
             qUtf8Printable(QStringLiteral("Expected a red software frame, received RGB(%1,%2,%3)")
                                .arg(result.centerPixel.red())
                                .arg(result.centerPixel.green())
                                .arg(result.centerPixel.blue())));
#else
    QSKIP("Windows D3D11 GPU sink path is unavailable in this build");
#endif
}

void GStreamerTest::_testWindowsGpuSinkPreservesDirectD3D11Memory()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

    GstElementFactory* uploadFactory = gst_element_factory_find("d3d11upload");
    if (!uploadFactory) {
        QSKIP("d3d11upload is unavailable");
    }
    gst_object_unref(uploadFactory);

    QVideoSink videoSink;
    auto result =
        runPipelineThroughQVideoSink(videoSink,
                                     "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! d3d11upload ! "
                                     "video/x-raw(memory:D3D11Memory),format=NV12",
                                     /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    if (!result.eos && result.errorMessage.contains(QStringLiteral("d3d11"), Qt::CaseInsensitive)) {
        QSKIP(qPrintable(QStringLiteral("D3D11 device unavailable in this runner: %1").arg(result.errorMessage)));
    }

    QVERIFY2(result.eos, qUtf8Printable(QStringLiteral("D3D11 pipeline: %1").arg(result.errorMessage)));
    QVERIFY(result.frameCount > 0);
    QCOMPARE(result.lastFormat.pixelFormat(), QVideoFrameFormat::Format_NV12);
#else
    QSKIP("D3D11 GPU sink path is only available in supported Windows builds");
#endif
}

void GStreamerTest::_testAppsinkTeardownUnderLoad()
{
    QVideoSink videoSink;

    {
        QObject controllerOwner;
        GstTest::TestPipeline pipeline;
        QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc is-live=true ! "
                                                "video/x-raw,format=BGRA,width=160,height=120,framerate=60/1 ! "
                                                "qgcvideosinkbin name=sink")),
                 qPrintable(pipeline.error()));
        const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
        QVERIFY(sinkBin);
        QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &controllerOwner));
        const GStreamer::GstElementPtr qvideosink =
            GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
        QVERIFY(qvideosink);

        QVERIFY(pipeline.play());
        QVERIFY_TRUE_WAIT(framesDelivered(qvideosink.get()) > 0, TestTimeout::mediumMs());
    }

    auto r = runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=RGBA,width=160,height=120,framerate=30/1");
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Restart: %1").arg(r.errorMessage)));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
}

void GStreamerTest::_testFrameCountsTelemetrySignal()
{
    // Telemetry counters are process-global and leak across tests; drain them so the
    // assertions below measure only this pipeline's contribution.
    for (auto p :
         {HwVideoBufferPath::None, HwVideoBufferPath::DmaBuf, HwVideoBufferPath::GlMemory, HwVideoBufferPath::D3D11,
          HwVideoBufferPath::D3D12, HwVideoBufferPath::IOSurface, HwVideoBufferPath::AHardwareBuffer}) {
        (void) GstHwPathTelemetry::take(p, GstHwPathTelemetry::Counter::MapFailures);
    }

    QVideoSink videoSink;
    QObject controllerOwner;
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc is-live=false num-buffers=90 ! "
                                            "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1 ! "
                                            "qgcvideosinkbin name=sink")),
             qPrintable(pipeline.error()));

    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(sinkBin);
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &controllerOwner));
    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
    QVERIFY(qvideosink);

    auto* controller = controllerOwner.findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(controller);
    QSignalSpy spy(controller, &QGCQVideoSinkController::frameCountsChanged);

    QVERIFY(pipeline.play());
    QVERIFY_SIGNAL_COUNT_WAIT(spy, 1, TestTimeout::mediumMs());

    QVERIFY(sinkStat(qvideosink.get(), GstQgc::sinkFramesField(HwVideoBufferPath::None)) > 0);
    quint64 gpuFallback = 0;
    for (auto p : {HwVideoBufferPath::DmaBuf, HwVideoBufferPath::GlMemory, HwVideoBufferPath::D3D11,
                   HwVideoBufferPath::D3D12, HwVideoBufferPath::IOSurface, HwVideoBufferPath::AHardwareBuffer}) {
        gpuFallback += GstHwPathTelemetry::peek(p, GstHwPathTelemetry::Counter::MapFailures);
    }
    QCOMPARE(gpuFallback, quint64(0));
}

void GStreamerTest::_testInactiveQgcQVideoSinkDropsAndCounts()
{
    QVideoSink videoSink;
    QObject controllerOwner;
    QSignalSpy frameSpy(&videoSink, &QVideoSink::videoFrameChanged);
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc is-live=false num-buffers=12 ! "
                                            "video/x-raw,format=BGRA,width=96,height=64,framerate=30/1 ! "
                                            "qgcvideosinkbin name=sink")),
             qPrintable(pipeline.error()));

    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(sinkBin);
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &controllerOwner));
    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
    QVERIFY(qvideosink);

    g_object_set(qvideosink.get(), "active", FALSE, nullptr);

    QVERIFY(pipeline.play());
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    // Delivery is queued to this thread, so one pass of its event loop would show a frame; 100 ms is plenty.
    QVERIFY_NO_SIGNAL_WAIT(frameSpy, 100);

    guint64 input = 0;
    guint64 dropped = 0;
    guint64 delivered = 0;
    g_object_get(qvideosink.get(), "frames-input", &input, "frames-dropped", &dropped, "frames-delivered", &delivered,
                 nullptr);
    QVERIFY(input >= guint64(12));
    QCOMPARE(dropped, input);
    QCOMPARE(delivered, guint64(0));
}

void GStreamerTest::_testGetAppsinkAccessor()
{
    // Construction is synchronous (GObject::constructed runs inside factory_make).
    const SinkBin sink = makeSinkBin();
    QVERIFY2(sink.bin, "qgcvideosinkbin factory not found");
    QVERIFY2(sink.qvideosink, "qvideosink accessor returned NULL after factory_make");
    QVERIFY(GST_IS_ELEMENT(sink.qvideosink.get()));

    // After transitioning to READY the accessor must still return a valid element.
    const auto stopBin = qScopeGuard([&sink] { (void) gst_element_set_state(sink.bin.get(), GST_STATE_NULL); });
    QVERIFY(gst_element_set_state(sink.bin.get(), GST_STATE_READY) != GST_STATE_CHANGE_FAILURE);
    const GStreamer::GstElementPtr afterReady =
        GStreamer::adoptElement(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(sink.bin.get())));
    QVERIFY2(afterReady, "qvideosink accessor returned NULL after READY");
    QVERIFY(GST_IS_ELEMENT(afterReady.get()));
}

void GStreamerTest::_testQVideoSinkControllerClearsElementOnDestroy()
{
    const SinkBin sink = makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const bin = sink.bin.get();
    GstElement* const qvideosink = sink.qvideosink.get();

    QVideoSink videoSink;
    auto controllerOwner = std::make_unique<QObject>();
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, &videoSink, controllerOwner.get()),
             "setupQVideoSinkElement() failed");

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(&videoSink));
    QCOMPARE(active, TRUE);

    controllerOwner.reset();

    installedSink = &videoSink;
    active = TRUE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);
}

void GStreamerTest::_testQVideoSinkControllerClearsElementWhenVideoSinkDestroyed()
{
    const SinkBin sink = makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const bin = sink.bin.get();
    GstElement* const qvideosink = sink.qvideosink.get();

    auto videoSink = std::make_unique<QVideoSink>();
    QObject controllerOwner;
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, videoSink.get(), &controllerOwner),
             "setupQVideoSinkElement() failed");

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(videoSink.get()));
    QCOMPARE(active, TRUE);

    videoSink.reset();
    QCoreApplication::sendPostedEvents(&controllerOwner, QEvent::MetaCall);

    installedSink = reinterpret_cast<gpointer>(quintptr(0x1));
    active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);
}

void GStreamerTest::_testQVideoSinkControllerNullSinkStillDeactivatesOnDestroy()
{
    const SinkBin sink = makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const bin = sink.bin.get();
    GstElement* const qvideosink = sink.qvideosink.get();

    QVideoSink videoSink;
    auto controllerOwner = std::make_unique<QObject>();
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, &videoSink, controllerOwner.get()),
             "setupQVideoSinkElement() failed");

    auto* controller = controllerOwner->findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(controller);

    controller->setVideoSink(QPointer<QVideoSink>());

    gpointer installedSink = &videoSink;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);

    controllerOwner.reset();

    active = FALSE;
    g_object_get(qvideosink, "active", &active, nullptr);
    QCOMPARE(active, FALSE);
}

void GStreamerTest::_testQVideoSinkControllerRepeatedSetupKeepsNewBindingActive()
{
    const SinkBin sink = makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const bin = sink.bin.get();
    GstElement* const qvideosink = sink.qvideosink.get();

    auto firstSink = std::make_unique<QVideoSink>();
    auto secondSink = std::make_unique<QVideoSink>();
    QObject controllerOwner;

    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, firstSink.get(), &controllerOwner),
             "initial setupQVideoSinkElement() failed");
    auto* firstController =
        controllerOwner.findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(firstController);

    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, secondSink.get(), &controllerOwner),
             "repeated setupQVideoSinkElement() failed");

    firstController->setActive(false);

    firstSink.reset();
    QCoreApplication::sendPostedEvents(&controllerOwner, QEvent::MetaCall);

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(secondSink.get()));
    QCOMPARE(active, TRUE);

    QCoreApplication::sendPostedEvents(&controllerOwner, QEvent::DeferredDelete);

    installedSink = nullptr;
    active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(secondSink.get()));
    QCOMPARE(active, TRUE);
}

void GStreamerTest::_testQVideoSinkControllerNoWindowStartsInactive()
{
    const SinkBin sink = makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const qvideosink = sink.qvideosink.get();

    QObject controllerOwner;
    QGCQVideoSinkController controller(qvideosink, &controllerOwner);
    controller.setActive(true);

    QQuickVideoOutput videoOutput;
    QVERIFY(videoOutput.window() == nullptr);
    QGCQVideoSinkController::syncActiveToWindowVisibility(&controllerOwner, &videoOutput);

    gboolean active = TRUE;
    g_object_get(qvideosink, "active", &active, nullptr);
    QCOMPARE(active, FALSE);
}

void GStreamerTest::_testCpuZeroCopyFrameRejectsWritableMap()
{
    GstVideoInfo info;
    gst_video_info_init(&info);
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 4, 4);

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    auto bufferGuard = qScopeGuard([&] { gst_buffer_unref(buffer); });

    GstMapInfo mapInfo;
    QVERIFY(gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE));
    std::memset(mapInfo.data, 0x7f, mapInfo.size);
    gst_buffer_unmap(buffer, &mapInfo);

    QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_BGRA8888);
    auto wrapped = CpuVideoFramePool::wrapZeroCopy(buffer, info, format, std::make_shared<std::atomic<int>>(0));
    QVERIFY(wrapped);

    QVideoFrame frame(std::move(wrapped));
    QVERIFY(frame.isValid());

    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    frame.unmap();

    QVERIFY(!frame.map(QVideoFrame::WriteOnly));
    QVERIFY(!frame.map(QVideoFrame::ReadWrite));
}

void GStreamerTest::_testCpuZeroCopyPinnedFramesCappedPerSink()
{
    GstVideoInfo info;
    gst_video_info_init(&info);
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 4, 4);
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    auto bufferGuard = qScopeGuard([&] { gst_buffer_unref(buffer); });
    const QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_BGRA8888);

    const auto sinkA = std::make_shared<std::atomic<int>>(0);
    const auto sinkB = std::make_shared<std::atomic<int>>(0);
    std::vector<std::unique_ptr<QAbstractVideoBuffer>> pinned;
    for (int i = 0; i < CpuVideoFramePool::kMaxPinnedFrames; ++i) {
        pinned.push_back(CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkA));
        QVERIFY(pinned.back());
    }

    // At the cap the caller must copy instead; another sink's budget is independent.
    QVERIFY(!CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkA));
    QVERIFY(CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkB));

    // Releasing a frame (Qt dropping it) frees its slot.
    pinned.pop_back();
    QVERIFY(CpuVideoFramePool::wrapZeroCopy(buffer, info, format, sinkA));
}

void GStreamerTest::_testCpuMemcpyActiveRowStrideHandling()
{
    // Width 753 is not 4-byte aligned; GStreamer will pad stride to 756.
    // If the memcpy copies stride bytes instead of active-row bytes, the
    // right-edge pixels of the right-most component will contain padding zeros.
    QVideoSink videoSink;
    QObject controllerOwner;

    QVideoFrame capturedFrame;
    // Controller teardown clears the QVideoSink with an invalid frame; keep the last real one.
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner, [&](const QVideoFrame& f) {
        if (f.isValid()) {
            capturedFrame = f;
        }
    });

    auto r = runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=BGRA,width=753,height=432,framerate=30/1",
                                          /*numBuffers=*/5);
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QVERIFY(capturedFrame.isValid());

    QVERIFY(capturedFrame.map(QVideoFrame::ReadOnly));
    const uchar* data = capturedFrame.bits(0);
    const int stride = capturedFrame.bytesPerLine(0);
    // Sample the last active pixel column (x=752) from row 0; BGRA so 4 bytes/pixel.
    const uchar* lastPixel = data + 752 * 4;
    // videotestsrc pattern=0 (smpte) fills with non-zero color in top rows.
    const bool nonZero = (lastPixel[0] | lastPixel[1] | lastPixel[2] | lastPixel[3]) != 0;
    capturedFrame.unmap();

    (void) stride;
    QVERIFY2(nonZero, "Last active column is all zeros — likely stride vs active-row memcpy bug");
}

void GStreamerTest::_testColorimetryPixelFormatMapping()
{
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV12), QVideoFrameFormat::Format_NV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV21), QVideoFrameFormat::Format_NV21);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420), QVideoFrameFormat::Format_YUV420P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YV12), QVideoFrameFormat::Format_YV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y42B), QVideoFrameFormat::Format_YUV422P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P010_10LE), QVideoFrameFormat::Format_P010);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_AYUV), QVideoFrameFormat::Format_AYUV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YUY2), QVideoFrameFormat::Format_YUYV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UYVY), QVideoFrameFormat::Format_UYVY);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY8), QVideoFrameFormat::Format_Y8);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY16_LE), QVideoFrameFormat::Format_Y16);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRA), QVideoFrameFormat::Format_BGRA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBA), QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420_10LE), QVideoFrameFormat::Format_YUV420P10);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P016_LE), QVideoFrameFormat::Format_P016);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y444), QVideoFrameFormat::Format_Invalid);
}

void GStreamerTest::_testCpuCapsFormatsRoundTripToQt()
{
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(GstQgc::buildCpuCapsString().c_str()));
    QVERIFY2(caps, "buildCpuCapsString() did not parse");
    const GstStructure* s = gst_caps_get_structure(caps.get(), 0);
    const GValue* fmt = gst_structure_get_value(s, "format");
    QVERIFY2(fmt && GST_VALUE_HOLDS_LIST(fmt), "CPU caps format field is not a list");
    const guint n = gst_value_list_get_size(fmt);
    QVERIFY2(n > 0, "CPU caps format list empty");
    for (guint i = 0; i < n; ++i) {
        const char* name = g_value_get_string(gst_value_list_get_value(fmt, i));
        const GstVideoFormat gf = gst_video_format_from_string(name);
        QVERIFY2(gf != GST_VIDEO_FORMAT_UNKNOWN,
                 qPrintable(QStringLiteral("caps lists unknown gst format '%1'").arg(name)));
        QVERIFY2(toQtPixelFormat(gf) != QVideoFrameFormat::Format_Invalid,
                 qPrintable(QStringLiteral("format '%1' advertised but not Qt-renderable").arg(name)));
    }
}

void GStreamerTest::_testAllocationQueryHwMemoryPoolHint()
{
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "video/x-raw(memory:DMABuf), format=(string)NV12, width=(int)64, height=(int)64, framerate=(fraction)30/1"));
    QVERIFY2(caps, "HW caps did not parse");

    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), TRUE));
    GstQgc::populateAllocationQuery(query.get());

    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), 1U);
    GstBufferPool* pool = nullptr;
    guint size = 0, minBuffers = 0, maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query.get(), 0, &pool, &size, &minBuffers, &maxBuffers);
    QVERIFY2(pool == nullptr, "GPU path must not propose a generic pool for native memory");
    QVERIFY2(minBuffers > guint(CpuVideoFramePool::kMaxPinnedFrames),
             "min-buffer hint must leave the decoder a buffer beyond those the sink and Qt hold");
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "VideoMeta not advertised on GPU path");
}

void GStreamerTest::_testQgcVideoSinkBinAllocationQueryAdvertisesVideoMeta()
{
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    const HwBuffers::ScopedGpuBinModeForTest directBin(HwBuffers::GpuBinMode::DirectDmaBuf);
#elif defined(QGC_GST_BIN_HAS_GLUPLOAD)
    QSKIP("Direct-DMABuf ALLOCATION VideoMeta contract does not apply to the glupload bin");
#endif

    GstElement* const element = gst_element_factory_make_full("qgcvideosinkbin", "gpu-zerocopy", TRUE, nullptr);
    QVERIFY2(element, "qgcvideosinkbin factory did not create an element");
    const GStreamer::GstElementPtr bin = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(element)));
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(bin.get(), "sink"));
    QVERIFY2(sinkPad, "qgcvideosinkbin has no sink pad");
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "video/x-raw(memory:DMABuf), format=(string)NV12, width=(int)64, height=(int)64, framerate=(fraction)30/1"));
    QVERIFY2(caps, "HW caps did not parse");
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), TRUE));
    QVERIFY2(query, "Could not allocate ALLOCATION query");

    QVERIFY2(gst_pad_query(sinkPad.get(), query.get()), "qgcvideosinkbin did not answer ALLOCATION query");
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "qgcvideosinkbin ALLOCATION query must advertise VideoMeta for gst-va DMABuf");

    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), 1U);
    GstBufferPool* pool = nullptr;
    guint size = 0, minBuffers = 0, maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query.get(), 0, &pool, &size, &minBuffers, &maxBuffers);
    QVERIFY2(pool == nullptr, "qgcvideosinkbin must not propose a generic pool for native memory");
    QVERIFY2(minBuffers > 0, "qgcvideosinkbin min-buffer hint must be non-zero");
}

void GStreamerTest::_testQgcQVideoSinkSystemMemoryAllocation_data()
{
    QTest::addColumn<bool>("needPool");

    QTest::newRow("pool needed") << true;
    // When upstream does not need a pool, the sink must not force one.
    QTest::newRow("no pool needed") << false;
}

void GStreamerTest::_testQgcQVideoSinkSystemMemoryAllocation()
{
    QFETCH(bool, needPool);

    GstElement* const element = gst_element_factory_make("qgcqvideosink", nullptr);
    QVERIFY2(element, "qgcqvideosink factory not found");
    const GStreamer::GstElementPtr sink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(element)));
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(sink.get(), "sink"));
    QVERIFY(sinkPad);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "video/x-raw, format=(string)BGRA, width=(int)64, height=(int)64, framerate=(fraction)30/1"));
    QVERIFY2(caps, "System-memory caps did not parse");
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), needPool));

    QVERIFY2(gst_pad_query(sinkPad.get(), query.get()), "qgcqvideosink did not answer ALLOCATION");
    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), needPool ? 1U : 0U);
    if (needPool) {
        GstBufferPool* pool = nullptr;
        gst_query_parse_nth_allocation_pool(query.get(), 0, &pool, nullptr, nullptr, nullptr);
        const GStreamer::GstObjectPtr ownedPool = GStreamer::adoptObject(GST_OBJECT(pool));
        QVERIFY2(pool, "need_pool=TRUE must get a pool proposal");
        QVERIFY2(gst_buffer_pool_has_option(pool, GST_BUFFER_POOL_OPTION_VIDEO_META),
                 "proposed pool must support VIDEO_META so decoders can write into it directly");
    }
    // Crop, orientation and video metadata must survive into GstQgcFrameMap either way.
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "VideoMeta not advertised for system-memory caps");
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_CROP_META_API_TYPE, nullptr),
             "VideoCropMeta not advertised for system-memory caps");
}

void GStreamerTest::_testColorimetryColorSpaceMapping()
{
    // Mirrors Qt 6.10.3 qgst.cpp: SMPTE240M maps to AdobeRgb, FCC remains Undefined.
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_SMPTE240M), QVideoFrameFormat::ColorSpace_AdobeRgb);
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_FCC), QVideoFrameFormat::ColorSpace_Undefined);
}

void GStreamerTest::_testColorimetryResolutionHeuristicMatchesQt()
{
    const auto inferredColorSpace = [](int height) {
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_I420, 1280, height);
        info.colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_UNKNOWN;
        info.colorimetry.range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
        info.colorimetry.transfer = GST_VIDEO_TRANSFER_UNKNOWN;

        QVideoFrameFormat format(QSize(1280, height), QVideoFrameFormat::Format_YUV420P);
        applyColorimetry(format, info, nullptr);
        return format.colorSpace();
    };

    QCOMPARE(inferredColorSpace(576), QVideoFrameFormat::ColorSpace_BT601);
    QCOMPARE(inferredColorSpace(720), QVideoFrameFormat::ColorSpace_BT709);
}

void GStreamerTest::_testColorimetryTransferMapping()
{
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SRGB), QVideoFrameFormat::ColorTransfer_Gamma22);
    // Regression: BT601 used to fall through to BT709 — Qt's own backend maps it distinctly.
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT601), QVideoFrameFormat::ColorTransfer_BT601);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT709), QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT2020_10), QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE2084), QVideoFrameFormat::ColorTransfer_ST2084);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ARIB_STD_B67), QVideoFrameFormat::ColorTransfer_STD_B67);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA10), QVideoFrameFormat::ColorTransfer_Linear);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA28), QVideoFrameFormat::ColorTransfer_Gamma28);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE240M), QVideoFrameFormat::ColorTransfer_Gamma22);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ADOBERGB), QVideoFrameFormat::ColorTransfer_Gamma22);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_LOG100), QVideoFrameFormat::ColorTransfer_Unknown);
}

void GStreamerTest::_testColorimetryColorRangeMapping()
{
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_0_255), QVideoFrameFormat::ColorRange_Full);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_16_235), QVideoFrameFormat::ColorRange_Video);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_UNKNOWN), QVideoFrameFormat::ColorRange_Unknown);
}

void GStreamerTest::_testPixelFormatAcceptedButNotAdvertised()
{
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRx), QVideoFrameFormat::Format_BGRX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBx), QVideoFrameFormat::Format_RGBX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_ARGB), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_xRGB), QVideoFrameFormat::Format_XRGB8888);

    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGR), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGB), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UNKNOWN), QVideoFrameFormat::Format_Invalid);
}

void GStreamerTest::_testAdvertisedFormatListMatchesTable()
{
    const std::string list = GstQgc::advertisedFormatList();
    for (const auto& e : GstQgc::kVideoFormatTable) {
        if (e.capsToken) {
            QVERIFY2(
                list.find(e.capsToken) != std::string::npos,
                qPrintable(
                    QStringLiteral("advertised token '%1' missing from list").arg(QString::fromUtf8(e.capsToken))));
        }
    }
    QVERIFY2(list.find("BGRx") == std::string::npos, "accepted-only BGRx must not be advertised");
    QVERIFY2(list.find("I420_10LE") == std::string::npos, "accepted-only I420_10LE must not be advertised");
    QVERIFY2(list.rfind("{ ", 0) == 0 && list.find(" }") != std::string::npos, "list must be brace-wrapped");
}

void GStreamerTest::_testAdvertisedFormatListForOffer_data()
{
    QTest::addColumn<uint>("offer");
    QTest::addColumn<QString>("expected");

    // Multi-plane GLMemory renders green through Qt's GL import, so glupload must stay single-plane RGB.
    QTest::newRow("glupload") << uint(GstQgc::kOfferGlUpload) << QStringLiteral("{ BGRA, RGBA }");
    QTest::newRow("linear-dma-drm") << uint(GstQgc::kOfferLinearDmaDrm)
                                    << QStringLiteral("{ NV12, NV21, I420, YV12, P010_10LE, BGRA, RGBA }");
}

void GStreamerTest::_testAdvertisedFormatListForOffer()
{
    QFETCH(uint, offer);
    QFETCH(QString, expected);
    QCOMPARE(QString::fromStdString(GstQgc::advertisedFormatList(offer)), expected);
}

void GStreamerTest::_testColorimetryFrameRatePropagation()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=I420,width=64,height=48,framerate=30/1",
                                                /*numBuffers=*/3);
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.streamFrameRate(), 30.0);
}

void GStreamerTest::_testApplyOrientationToFrameMapping()
{
#ifndef QGC_HAS_GST_VIDEO_ORIENTATION_META
    QSKIP("GStreamer build lacks GstVideoOrientationMeta");
#else
    // Verifies each GStreamer orientation enum maps to the correct (rotation, mirrored) tuple
    // on QVideoFrame. Lock-down test: prior versions had subtle mismatches between gst's
    // diagonal-flip semantics and Qt's rotate-then-mirror composition.
    struct Case
    {
        GstVideoOrientationMethod gst;
        QtVideo::Rotation expectedRot;
        bool expectedMirrored;
    };

    const Case cases[] = {
        {GST_VIDEO_ORIENTATION_IDENTITY, QtVideo::Rotation::None, false},
        {GST_VIDEO_ORIENTATION_90R, QtVideo::Rotation::Clockwise90, false},
        {GST_VIDEO_ORIENTATION_180, QtVideo::Rotation::Clockwise180, false},
        {GST_VIDEO_ORIENTATION_90L, QtVideo::Rotation::Clockwise270, false},
        {GST_VIDEO_ORIENTATION_HORIZ, QtVideo::Rotation::None, true},
        {GST_VIDEO_ORIENTATION_VERT, QtVideo::Rotation::Clockwise180, true},
        {GST_VIDEO_ORIENTATION_UL_LR, QtVideo::Rotation::Clockwise90, true},
        {GST_VIDEO_ORIENTATION_UR_LL, QtVideo::Rotation::Clockwise270, true},
    };
    for (const Case& c : cases) {
        QVideoFrame frame = QVideoFrame(QVideoFrameFormat(QSize(2, 2), QVideoFrameFormat::Format_BGRA8888));
        // Pre-poison so a no-op switch case (default branch) wouldn't accidentally match.
        frame.setRotation(QtVideo::Rotation::Clockwise90);
        frame.setMirrored(true);
        applyOrientationToFrame(frame, c.gst);
        QCOMPARE(frame.rotation(), c.expectedRot);
        QCOMPARE(frame.mirrored(), c.expectedMirrored);
    }
#endif
}

void GStreamerTest::_testAdapterFlushDropsInFlightSamples()
{
    // Counts come from the element: latest-frame-only delivery coalesces videoFrameChanged emissions.
    const auto counts = [](GstElement* sink) {
        guint64 input = 0;
        guint64 delivered = 0;
        guint64 dropped = 0;
        g_object_get(sink, "frames-input", &input, "frames-delivered", &delivered, "frames-dropped", &dropped, nullptr);
        return std::array<guint64, 3>{input, delivered, dropped};
    };

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);

    // While flushing, the base sink refuses buffers before show_frame.
    QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
    const auto beforeFlushedFrame = counts(harness.element());
    QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_FLUSHING);
    QCOMPARE(counts(harness.element()), beforeFlushedFrame);

    // After the flush, every buffer the sink saw is accounted for as delivered or dropped, none left in flight.
    QVERIFY(harness.pushEvent(gst_event_new_flush_stop(/*reset_time=*/TRUE)));
    QVERIFY(harness.pushSegment());
    QCOMPARE(harness.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    const auto [input, delivered, dropped] = counts(harness.element());
    QCOMPARE(input, guint64(2));
    QCOMPARE(delivered + dropped, input);
}

void GStreamerTest::_testQgcQVideoSinkDeliversLatestFrameOnly()
{
    int emissions = 0;
    qint64 lastStartUs = -1;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame& frame) {
        ++emissions;
        lastStartUs = frame.startTime();
    });
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    // Three frames while the GUI thread is "stalled": all are handed off, but only the newest is shown. The first is
    // also the preroll buffer, which must not be shown twice.
    for (int i = 1; i <= 3; ++i) {
        QCOMPARE(harness.pushFrame(i * GST_SECOND), GST_FLOW_OK);
    }
    guint64 input = 0;
    g_object_get(harness.element(), "frames-input", &input, nullptr);
    QCOMPARE(input, guint64(3));
    QCOMPARE(harness.delivered(), guint64(3));
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 1);
    QCOMPARE(lastStartUs, qint64(3 * GST_SECOND / GST_USECOND));

    // Delivery re-arms once the GUI side has taken the frame.
    QCOMPARE(harness.pushFrame(4 * GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 2);
    QCOMPARE(lastStartUs, qint64(4 * GST_SECOND / GST_USECOND));
}

void GStreamerTest::_testQgcQVideoSinkRenegotiatesMidStream()
{
    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(videoSink.videoFrame().pixelFormat(), QVideoFrameFormat::Format_BGRA8888);

    // Same byte size as the first format, so a frame mapped with stale caps would still map and show the old layout.
    GstCaps* const caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGBA", "width", G_TYPE_INT, 8,
                                              "height", G_TYPE_INT, 2, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr);
    QVERIFY(harness.pushEvent(gst_event_new_caps(caps)));
    gst_caps_unref(caps);
    QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);

    const QVideoFrame frame = videoSink.videoFrame();
    QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(frame.size(), QSize(8, 2));
}

void GStreamerTest::_testQgcQVideoSinkAppliesCropMeta()
{
    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    GstBuffer* const buffer = QgcQVideoSinkHarness::makeFrame(GST_SECOND);
    GstVideoCropMeta* const crop = gst_buffer_add_video_crop_meta(buffer);
    crop->x = 1;
    crop->y = 1;
    crop->width = 2;
    crop->height = 3;
    QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);

    const QVideoFrameFormat format = videoSink.videoFrame().surfaceFormat();
    QCOMPARE(format.viewport(), QRect(1, 1, 2, 3));
    QCOMPARE(format.frameSize(), QSize(QgcQVideoSinkHarness::kWidth, QgcQVideoSinkHarness::kHeight));
}

void GStreamerTest::_testQgcQVideoSinkStatsArePerSink()
{
    QVideoSink videoSinkA;
    QVideoSink videoSinkB;
    QgcQVideoSinkHarness harnessA(&videoSinkA);
    QgcQVideoSinkHarness harnessB(&videoSinkB);
    QVERIFY(harnessA.isReady() && harnessB.isReady());

    QCOMPARE(harnessA.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harnessA.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harnessB.pushFrame(GST_SECOND), GST_FLOW_OK);

    const char* const cpuField = GstQgc::sinkFramesField(HwVideoBufferPath::None);
    QCOMPARE(sinkStat(harnessA.element(), cpuField), qint64(2));
    QCOMPARE(sinkStat(harnessB.element(), cpuField), qint64(1));
    QCOMPARE(framesDelivered(harnessB.element()), guint64(1));

    const GStreamer::GstStructurePtr stats = GstQgc::frameStats(harnessA.element());
    QVERIFY(stats);
    const GstQgc::PathStats formatted = GstQgc::formatSinkStats(stats.get());
    QCOMPARE(formatted.line, QStringLiteral(" CPU:2"));
    QCOMPARE(formatted.totalDelivered, quint64(0));

    // A receiver reuses its sink across sessions and reports each session from its start.
    QCOMPARE(harnessA.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    const GStreamer::GstStructurePtr later = GstQgc::frameStats(harnessA.element());
    QVERIFY(later);
    QCOMPARE(GstQgc::formatSinkStats(later.get(), stats.get()).line, QStringLiteral(" CPU:1"));
}

void GStreamerTest::_testQgcQVideoSinkKeepsBaseSinkStats()
{
    GstElement* const element = gst_element_factory_make("qgcqvideosink", nullptr);
    QVERIFY(element);
    const GStreamer::GstElementPtr sink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(element)));

    // Generic tools read GstBaseSink's "stats"; the sink's own accounting lives beside it in "frame-stats".
    QVERIFY(g_object_class_find_property(G_OBJECT_GET_CLASS(sink.get()), "frame-stats"));

    GstStructure* rawStats = nullptr;
    g_object_get(sink.get(), "stats", &rawStats, nullptr);
    const GStreamer::GstStructurePtr stats = GStreamer::adoptStructure(rawStats);
    QVERIFY(stats);
    QVERIFY(gst_structure_has_field(stats.get(), "rendered"));
}

void GStreamerTest::_testQgcQVideoSinkGpuPathSelection_data()
{
    QTest::addColumn<bool>("gpuZeroCopy");
    QTest::addColumn<QString>("memoryFeature");
    QTest::addColumn<bool>("fdMemory");
    QTest::addColumn<qint64>("expectedGpuFallbacks");
    QTest::addColumn<quint64>("expectedNoPathVerdicts");

    // Every row pushes three buffers, in system memory unless fdMemory.
    QTest::newRow("gpu-zerocopy-off") << false << QStringLiteral("memory:DMABuf") << false << qint64(0) << quint64(0);
    // Software-decoded frames never enter path selection.
    QTest::newRow("system-memory-caps") << true << QString() << false << qint64(0) << quint64(0);
    // Other memory under plain video/x-raw caps (vtdec's CoreVideo buffers, V4L2 dmabuf export) still tries the GPU
    // paths: the first frame finds no path for this allocator, the rest reuse that verdict.
    QTest::newRow("system-memory-caps-fd-buffers") << true << QString() << true << qint64(3) << quint64(1);
    QTest::newRow("gpu-caps-system-buffers")
        << true << QStringLiteral("memory:DMABuf") << false << qint64(3) << quint64(1);
}

void GStreamerTest::_testQgcQVideoSinkGpuPathSelection()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else
    QFETCH(bool, gpuZeroCopy);
    QFETCH(QString, memoryFeature);
    QFETCH(bool, fdMemory);
    QFETCH(qint64, expectedGpuFallbacks);
    QFETCH(quint64, expectedNoPathVerdicts);
#if !defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    if (fdMemory) {
        QSKIP("fd-backed test buffers need memfd (desktop Linux)");
    }
#endif

    QVideoSink videoSink;
    const QByteArray feature = memoryFeature.toUtf8();
    QgcQVideoSinkHarness harness(&videoSink, gpuZeroCopy, feature.isEmpty() ? nullptr : feature.constData());
    QVERIFY(harness.isReady());

    using GstHwPathTelemetry::HwFallbackReason;
    const quint64 noPathBefore =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::None, HwFallbackReason::UnknownMemType);
    for (int i = 1; i <= 3; ++i) {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        if (fdMemory) {
            GstBuffer* const buffer = QgcQVideoSinkHarness::makeFdFrame(i * GST_SECOND);
            QVERIFY(buffer);
            QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
            continue;
        }
#endif
        QCOMPARE(harness.pushFrame(i * GST_SECOND), GST_FLOW_OK);
    }
    const quint64 noPathAfter =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::None, HwFallbackReason::UnknownMemType);

    QCOMPARE(sinkStat(harness.element(), GstQgc::sinkFramesField(HwVideoBufferPath::None)), qint64(3));
    QCOMPARE(sinkStat(harness.element(), "gpu-fallbacks"), expectedGpuFallbacks);
    QCOMPARE(sinkStat(harness.element(), "stream-demotions"), qint64(expectedGpuFallbacks > 0 ? 1 : 0));
    QCOMPARE(noPathAfter - noPathBefore, expectedNoPathVerdicts);
#endif
}

void GStreamerTest::_testQgcQVideoSinkDropsUndeliveredFrame_data()
{
    QTest::addColumn<bool>("viaFlush");
    QTest::newRow("flush-start") << true;
    QTest::newRow("stop") << false;
}

void GStreamerTest::_testQgcQVideoSinkDropsUndeliveredFrame()
{
    QFETCH(bool, viaFlush);

    int emissions = 0;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame&) { ++emissions; });
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    if (viaFlush) {
        QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
    } else {
        QCOMPARE(gst_element_set_state(harness.element(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 0);
    QVERIFY(!videoSink.videoFrame().isValid());
}

void GStreamerTest::_testQgcQVideoSinkPtsGuardResync_data()
{
    QTest::addColumn<QString>("boundary");
    QTest::addColumn<bool>("discont");
    QTest::addColumn<qint64>("nextPtsMs");
    QTest::addColumn<bool>("expectDelivered");

    // Baseline frame is at 10 s; 9.9 s is a small regression the guard drops unless a timeline boundary resets it.
    QTest::newRow("small-regression-dropped") << QString() << false << qint64(9900) << false;
    QTest::newRow("flush-resets") << QStringLiteral("flush") << false << qint64(9900) << true;
    QTest::newRow("segment-resets") << QStringLiteral("segment") << false << qint64(9900) << true;
    QTest::newRow("stream-start-resets") << QStringLiteral("stream-start") << false << qint64(9900) << true;
    QTest::newRow("discont-resyncs") << QString() << true << qint64(9900) << true;
    QTest::newRow("large-jump-resyncs") << QString() << false << qint64(0) << true;
}

void GStreamerTest::_testQgcQVideoSinkPtsGuardResync()
{
    QFETCH(QString, boundary);
    QFETCH(bool, discont);
    QFETCH(qint64, nextPtsMs);
    QFETCH(bool, expectDelivered);

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QCOMPARE(harness.pushFrame(10 * GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harness.delivered(), guint64(1));

    // Identical caps throughout, so GstBaseSink never re-runs set_caps.
    if (boundary == QStringLiteral("flush")) {
        QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
        QVERIFY(harness.pushEvent(gst_event_new_flush_stop(/*reset_time=*/TRUE)));
        QVERIFY(harness.pushSegment());  // data after a flush requires a fresh segment
    } else if (boundary == QStringLiteral("segment")) {
        QVERIFY(harness.pushSegment());
    } else if (boundary == QStringLiteral("stream-start")) {
        QVERIFY(harness.pushEvent(gst_event_new_stream_start("qgc-harness-restart")));
    }

    QCOMPARE(harness.pushFrame(static_cast<GstClockTime>(nextPtsMs) * GST_MSECOND, discont), GST_FLOW_OK);
    QCOMPARE(harness.delivered(), guint64(expectDelivered ? 2 : 1));
}

void GStreamerTest::_testQgcQVideoSinkMapFailuresPostElementError()
{
    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    // Periodic per-failure warnings are rate-limited by process-global counters, so their count is not fixed.
    ignoreLogMessage("Video.GStreamer.HwBuffers.CpuPool", QtWarningMsg,
                     QRegularExpression(QStringLiteral("gst_video_frame_map failed")));
    ignoreLogMessage("Video.GStreamer.QgcQVideoSink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("mapping failed")));
    expectLogMessage("Video.GStreamer.QgcQVideoSink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("consecutive map failures")));

    // Undersized buffers can never be mapped; the sink must eventually give up with a descriptive element error.
    GstFlowReturn ret = GST_FLOW_OK;
    for (int i = 0; (i < 1000) && (ret == GST_FLOW_OK); ++i) {
        ret = harness.pushFrame(GST_CLOCK_TIME_NONE, false, /*size=*/4);
    }
    verifyExpectedLogMessage();
    QCOMPARE(ret, GST_FLOW_ERROR);

    const GStreamer::GstMessagePtr msg =
        GStreamer::adoptMessage(gst_bus_pop_filtered(harness.bus(), GST_MESSAGE_ERROR));
    QVERIFY2(msg, "map-failure escalation must post an element error before returning GST_FLOW_ERROR");
    GError* rawError = nullptr;
    gchar* rawDebug = nullptr;
    gst_message_parse_error(msg.get(), &rawError, &rawDebug);
    const GStreamer::GErrorPtr error = GStreamer::adoptError(rawError);
    const GStreamer::GCharPtr debug = GStreamer::adoptGChar(rawDebug);
    QVERIFY(g_error_matches(error.get(), GST_STREAM_ERROR, GST_STREAM_ERROR_FAILED));
    const QString details = QString::fromUtf8(debug ? debug.get() : "");
    QVERIFY2(details.contains(QStringLiteral("consecutive map failures")), qPrintable(details));
}

void GStreamerTest::_testQVideoSinkControllerReleaseClearsVideoFrame()
{
    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(nullptr);
    QVERIFY(harness.isReady());
    QObject controllerOwner;
    auto* controller = new QGCQVideoSinkController(harness.element(), &controllerOwner);
    controller->setVideoSink(QPointer<QVideoSink>(&videoSink));

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QVERIFY(videoSink.videoFrame().isValid());

    // The released sink must not keep the last GstSample-backed frame alive.
    controller->prepareForRelease();
    QVERIFY(!videoSink.videoFrame().isValid());
}

#endif
