#include "GstQgcSinkTest.h"

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
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>

#include "CpuVideoFramePool.h"
#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "GstQgcFrameMap.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/GstQgcAllocation.h"
#include "gstqgc/GstQgcBinChain.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcRenderTarget.h"
#include "gstqgc/GstQgcTestSupport.h"
#include "gstqgc/GstQgcVideoFormats.h"
#include "gstqgc/gstqgcqvideosink.h"
#include "gstqgc/gstqgcvideosinkbin.h"
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#endif

namespace {

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

    /// Whether the sink asked upstream to renegotiate since the last call.
    bool takeReconfigure() { return gst_pad_check_reconfigure(_src.get()); }

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

    guint64 delivered() const { return GstQgcTest::framesDelivered(_sink.get()); }

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

/// Sorted factory names of a qgcvideosinkbin holding @p others behind its default converter: videoconvertscale, or
/// videoconvert and videoscale before GStreamer 1.22.
QStringList withConverter(QStringList others)
{
    if (GstTest::firstMissingFactory({"videoconvertscale"}).isEmpty()) {
        others << QStringLiteral("videoconvertscale");
    } else {
        others << QStringLiteral("videoconvert") << QStringLiteral("videoscale");
    }
    others.sort();
    return others;
}

/// What @p element's sink pad accepts.
GStreamer::GstCapsPtr sinkPadCaps(GstElement* element)
{
    const GStreamer::GstPadPtr pad = GStreamer::adoptPad(gst_element_get_static_pad(element, "sink"));
    return GStreamer::adoptCaps(pad ? gst_pad_query_caps(pad.get(), nullptr) : nullptr);
}

constexpr GstQgc::RenderTarget kOpenGlOnEgl{.backend = GstQgc::RenderBackend::OpenGL, .egl = true};

/// Stands in for Qt's EGLDisplay (GstQgc::SinkEnvironment::dmaBufDisplay), so the sink offers DMABuf; never passed to
/// EGL by these tests.
void* const kFakeQtEglDisplay = reinterpret_cast<void*>(quintptr(1));

}  // namespace

void GstQgcSinkTest::_testQgcVideoSinkBinGpuZeroCopyProperty()
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

        // The converter and qgcqvideosink, whose caps ask for square pixels.
        QCOMPARE(GstTest::childFactoryNames(GST_BIN(bin.get())), withConverter({QStringLiteral("qgcqvideosink")}));
        const GStreamer::GstCapsPtr caps = sinkPadCaps(qvideosink.get());
        QVERIFY(caps && !gst_caps_is_empty(caps.get()));
        for (guint i = 0; i < gst_caps_get_size(caps.get()); ++i) {
            gint parN = 0;
            gint parD = 0;
            QVERIFY(
                gst_structure_get_fraction(gst_caps_get_structure(caps.get(), i), "pixel-aspect-ratio", &parN, &parD));
            QCOMPARE(parN, parD);
        }
    }

    {
        // disable-par lets the sink take any pixel aspect ratio.
        const GStreamer::GstElementPtr bin = makeBin(FALSE, TRUE);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (CPU branch, disable-par=TRUE)");
        const GStreamer::GstElementPtr qvideosink =
            GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(bin.get()), "qgcqvideosink"));
        QVERIFY(qvideosink);
        const GStreamer::GstCapsPtr caps = sinkPadCaps(qvideosink.get());
        QVERIFY(caps && !gst_caps_is_empty(caps.get()));
        for (guint i = 0; i < gst_caps_get_size(caps.get()); ++i) {
            QVERIFY(!gst_structure_has_field(gst_caps_get_structure(caps.get(), i), "pixel-aspect-ratio"));
        }
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

void GstQgcSinkTest::_testQgcVideoSinkBinGpuBinVariant_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("egl");
    QTest::addColumn<QStringList>("children");
    QTest::addColumn<QStringList>("memoryFeatures");
    QTest::addColumn<bool>("singlePlaneRgb2D");

    const QString systemMemory = QStringLiteral("memory:SystemMemory");
    const QStringList passthroughChildren = withConverter({QStringLiteral("qgcqvideosink")});
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) || defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    const int openGl = int(GstQgc::RenderBackend::OpenGL);
#endif
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    const int glUpload = int(GstQgc::GpuBinMode::GlUpload);
    const QStringList gluploadChildren =
        withConverter({QStringLiteral("glcolorconvert"), QStringLiteral("glupload"), QStringLiteral("qgcqvideosink")});
    const QStringList glMemory{QStringLiteral("memory:GLMemory")};
    // glcolorconvert turns external-OES into texture-target=2D; Qt samples multi-plane GLMemory as green frames.
    QTest::newRow("glupload") << glUpload << openGl << true << gluploadChildren << glMemory << true;
    // GLMemory can't reach a renderer that isn't OpenGL: the bin would round-trip every frame.
    QTest::newRow("glupload-under-vulkan") << glUpload << int(GstQgc::RenderBackend::Vulkan) << false
                                           << passthroughChildren << QStringList{systemMemory} << false;
    // GLX, or EGL not known yet (Android before the scene graph runs): gst-gl shares Qt's OpenGL context either way.
    QTest::newRow("glupload-egl-unknown") << glUpload << openGl << false << gluploadChildren << glMemory << true;
#endif
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    // DMA_DRM fragments carry memory:DMABuf too; GLMemory stays out so gst-gl can't take the import over.
    QTest::newRow("direct-dmabuf") << int(GstQgc::GpuBinMode::DirectDmaBuf) << openGl << true << passthroughChildren
                                   << QStringList{QStringLiteral("memory:DMABuf"), systemMemory} << false;
#endif
#if !defined(QGC_GST_BIN_HAS_GLUPLOAD) && !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QStringList direct3D11Features;
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    direct3D11Features << QStringLiteral("memory:D3D11Memory");
#endif
    QTest::newRow("passthrough-d3d11") << int(GstQgc::GpuBinMode::Passthrough) << int(GstQgc::RenderBackend::Direct3D11)
                                       << false << passthroughChildren << (direct3D11Features << systemMemory) << false;
    // vtdec's IOSurface-backed buffers arrive in system memory; GLMemory would be downloaded under Metal.
    QTest::newRow("passthrough-metal") << int(GstQgc::GpuBinMode::Passthrough) << int(GstQgc::RenderBackend::Metal)
                                       << false << passthroughChildren << QStringList{systemMemory} << false;
#endif
}

void GstQgcSinkTest::_testQgcVideoSinkBinGpuBinVariant()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else
    QFETCH(int, mode);
    QFETCH(int, backend);
    QFETCH(bool, egl);
    QFETCH(QStringList, children);
    QFETCH(QStringList, memoryFeatures);
    QFETCH(bool, singlePlaneRgb2D);

    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = egl},
         .gpuBin = {.mode = static_cast<GstQgc::GpuBinMode>(mode)},
         .dmaBufDisplay = kFakeQtEglDisplay});

    GstElement* element = gst_element_factory_make_full("qgcvideosinkbin", "gpu-zerocopy", TRUE, nullptr);
    QVERIFY2(element, "Failed to create qgcvideosinkbin (GPU branch)");
    const GStreamer::GstElementPtr bin = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(element)));
    QCOMPARE(GstTest::childFactoryNames(GST_BIN(bin.get())), children);

    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(bin.get()), "qgcqvideosink"));
    QVERIFY(qvideosink);
    const GStreamer::GstCapsPtr caps = sinkPadCaps(qvideosink.get());
    QVERIFY2(caps, "qgcqvideosink returned no caps");
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

void GstQgcSinkTest::_testQgcVideoSinkBinRejectsFailedAdopt()
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

void GstQgcSinkTest::_testQgcBinChainFailedAdoptDropsFloatingRefOnce()
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

void GstQgcSinkTest::_testQgcVideoSinkBinQVideoSinkAccessor()
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

void GstQgcSinkTest::_testQVideoSinkControllerClearsElementOnDestroy()
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

void GstQgcSinkTest::_testQVideoSinkControllerClearsElementWhenVideoSinkDestroyed()
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

void GstQgcSinkTest::_testQVideoSinkControllerNullSinkStillDeactivatesOnDestroy()
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

void GstQgcSinkTest::_testQVideoSinkControllerRepeatedSetupKeepsNewBindingActive()
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
    auto* firstController = controllerOwner.findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
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

void GstQgcSinkTest::_testQVideoSinkControllerNoWindowStartsInactive()
{
    const SinkBin sink = makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const qvideosink = sink.qvideosink.get();

    QObject controllerOwner;
    QGCQVideoSinkController controller(qvideosink, &controllerOwner);
    controller.setActive(true);

    QQuickVideoOutput videoOutput;
    QVERIFY(videoOutput.window() == nullptr);
    controller.followWindowVisibility(&videoOutput);

    gboolean active = TRUE;
    g_object_get(qvideosink, "active", &active, nullptr);
    QCOMPARE(active, FALSE);
}

void GstQgcSinkTest::_testCpuZeroCopyFrameRejectsWritableMap()
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

void GstQgcSinkTest::_testCpuZeroCopyPinnedFramesCappedPerSink()
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

void GstQgcSinkTest::_testColorimetryPixelFormatMapping()
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

void GstQgcSinkTest::_testCpuCapsFormatsRoundTripToQt()
{
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(GstQgc::buildSinkCaps({}).c_str()));
    QVERIFY2(caps, "buildSinkCaps() did not parse");
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

void GstQgcSinkTest::_testAllocationQueryHwMemoryPoolHint_data()
{
    QTest::addColumn<QByteArray>("minBuffersEnv");
    QTest::addColumn<int>("expectedMinBuffers");

    // Default: a buffer beyond those the sink and Qt hold.
    const int kDefault = CpuVideoFramePool::kMaxPinnedFrames + 1;
    QTest::newRow("default") << QByteArray() << kDefault;
    QTest::newRow("env-override") << QByteArrayLiteral("3") << 3;
    QTest::newRow("env-invalid") << QByteArrayLiteral("0") << kDefault;
}

void GstQgcSinkTest::_testAllocationQueryHwMemoryPoolHint()
{
    QFETCH(QByteArray, minBuffersEnv);
    QFETCH(int, expectedMinBuffers);
    const TestFixtures::EnvVarFixture envGuard("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS");
    if (minBuffersEnv.isEmpty()) {
        qunsetenv("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS");
    } else {
        qputenv("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS", minBuffersEnv);
    }

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
    QCOMPARE(int(minBuffers), expectedMinBuffers);
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "VideoMeta not advertised on GPU path");
    // No frame path reads it, and a producer that attaches it may skip uploading.
    QVERIFY(!gst_query_find_allocation_meta(query.get(), GST_VIDEO_GL_TEXTURE_UPLOAD_META_API_TYPE, nullptr));
}

void GstQgcSinkTest::_testQgcVideoSinkBinAllocationQueryAdvertisesVideoMeta()
{
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    const GstQgc::ScopedSinkEnvironmentForTest environment({.target = kOpenGlOnEgl,
                                                            .gpuBin = {.mode = GstQgc::GpuBinMode::DirectDmaBuf},
                                                            .dmaBufDisplay = kFakeQtEglDisplay});
#else
    QSKIP("The ALLOCATION VideoMeta contract covers the direct-DMABuf bin");
#endif

    GstElement* const element = gst_element_factory_make_full("qgcvideosinkbin", "gpu-zerocopy", TRUE, nullptr);
    QVERIFY2(element, "qgcvideosinkbin factory did not create an element");
    const GStreamer::GstElementPtr bin = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(element)));
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(bin.get(), "sink"));
    QVERIFY2(sinkPad, "qgcvideosinkbin has no sink pad");
    const GStreamer::GstPadPtr src = GStreamer::adoptPad(GST_PAD(gst_object_ref_sink(gst_pad_new("src", GST_PAD_SRC))));
    QCOMPARE(gst_pad_link(src.get(), sinkPad.get()), GST_PAD_LINK_OK);
    QVERIFY(gst_pad_set_active(src.get(), TRUE));
    const auto stop = qScopeGuard([&bin, &src] {
        (void) gst_element_set_state(bin.get(), GST_STATE_NULL);
        (void) gst_pad_set_active(src.get(), FALSE);
    });
    QVERIFY(gst_element_set_state(bin.get(), GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);

    // Negotiated as a VA decoder would, so the converter passes the DMABuf frames and their ALLOCATION query through.
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "video/x-raw(memory:DMABuf), format=(string)NV12, width=(int)64, height=(int)64, framerate=(fraction)30/1"));
    QVERIFY2(caps, "HW caps did not parse");
    GstSegment segment;
    gst_segment_init(&segment, GST_FORMAT_TIME);
    QVERIFY(gst_pad_push_event(src.get(), gst_event_new_stream_start("qgc-allocation")));
    QVERIFY(gst_pad_push_event(src.get(), gst_event_new_caps(caps.get())));
    QVERIFY(gst_pad_push_event(src.get(), gst_event_new_segment(&segment)));

    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), TRUE));
    QVERIFY2(query, "Could not allocate ALLOCATION query");
    QVERIFY2(gst_pad_peer_query(src.get(), query.get()), "qgcvideosinkbin did not answer ALLOCATION query");
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "qgcvideosinkbin ALLOCATION query must advertise VideoMeta for gst-va DMABuf");

    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), 1U);
    GstBufferPool* pool = nullptr;
    guint size = 0, minBuffers = 0, maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query.get(), 0, &pool, &size, &minBuffers, &maxBuffers);
    QVERIFY2(pool == nullptr, "qgcvideosinkbin must not propose a generic pool for native memory");
    QVERIFY2(minBuffers > 0, "qgcvideosinkbin min-buffer hint must be non-zero");
}

void GstQgcSinkTest::_testQgcQVideoSinkSystemMemoryAllocation_data()
{
    QTest::addColumn<bool>("needPool");

    QTest::newRow("pool needed") << true;
    // When upstream does not need a pool, the sink must not force one.
    QTest::newRow("no pool needed") << false;
}

void GstQgcSinkTest::_testQgcQVideoSinkSystemMemoryAllocation()
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

void GstQgcSinkTest::_testColorimetryColorSpaceMapping()
{
    // Mirrors Qt 6.10.3 qgst.cpp: SMPTE240M maps to AdobeRgb, FCC remains Undefined.
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_SMPTE240M), QVideoFrameFormat::ColorSpace_AdobeRgb);
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_FCC), QVideoFrameFormat::ColorSpace_Undefined);
}

void GstQgcSinkTest::_testColorimetryResolutionHeuristicMatchesQt()
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

void GstQgcSinkTest::_testColorimetryTransferMapping()
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

void GstQgcSinkTest::_testColorimetryColorRangeMapping()
{
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_0_255), QVideoFrameFormat::ColorRange_Full);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_16_235), QVideoFrameFormat::ColorRange_Video);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_UNKNOWN), QVideoFrameFormat::ColorRange_Unknown);
}

void GstQgcSinkTest::_testPixelFormatAcceptedButNotAdvertised()
{
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRx), QVideoFrameFormat::Format_BGRX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBx), QVideoFrameFormat::Format_RGBX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_ARGB), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_xRGB), QVideoFrameFormat::Format_XRGB8888);

    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGR), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGB), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UNKNOWN), QVideoFrameFormat::Format_Invalid);
}

void GstQgcSinkTest::_testAdvertisedFormatListMatchesTable()
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

void GstQgcSinkTest::_testAdvertisedFormatListForOffer_data()
{
    QTest::addColumn<uint>("offer");
    QTest::addColumn<QString>("expected");

    // Multi-plane GLMemory renders green through Qt's GL import, so glupload must stay single-plane RGB.
    QTest::newRow("glupload") << uint(GstQgc::kOfferGlUpload) << QStringLiteral("{ BGRA, RGBA }");
    QTest::newRow("linear-dma-drm") << uint(GstQgc::kOfferDmaDrm)
                                    << QStringLiteral("{ NV12, NV21, I420, YV12, P010_10LE, BGRA, RGBA }");
}

void GstQgcSinkTest::_testAdvertisedFormatListForOffer()
{
    QFETCH(uint, offer);
    QFETCH(QString, expected);
    QCOMPARE(QString::fromStdString(GstQgc::advertisedFormatList(offer)), expected);
}

void GstQgcSinkTest::_testApplyOrientationToFrameMapping()
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

void GstQgcSinkTest::_testAdapterFlushDropsInFlightSamples()
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

void GstQgcSinkTest::_testQgcQVideoSinkDeliversLatestFrameOnly()
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

void GstQgcSinkTest::_testQgcQVideoSinkRenegotiatesMidStream()
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

void GstQgcSinkTest::_testQgcQVideoSinkAppliesCropMeta()
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

void GstQgcSinkTest::_testQgcQVideoSinkAppliesOrientationTag_data()
{
    QTest::addColumn<QString>("tag");
    QTest::addColumn<bool>("restartStream");
    QTest::addColumn<int>("rotation");
    QTest::addColumn<bool>("mirrored");

    QTest::newRow("rotate-90") << QStringLiteral("rotate-90") << false << int(QtVideo::Rotation::Clockwise90) << false;
    // Flipped, then turned 90 degrees clockwise: Qt turns first, so that's 270 degrees, then mirrored.
    QTest::newRow("flip-rotate-90") << QStringLiteral("flip-rotate-90") << false << int(QtVideo::Rotation::Clockwise270)
                                    << true;
    // The tag belongs to the stream it came with.
    QTest::newRow("new-stream-untagged") << QStringLiteral("rotate-90") << true << int(QtVideo::Rotation::None)
                                         << false;
}

void GstQgcSinkTest::_testQgcQVideoSinkAppliesOrientationTag()
{
    QFETCH(QString, tag);
    QFETCH(bool, restartStream);
    QFETCH(int, rotation);
    QFETCH(bool, mirrored);

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    GstTagList* const tags = gst_tag_list_new(GST_TAG_IMAGE_ORIENTATION, qPrintable(tag), nullptr);
    QVERIFY(harness.pushEvent(gst_event_new_tag(tags)));
    if (restartStream) {
        QVERIFY(harness.pushEvent(gst_event_new_stream_start("qgc-harness-restart")));
        QVERIFY(harness.pushSegment());
    }

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    const QVideoFrame frame = videoSink.videoFrame();
    QVERIFY(frame.isValid());
    QCOMPARE(int(frame.rotation()), rotation);
    QCOMPARE(frame.mirrored(), mirrored);
}

void GstQgcSinkTest::_testQgcQVideoSinkStatsArePerSink()
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
    QCOMPARE(GstQgcTest::sinkStat(harnessA.element(), cpuField), qint64(2));
    QCOMPARE(GstQgcTest::sinkStat(harnessB.element(), cpuField), qint64(1));
    QCOMPARE(GstQgcTest::framesDelivered(harnessB.element()), guint64(1));

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

void GstQgcSinkTest::_testQgcQVideoSinkKeepsBaseSinkStats()
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

void GstQgcSinkTest::_testQgcQVideoSinkGpuPathSelection_data()
{
    QTest::addColumn<bool>("gpuZeroCopy");
    QTest::addColumn<QString>("memoryFeature");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("fdMemory");
    QTest::addColumn<qint64>("expectedGpuFallbacks");
    QTest::addColumn<quint64>("expectedNoPathVerdicts");

    const int openGl = int(GstQgc::RenderBackend::OpenGL);
    // Every row pushes three buffers, in system memory unless fdMemory.
    QTest::newRow("gpu-zerocopy-off") << false << QString() << openGl << true << qint64(0) << quint64(0);
    // Software-decoded frames never enter path selection.
    QTest::newRow("system-memory-caps") << true << QString() << openGl << false << qint64(0) << quint64(0);
    // Other memory under plain video/x-raw caps (vtdec's CoreVideo buffers, V4L2 dmabuf export) still tries the GPU
    // paths: the first frame finds no path for this allocator, the rest reuse that verdict.
    QTest::newRow("system-memory-caps-fd-buffers") << true << QString() << openGl << true << qint64(3) << quint64(1);
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QTest::newRow("gpu-caps-system-buffers")
        << true << QStringLiteral("memory:DMABuf") << openGl << false << qint64(3) << quint64(1);
#elif defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QTest::newRow("gpu-caps-system-buffers")
        << true << QStringLiteral("memory:D3D11Memory") << int(GstQgc::RenderBackend::Direct3D11) << false << qint64(3)
        << quint64(1);
#endif
}

void GstQgcSinkTest::_testQgcQVideoSinkGpuPathSelection()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else
    QFETCH(bool, gpuZeroCopy);
    QFETCH(QString, memoryFeature);
    QFETCH(int, backend);
    QFETCH(bool, fdMemory);
    QFETCH(qint64, expectedGpuFallbacks);
    QFETCH(quint64, expectedNoPathVerdicts);
#if !defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    if (fdMemory) {
        QSKIP("fd-backed test buffers need memfd (desktop Linux)");
    }
#endif
    // A render target that imports the row's GPU memory, so the sink accepts its caps.
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = true},
         .dmaBufDisplay = kFakeQtEglDisplay});

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

    QCOMPARE(GstQgcTest::sinkStat(harness.element(), GstQgc::sinkFramesField(HwVideoBufferPath::None)), qint64(3));
    QCOMPARE(GstQgcTest::sinkStat(harness.element(), "gpu-fallbacks"), expectedGpuFallbacks);
    QCOMPARE(GstQgcTest::sinkStat(harness.element(), "stream-demotions"), qint64(expectedGpuFallbacks > 0 ? 1 : 0));
    QCOMPARE(noPathAfter - noPathBefore, expectedNoPathVerdicts);
#endif
}

void GstQgcSinkTest::_testQgcQVideoSinkCapsFollowRenderTarget_data()
{
    QTest::addColumn<bool>("gpuZeroCopy");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("egl");
    QTest::addColumn<QStringList>("memoryFeatures");

    const QString systemMemory = QStringLiteral("memory:SystemMemory");
    const QStringList systemOnly{systemMemory};
    QStringList openGlOnEgl;
    // GLX on Linux; WGL or CGL elsewhere, which gst-gl can't share.
    QStringList openGlNotEgl;
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    openGlOnEgl << QStringLiteral("memory:DMABuf");
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    openGlOnEgl << QStringLiteral("memory:GLMemory");
#if defined(Q_OS_LINUX)
    openGlNotEgl << QStringLiteral("memory:GLMemory");
#endif
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    openGlOnEgl << QStringLiteral("memory:AHardwareBuffer");
#endif
    QStringList direct3D11;
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    direct3D11 << QStringLiteral("memory:D3D11Memory");
#endif

    using GstQgc::RenderBackend;
    QTest::newRow("cpu-sink") << false << int(RenderBackend::OpenGL) << true << systemOnly;
    QTest::newRow("unknown") << true << int(RenderBackend::Unknown) << false << systemOnly;
    QTest::newRow("null") << true << int(RenderBackend::Null) << false << systemOnly;
    QTest::newRow("opengl-egl") << true << int(RenderBackend::OpenGL) << true << (openGlOnEgl << systemMemory);
    QTest::newRow("opengl-glx") << true << int(RenderBackend::OpenGL) << false << (openGlNotEgl << systemMemory);
    QTest::newRow("vulkan") << true << int(RenderBackend::Vulkan) << false << systemOnly;
    QTest::newRow("metal") << true << int(RenderBackend::Metal) << false << systemOnly;
    QTest::newRow("d3d11") << true << int(RenderBackend::Direct3D11) << false << (direct3D11 << systemMemory);
    QTest::newRow("d3d12") << true << int(RenderBackend::Direct3D12) << false << systemOnly;
}

void GstQgcSinkTest::_testQgcQVideoSinkCapsFollowRenderTarget()
{
    QFETCH(bool, gpuZeroCopy);
    QFETCH(int, backend);
    QFETCH(bool, egl);
    QFETCH(QStringList, memoryFeatures);
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = egl},
         .dmaBufDisplay = kFakeQtEglDisplay});

    GstElement* const element = gst_element_factory_make_full("qgcqvideosink", "gpu-zerocopy", gpuZeroCopy, nullptr);
    QVERIFY(element);
    const GStreamer::GstElementPtr sink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(element)));
    const GStreamer::GstCapsPtr caps = sinkPadCaps(sink.get());
    QVERIFY(caps);
    QCOMPARE(capsMemoryFeatures(caps.get()), memoryFeatures);
    // The GL path samples only GL_TEXTURE_2D; amcvideodec's external-OES textures would show black.
    for (guint i = 0; i < gst_caps_get_size(caps.get()); ++i) {
        const GstCapsFeatures* const features = gst_caps_get_features(caps.get(), i);
        if (features && gst_caps_features_contains(features, "memory:GLMemory")) {
            QCOMPARE(
                QString::fromUtf8(gst_structure_get_string(gst_caps_get_structure(caps.get(), i), "texture-target")),
                QStringLiteral("2D"));
        }
    }
}

void GstQgcSinkTest::_testQgcQVideoSinkReconfiguresOnRenderTargetChange()
{
    QVideoSink videoSink;
    const GstQgc::ScopedSinkEnvironmentForTest vulkan({.target = {.backend = GstQgc::RenderBackend::Vulkan}});
    QgcQVideoSinkHarness harness(&videoSink, /*gpuZeroCopy=*/true);
    QVERIFY(harness.isReady());
    (void) harness.takeReconfigure();  // linking marks a source pad for reconfiguration

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QVERIFY(!harness.takeReconfigure());

    {
        // E.g. the scene graph came up on OpenGL after the stream negotiated: GPU memory became importable.
        const GstQgc::ScopedSinkEnvironmentForTest openGl({.target = kOpenGlOnEgl});
        QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
        QVERIFY(harness.takeReconfigure());
        QCOMPARE(harness.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
        QVERIFY2(!harness.takeReconfigure(), "one change must ask for one renegotiation");

        const GStreamer::GstCapsPtr caps = sinkPadCaps(harness.element());
        QVERIFY(caps);
        QCOMPARE(harness.pushFrame(4 * GST_SECOND), GST_FLOW_OK);
        QVERIFY(!harness.takeReconfigure());
    }
    QCOMPARE(harness.pushFrame(5 * GST_SECOND), GST_FLOW_OK);
    QVERIFY(harness.takeReconfigure());
}

void GstQgcSinkTest::_testQgcQVideoSinkReconfiguresOnDmaBufImportChange()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    GstDmaBufVideoBuffer::resetCachedState();
    const auto resetLatch = qScopeGuard([] { GstDmaBufVideoBuffer::resetCachedState(); });
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = kOpenGlOnEgl, .dmaBufDisplay = kFakeQtEglDisplay});
    const auto offersDmaBuf = [](GstElement* sink) {
        const GStreamer::GstCapsPtr caps = sinkPadCaps(sink);
        return caps && capsMemoryFeatures(caps.get()).contains(QStringLiteral("memory:DMABuf"));
    };

    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink, /*gpuZeroCopy=*/true);
    QVERIFY(harness.isReady());
    (void) harness.takeReconfigure();  // linking marks a source pad for reconfiguration
    QVERIFY(offersDmaBuf(harness.element()));
    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QVERIFY(!harness.takeReconfigure());

    // An import failure no later frame avoids: the sink must stop offering DMABuf (DMA_DRM included) at once, or a
    // tiled stream would stay negotiated and its frames could only be dropped.
    expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg,
                     QRegularExpression(QStringLiteral("EGL_BAD_MATCH")));
    GstDmaBufVideoBuffer::recordImportFailureForTest(/*unrecoverable=*/true);
    verifyExpectedLogMessage();
    QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QVERIFY2(harness.takeReconfigure(), "disabling the DMABuf import must ask upstream to renegotiate");
    QVERIFY(!offersDmaBuf(harness.element()));
    QCOMPARE(harness.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    QVERIFY2(!harness.takeReconfigure(), "one change must ask for one renegotiation");

    // A recreated scene graph clears the latch.
    GstDmaBufVideoBuffer::resetCachedState();
    QCOMPARE(harness.pushFrame(4 * GST_SECOND), GST_FLOW_OK);
    QVERIFY(harness.takeReconfigure());
    QVERIFY(offersDmaBuf(harness.element()));
#endif
}

void GstQgcSinkTest::_testQgcQVideoSinkDropsUndeliveredFrame_data()
{
    QTest::addColumn<bool>("viaFlush");
    QTest::newRow("flush-start") << true;
    QTest::newRow("stop") << false;
}

void GstQgcSinkTest::_testQgcQVideoSinkDropsUndeliveredFrame()
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

void GstQgcSinkTest::_testQgcQVideoSinkForwardsTimestamps()
{
    QVideoSink videoSink;
    QgcQVideoSinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QCOMPARE(harness.pushFrame(10 * GST_SECOND), GST_FLOW_OK);

    // Timestamps going backwards without DISCONT, as after an encoder restart or an RTP resync; Qt's display path
    // ignores timestamps, so the frame is shown, timestamped as it came.
    constexpr GstClockTime kRegressed = 9900 * GST_MSECOND;
    QCOMPARE(harness.pushFrame(kRegressed), GST_FLOW_OK);
    QCOMPARE(harness.delivered(), guint64(2));
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    const QVideoFrame frame = videoSink.videoFrame();
    QCOMPARE(frame.startTime(), qint64(kRegressed / GST_USECOND));
    // Without a buffer duration the end stays unknown rather than collapsing onto the start.
    QCOMPARE(frame.endTime(), qint64(-1));
}

void GstQgcSinkTest::_testQgcQVideoSinkMapFailuresPostElementError()
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

void GstQgcSinkTest::_testQVideoSinkControllerReleaseClearsVideoFrame()
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

UT_REGISTER_TEST(GstQgcSinkTest, TestLabel::Unit)
