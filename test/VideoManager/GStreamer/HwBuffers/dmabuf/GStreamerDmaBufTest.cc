#include "HwBuffers/GstHwBuffersPipelineTest.h"
#include "HwBuffers/GstHwBuffersTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <memory>
#include <rhi/qrhi.h>
#include <vector>
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <gst/allocators/gstdmabuf.h>
#include <unistd.h>
#endif

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBufferFactory.h"
#include "GstQgcFrameMap.h"
#include "HwBuffers/common/HwBuffers.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcRenderTarget.h"
#include "gstqgc/GstQgcTestSupport.h"
#include "gstqgc/GstQgcVideoFormats.h"
#include "gstqgc/gstqgcqvideosink.h"
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#include "GstQtDmaBufImport.h"
#endif
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCRhiCapture.h"
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
namespace {

// Single-memory 64x64 DMABuf sample backed by a memfd: enough for the streaming-thread gates and CPU map. @p capsString
// replaces the system-memory caps (e.g. DMA_DRM caps).
GstSample* makeMemfdDmaBufSample(GstVideoFormat format, GstVideoInfo& info, const char* capsString = nullptr)
{
    gst_video_info_set_format(&info, format, 64, 64);
    const int fd = GstTest::makeMemfd(QByteArray(static_cast<qsizetype>(GST_VIDEO_INFO_SIZE(&info)), '\0'));
    if (fd < 0) {
        return nullptr;
    }
    GstAllocator* allocator = gst_dmabuf_allocator_new();
    GstMemory* memory = gst_dmabuf_allocator_alloc(allocator, fd, GST_VIDEO_INFO_SIZE(&info));
    gst_object_unref(allocator);
    if (!memory) {
        ::close(fd);
        return nullptr;
    }
    GstBuffer* buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, memory);
    GstCaps* caps = capsString ? gst_caps_from_string(capsString) : gst_video_info_to_caps(&info);
    GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr);
    gst_buffer_unref(buffer);
    gst_caps_unref(caps);
    return sample;
}

}  // namespace
#endif

void GstHwBuffersPipelineTest::_testDmaBufDispatch()
{
#if !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QSKIP("Direct-DMABuf bin not compiled in this build");
#else
    // The DMABuf path and its DMA_DRM caps both need the EGLDisplay Qt renders with.
    if (!QGCRhiCapture::qtEglDisplay()) {
        QSKIP("Qt renders without EGL here (offscreen QPA, GLX)");
    }
    // The sink offers DMABuf only to Qt's OpenGL over EGL.
    constexpr GstQgc::RenderTarget kDmaBufTarget{.backend = GstQgc::RenderBackend::OpenGL, .egl = true};
    const GstQgc::ScopedSinkEnvironmentForTest environment({.target = kDmaBufTarget,
                                                            .gpuBin = {.mode = GstQgc::GpuBinMode::DirectDmaBuf},
                                                            .dmaBufDisplay = QGCRhiCapture::qtEglDisplay()});

    GSTTEST_SKIP_UNLESS_FACTORIES("vah264enc", "vah264dec", "vapostproc");

    const auto onlyDmaBufCaps = [](GstCaps* caps) {
        GstCaps* out = gst_caps_new_empty();
        for (guint i = 0; caps && (i < gst_caps_get_size(caps)); ++i) {
            GstCapsFeatures* features = gst_caps_get_features(caps, i);
            if (!features || !gst_caps_features_contains(features, "memory:DMABuf")) {
                continue;
            }
            gst_caps_append_structure_full(out, gst_structure_copy(gst_caps_get_structure(caps, i)),
                                           gst_caps_features_copy(features));
        }
        return GStreamer::adoptCaps(out);
    };

    const GstQgc::SinkCapsPolicy dmaBufPolicy{.gpuZeroCopy = true,
                                              .gpuBin = GstQgc::GpuBinMode::DirectDmaBuf,
                                              .target = kDmaBufTarget,
                                              .dmaBufDisplay = QGCRhiCapture::qtEglDisplay()};
    const GStreamer::GstCapsPtr qgcGpuCaps =
        GStreamer::adoptCaps(gst_caps_from_string(GstQgc::buildSinkCaps(dmaBufPolicy).c_str()));
    QVERIFY2(qgcGpuCaps, "QGC GPU caps did not parse");
    const GStreamer::GstCapsPtr qgcDmaBufCaps = onlyDmaBufCaps(qgcGpuCaps.get());
    QVERIFY2(!gst_caps_is_empty(qgcDmaBufCaps.get()), "QGC GPU caps did not include a DMABuf structure");

    {
        GstTest::TestPipeline preflight;
        if (!preflight.launch(QStringLiteral("videotestsrc num-buffers=2 ! "
                                             "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! "
                                             "vah264enc ! h264parse ! vah264dec ! vapostproc ! "
                                             "capsfilter name=qgccaps ! fakesink sync=false"))) {
            QSKIP(qPrintable(QStringLiteral("DMABuf preflight parse skipped: %1").arg(preflight.error())));
        }
        g_object_set(preflight.element("qgccaps").get(), "caps", qgcDmaBufCaps.get(), nullptr);

        if (!preflight.play()) {
            QSKIP("DMABuf preflight failed to PLAY (no VA driver / DRI device?)");
        }
        const QString failure = preflight.runToEos();
        if (!failure.isEmpty()) {
            const GStreamer::GCharPtr qgcCapsStr = GStreamer::adoptGChar(gst_caps_to_string(qgcDmaBufCaps.get()));
            QSKIP(qPrintable(QStringLiteral("VA driver cannot produce QGC-compatible direct DMABuf caps (%1). QGC: %2")
                                 .arg(failure, QString::fromUtf8(qgcCapsStr ? qgcCapsStr.get() : ""))));
        }
    }

    QVideoSink videoSink;
    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("videotestsrc num-buffers=10 ! "
                                        "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! "
                                        "vah264enc ! h264parse ! vah264dec ! vapostproc ! "
                                        "capsfilter name=qgccaps ! "
                                        "qgcvideosinkbin name=sink gpu-zerocopy=true"))) {
        QSKIP(qPrintable(QStringLiteral("DMABuf pipeline parse skipped: %1").arg(pipeline.error())));
    }
    g_object_set(pipeline.element("qgccaps").get(), "caps", qgcDmaBufCaps.get(), nullptr);

    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    // Without a real QVideoSink, makeAdapterContext(true) never sets gpuEnabled / resolves the
    // EGLDisplay, so show_frame's HwVideoBufferContext stays CPU-only and the DmaBuf counter never moves.
    auto owner = std::make_unique<QObject>();
    if (!GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, owner.get())) {
        QSKIP("setupQVideoSinkElement() failed (no GPU context for DMABuf)");
    }

    const GStreamer::GstElementPtr qvideosink = GstQgcTest::qvideosink(sinkBin.get());
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(qvideosink.get(), "sink"));
    QVERIFY2(sinkPad, "qgcqvideosink has no sink pad");

    struct ProbeState
    {
        std::atomic<int> bufferCount{0};
        std::atomic<int> dmaBufCount{0};
    } probe;

    auto probeCb = +[](GstPad* /*pad*/, GstPadProbeInfo* info, gpointer userData) -> GstPadProbeReturn {
        auto* st = static_cast<ProbeState*>(userData);
        GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
        if (buf) {
            st->bufferCount.fetch_add(1, std::memory_order_relaxed);
            GstMemory* mem = gst_buffer_peek_memory(buf, 0);
            if (mem && gst_is_dmabuf_memory(mem)) {
                st->dmaBufCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return GST_PAD_PROBE_OK;
    };
    GStreamer::PadProbe bufferProbe;
    QVERIFY(bufferProbe.add(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, probeCb, &probe));

    if (!pipeline.play()) {
        QSKIP("DMABuf pipeline failed to PLAY (no VA driver / DRI device?)");
    }

    const GStreamer::GstMessagePtr msg = pipeline.waitForEosOrError();
    const QString negotiatedStr = GstTest::currentCapsString(sinkPad.get());
    if (msg && (GST_MESSAGE_TYPE(msg.get()) == GST_MESSAGE_ERROR)) {
        // GST_ELEMENT_FLOW_ERROR carries the streaming thread's GstFlowReturn as "flow-return".
        const GstStructure* details = nullptr;
        gst_message_parse_error_details(msg.get(), &details);
        gint flowReturn = GST_FLOW_OK;
        const bool notNegotiated = details && gst_structure_get_int(details, "flow-return", &flowReturn) &&
                                   (flowReturn == GST_FLOW_NOT_NEGOTIATED);
        // The preflight already ran the VA elements. The sink holding the direct bin's DMABuf caps when the source
        // stops with not-negotiated means the driver can't allocate any layout the sink offered (iHD: "driver set a
        // modifier different from allowed list").
        if (notNegotiated && negotiatedStr.contains(QStringLiteral("memory:DMABuf"))) {
            QSKIP(qPrintable(QStringLiteral("VA driver exports no DMABuf layout Qt's EGL display can import. Agreed "
                                            "caps: %1")
                                 .arg(negotiatedStr)));
        }
        QSKIP(qPrintable(
            QStringLiteral("VA pipeline into the direct DMABuf bin failed: %1").arg(GstTest::errorText(msg.get()))));
    }

    const auto dmaBufFrames = [&qvideosink] {
        guint64 frames = 0;
        if (const GStreamer::GstStructurePtr stats = GstQgc::frameStats(qvideosink.get())) {
            (void) gst_structure_get_uint64(stats.get(), GstQgc::sinkFramesField(HwVideoBufferPath::DmaBuf), &frames);
        }
        return frames;
    };
    const int bufferCount = probe.bufferCount.load();
    const int dmaBufCount = probe.dmaBufCount.load();
    QVERIFY2(bufferCount > 0, "No buffers reached qgcqvideosink under DMABuf caps");
    if (dmaBufCount == 0 || !negotiatedStr.contains(QStringLiteral("memory:DMABuf"))) {
        QSKIP(qPrintable(QStringLiteral("VA driver chose system memory, not DMABuf. Buffers: %1, "
                                        "negotiated caps: %2")
                             .arg(bufferCount)
                             .arg(negotiatedStr)));
    }
    QVERIFY2(
        QTest::qWaitFor([&] { return dmaBufFrames() > 0; }, TestTimeout::shortDuration()),
        qPrintable(QStringLiteral("DMABuf negotiated but the sink delivered no DmaBuf frames. Buffers: %1, caps: %2")
                       .arg(bufferCount)
                       .arg(negotiatedStr)));
#endif
}

void GstHwBuffersTest::_testDmaDrmModifierPolicy_data()
{
    QTest::addColumn<quint64>("modifier");
    QTest::addColumn<bool>("importable");

    constexpr quint64 kIntel = 0x01ULL << 56;
    constexpr quint64 kAmd = 0x02ULL << 56;
    constexpr quint64 kAmdGfx9Swizzle = (0x19ULL << 8) | 0x1ULL;  // TILE=64K_R_X, TILE_VERSION=GFX9
    QTest::newRow("linear") << quint64(0) << true;
    QTest::newRow("intel-x-tiled") << (kIntel | 1) << true;
    QTest::newRow("intel-y-tiled") << (kIntel | 2) << true;
    QTest::newRow("intel-4-tiled") << (kIntel | 9) << true;
    QTest::newRow("intel-y-tiled-ccs") << (kIntel | 4) << false;
    QTest::newRow("intel-gen12-rc-ccs") << (kIntel | 6) << false;
    QTest::newRow("intel-4-tiled-mtl-rc-ccs") << (kIntel | 13) << false;
    QTest::newRow("amd-swizzle") << (kAmd | kAmdGfx9Swizzle) << true;
    QTest::newRow("amd-swizzle-dcc") << (kAmd | kAmdGfx9Swizzle | (1ULL << 13)) << false;
    QTest::newRow("arm-afbc") << ((0x08ULL << 56) | 0x1) << false;
    QTest::newRow("qcom-ubwc") << ((0x05ULL << 56) | 0x1) << false;
    QTest::newRow("invalid") << quint64(0x00ffffffffffffffULL) << false;
}

void GstHwBuffersTest::_testDmaDrmModifierPolicy()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
    QFETCH(quint64, modifier);
    QFETCH(bool, importable);

    // Qt imports each plane as its own EGLImage: compression and auxiliary (CCS/DCC) planes can't survive that.
    QCOMPARE(GstHw::dmaDrmModifierImportableForTest(modifier), importable);
#else
    QSKIP("DMABuf DMA_DRM caps unavailable in this build");
#endif
}

void GstHwBuffersTest::_testDmaDrmPlaneFourccsMatchQtImporter()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
    // The caps only offer a modifier every plane can be sampled with, as the importer will sample it.
    for (const GstQgc::VideoFormatEntry& entry : GstQgc::kVideoFormatTable) {
        if (!entry.capsToken || !(entry.offers & GstQgc::kOfferDmaDrm)) {
            continue;
        }
        const GstQtDmaBufImport::PlaneFourccs imported = GstQtDmaBufImport::planeFourccs(entry.qt);
        const guint planes = GST_VIDEO_FORMAT_INFO_N_PLANES(gst_video_format_get_info(entry.gst));
        QCOMPARE(imported.count, int(planes));
        for (guint plane = 0; plane < planes; ++plane) {
            QCOMPARE(GstHw::dmaDrmPlaneImportFourccForTest(entry.gst, plane), imported.fourcc[plane]);
        }
    }
    // BGRA's bytes are B,G,R,A: DRM ARGB8888, which samples red as red.
    QCOMPARE(GstQtDmaBufImport::planeFourccs(QVideoFrameFormat::Format_BGRA8888).fourcc[0],
             quint32(GST_MAKE_FOURCC('A', 'R', '2', '4')));
#else
    QSKIP("DMABuf DMA_DRM caps unavailable in this build");
#endif
}

void GstHwBuffersTest::_testDmaBufPathNeedsQtEglDisplay()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    if (QGCRhiCapture::qtEglDisplay()) {
        QSKIP("Qt renders with EGL on this platform");
    }
    HwVideoBufferContext context = HwBuffers::makeAdapterContext(true);
    context.dmaBufEglDisplay = HwBuffers::dmaBufImportDisplay();
    QVERIFY(!context.dmaBufEglDisplay);

    // Without Qt's EGLDisplay no import can succeed: DMABuf memory must take the CPU path before any import attempt.
    GstVideoInfo info;
    GstSample* sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    const auto sampleGuard = qScopeGuard([sample] { gst_sample_unref(sample); });
    HwVideoBufferPath path = HwVideoBufferPath::DmaBuf;
    const auto hwBuffer =
        makeHwVideoBuffer(gst_sample_get_buffer(sample), gst_sample_get_caps(sample), info,
                          QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12), context, path, nullptr);
    QVERIFY(!hwBuffer);
    QCOMPARE(path, HwVideoBufferPath::None);
#endif
}

void GstHwBuffersPipelineTest::_testHwBufferMapTexturesGuard()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else

    const quint64 failsBefore =
        GstHwPathTelemetry::peek(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures);

    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral(
            R"(videotestsrc num-buffers=1 ! capsfilter caps="video/x-raw(memory:DMABuf),format=NV12,width=64,height=64" ! )"
            "fakesink name=sink sync=false"))) {
        QSKIP("Could not construct DMABuf test pipeline (element missing)");
    }
    if (gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        QSKIP("DMABuf pipeline failed to reach PAUSED (no DMABuf source on this machine)");
    }
    (void) gst_element_get_state(pipeline.get(), nullptr, nullptr, GstTest::clockTime(TestTimeout::shortDuration()));

    GstSample* sample = nullptr;
    if (const GStreamer::GstElementPtr fakesink = pipeline.element("sink")) {
        g_object_get(fakesink.get(), "last-sample", &sample, nullptr);
    }
    if (!sample) {
        QSKIP("No DMABuf sample produced (caps negotiation failed on this machine)");
    }

    GstVideoInfo info;
    GstCaps* caps = gst_sample_get_caps(sample);
    gst_video_info_from_caps(&info, caps);
    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_NV12);

    GstDmaBufVideoBuffer buf(sample, info, fmt, EGL_NO_DISPLAY);
    QVideoFrameTexturesUPtr old;
    QVideoFrameTexturesUPtr result = buf.mapTextures(*reinterpret_cast<QRhi*>(1), old);
    QVERIFY(!result);

    const quint64 failsAfter =
        GstHwPathTelemetry::peek(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures);
    QVERIFY(failsAfter > failsBefore);

    gst_sample_unref(sample);
#endif
}

void GstHwBuffersTest::_testHwBufferFactoryCacheRejectsMemoryTypeChange()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    GstVideoInfo info;
    gst_video_info_init(&info);
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 4, 4);

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    const auto bufferGuard = qScopeGuard([&] { gst_buffer_unref(buffer); });

    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_video_info_to_caps(&info));
    QVERIFY(caps);

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    context.dmaBufEglDisplay = reinterpret_cast<EGLDisplay>(quintptr(1));

    HwResolvedPathCache cache;
    cache.path = HwVideoBufferPath::DmaBuf;
    cache.validated = true;

    HwVideoBufferPath path = HwVideoBufferPath::None;
    QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_BGRA8888);

    auto hwBuffer = makeHwVideoBuffer(buffer, caps.get(), info, format, context, path, &cache);
    QVERIFY2(!hwBuffer, "Cached DMABuf path must reject a system-memory buffer and fall back to CPU mapping");
    QCOMPARE(path, HwVideoBufferPath::None);
    QVERIFY(!cache.validated);
#endif
}

void GstHwBuffersTest::_testDmaBufTiledFrameRefusesCpuMap()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH) || !GST_CHECK_VERSION(1, 24, 0)
    QSKIP("DMABuf DMA_DRM import not compiled in this build");
#else
    GstDmaBufVideoBuffer::resetCachedState();
    const QVideoFrameFormat format(QSize(64, 64), QVideoFrameFormat::Format_NV12);
    GstVideoInfo info;

    GstSample* linearSample = makeMemfdDmaBufSample(
        GST_VIDEO_FORMAT_NV12, info, "video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=NV12,width=64,height=64");
    QVERIFY(linearSample);
    GstDmaBufVideoBuffer linear(linearSample, info, format, EGL_NO_DISPLAY);
    gst_sample_unref(linearSample);
    QCOMPARE(linear.map(QVideoFrame::ReadOnly).planeCount, 2);
    linear.unmap();

    // Reading Y-tiled memory as rows would show scrambled pixels.
    GstSample* tiledSample =
        makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info,
                              "video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=NV12:0x0100000000000002,"
                              "width=64,height=64");
    QVERIFY(tiledSample);
    GstDmaBufVideoBuffer tiled(tiledSample, info, format, EGL_NO_DISPLAY);
    gst_sample_unref(tiledSample);
    expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg,
                     QRegularExpression(QStringLiteral("tiled DMABuf frame has no CPU layout")));
    QCOMPARE(tiled.map(QVideoFrame::ReadOnly).planeCount, 0);
    verifyExpectedLogMessage();
    QVERIFY2(!tiled.validatePlaneHandles(), "A tiled frame imports only through EGL modifier attributes");
#endif
}

void GstHwBuffersTest::_testDmaBufTiledFrameDroppedWithoutImport_data()
{
    QTest::addColumn<QString>("drmFormat");
    QTest::addColumn<bool>("qtDisplay");
    QTest::addColumn<bool>("importDisabled");
    QTest::addColumn<bool>("cpuFrame");

    // DMA_DRM caps negotiated before the import was disabled, or while the sink's GPU context had no Qt EGLDisplay.
    const QString yTiled = QStringLiteral("NV12:0x0100000000000002");
    QTest::newRow("tiled, import disabled") << yTiled << true << true << false;
    QTest::newRow("tiled, no EGL display") << yTiled << false << false << false;
    QTest::newRow("linear, no EGL display") << QStringLiteral("NV12") << false << false << true;
}

void GstHwBuffersTest::_testDmaBufTiledFrameDroppedWithoutImport()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH) || !GST_CHECK_VERSION(1, 24, 0)
    QSKIP("DMABuf DMA_DRM import not compiled in this build");
#else
    QFETCH(QString, drmFormat);
    QFETCH(bool, qtDisplay);
    QFETCH(bool, importDisabled);
    QFETCH(bool, cpuFrame);

    GstDmaBufVideoBuffer::resetCachedState();
    const auto resetLatch = qScopeGuard([] { GstDmaBufVideoBuffer::resetCachedState(); });
    if (importDisabled) {
        expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg,
                         QRegularExpression(QStringLiteral("EGL_BAD_MATCH")));
        GstDmaBufVideoBuffer::recordImportFailureForTest(/*unrecoverable=*/true);
        verifyExpectedLogMessage();
        ignoreLogMessage("Video.GStreamer.HwBuffers.Factory", QtWarningMsg,
                         QRegularExpression(QStringLiteral("plane-handle validation")));
    }

    GstVideoInfo info;
    const QByteArray caps = QStringLiteral("video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=%1,width=64,height=64")
                                .arg(drmFormat)
                                .toLatin1();
    GstSample* sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info, caps.constData());
    QVERIFY(sample);
    const auto sampleGuard = qScopeGuard([sample] { gst_sample_unref(sample); });

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    // Never reaches EGL: the disabled importer refuses the frame first.
    context.dmaBufEglDisplay = qtDisplay ? reinterpret_cast<EGLDisplay>(quintptr(1)) : EGL_NO_DISPLAY;
    const MappedFrame mapped = mapSampleToFrame(gst_sample_get_buffer(sample), gst_sample_get_caps(sample), info,
                                                QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12),
                                                context, std::make_shared<std::atomic<int>>(0));

    QCOMPARE(mapped.gpuPath, HwVideoBufferPath::None);
    QCOMPARE(mapped.frame.isValid(), cpuFrame);
    QVERIFY2(mapped.noCpuLayout != cpuFrame, "a tiled frame must be dropped rather than read as linear rows");
#endif
}

void GstHwBuffersTest::_testDmaBufImportFailureLatch_data()
{
    QTest::addColumn<bool>("unrecoverable");
    QTest::addColumn<int>("failuresToDisable");
    QTest::addColumn<QString>("warning");

    QTest::newRow("EGL_BAD_MATCH disables at once") << true << 1 << QStringLiteral("EGL_BAD_MATCH");
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QTest::newRow("a run of rejected imports disables")
        << false << GstDmaBufVideoBuffer::kMaxConsecutiveImportFailures << QStringLiteral("rejected every recent");
#endif
}

void GstHwBuffersTest::_testDmaBufImportFailureLatch()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    QFETCH(bool, unrecoverable);
    QFETCH(int, failuresToDisable);
    QFETCH(QString, warning);

    GstDmaBufVideoBuffer::resetCachedState();
    const auto resetGuard = qScopeGuard([] { GstDmaBufVideoBuffer::resetCachedState(); });

    GstVideoInfo info;
    GstSample* sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    GstDmaBufVideoBuffer frame(sample, info, QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12),
                               EGL_NO_DISPLAY);
    gst_sample_unref(sample);
    QVERIFY(frame.validatePlaneHandles());

    for (int i = 1; i < failuresToDisable; ++i) {
        GstDmaBufVideoBuffer::recordImportFailureForTest(unrecoverable);
    }
    QVERIFY(frame.validatePlaneHandles());

    expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg, QRegularExpression(warning));
    GstDmaBufVideoBuffer::recordImportFailureForTest(unrecoverable);
    verifyExpectedLogMessage();
    QVERIFY(GstDmaBufVideoBuffer::importDisabled());
    QVERIFY2(!frame.validatePlaneHandles(), "A disabled importer must send frames to the CPU path");
#endif
}

void GstHwBuffersTest::_testDmaBufVulkanImportRejectsMultiPlanar_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("rhiBackend");
    QTest::addColumn<bool>("expectAccepted");

    QTest::newRow("NV12 under Vulkan") << int(GST_VIDEO_FORMAT_NV12) << int(QRhi::Vulkan) << false;
    QTest::newRow("P010 under Vulkan") << int(GST_VIDEO_FORMAT_P010_10LE) << int(QRhi::Vulkan) << false;
    QTest::newRow("I420 under Vulkan") << int(GST_VIDEO_FORMAT_I420) << int(QRhi::Vulkan) << false;
    QTest::newRow("RGBA under Vulkan") << int(GST_VIDEO_FORMAT_RGBA) << int(QRhi::Vulkan) << true;
    QTest::newRow("BGRA under Vulkan") << int(GST_VIDEO_FORMAT_BGRA) << int(QRhi::Vulkan) << true;
    QTest::newRow("NV12 under OpenGL") << int(GST_VIDEO_FORMAT_NV12) << int(QRhi::OpenGLES2) << true;
}

void GstHwBuffersTest::_testDmaBufVulkanImportRejectsMultiPlanar()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH) || !defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QSKIP("DMABuf->Vulkan import not compiled in this build");
#else
    QFETCH(int, format);
    QFETCH(int, rhiBackend);
    QFETCH(bool, expectAccepted);

    auto& snapshotBackend = QGCRhiCapture::deviceSnapshot().backend;
    const int previousBackend = snapshotBackend.load();
    const auto restoreBackend = qScopeGuard([&] { snapshotBackend.store(previousBackend); });
    snapshotBackend.store(rhiBackend);

    GstVideoInfo info;
    GstSample* sample = makeMemfdDmaBufSample(static_cast<GstVideoFormat>(format), info);
    QVERIFY(sample);
    const GstDmaBufVideoBuffer frame(sample, info, QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12),
                                     EGL_NO_DISPLAY);
    gst_sample_unref(sample);

    QCOMPARE(frame.validatePlaneHandles(), expectAccepted);
#endif
}

#endif
