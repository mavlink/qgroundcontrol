#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <memory>
#include <rhi/qrhi.h>
#include <vector>
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <gst/allocators/gstdmabuf.h>
#include <sys/mman.h>
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
#include "HwBuffers/common/HwBuffers.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcVideoFormats.h"
#include "gstqgc/gstqgcqvideosink.h"
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#endif
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCRhiCapture.h"
#endif

void GStreamerTest::_testDmaBufDispatch()
{
#if !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QSKIP("Direct-DMABuf bin not compiled in this build");
#endif
    const HwBuffers::ScopedGpuBinModeForTest directBin(HwBuffers::GpuBinMode::DirectDmaBuf);

    const QString missingVa = GstTest::firstMissingFactory({"vah264enc", "vah264dec", "vapostproc"});
    if (!missingVa.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("No VA-API stack: %1 is not registered").arg(missingVa)));
    }

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

    const GStreamer::GstCapsPtr qgcGpuCaps = GStreamer::adoptCaps(
        gst_caps_from_string(GstQgc::buildGpuCapsString(HwBuffers::GpuBinMode::DirectDmaBuf).c_str()));
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

    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    const GStreamer::GstPadPtr appsinkPad = GStreamer::adoptPad(gst_element_get_static_pad(qvideosink.get(), "sink"));
    QVERIFY2(appsinkPad, "qgcqvideosink has no sink pad");

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
            if (mem && mem->allocator && mem->allocator->mem_type &&
                g_str_has_prefix(mem->allocator->mem_type, "DMABuf")) {
                st->dmaBufCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return GST_PAD_PROBE_OK;
    };
    GStreamer::PadProbe bufferProbe;
    QVERIFY(bufferProbe.add(appsinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, probeCb, &probe));

    if (!pipeline.play()) {
        QSKIP("DMABuf pipeline failed to PLAY (no VA driver / DRI device?)");
    }

    const GStreamer::GstMessagePtr msg = pipeline.waitForEosOrError();
    const QString negotiatedStr = GstTest::currentCapsString(appsinkPad.get());
    if (msg && (GST_MESSAGE_TYPE(msg.get()) == GST_MESSAGE_ERROR)) {
        // GST_ELEMENT_FLOW_ERROR carries the streaming thread's GstFlowReturn as "flow-return".
        const GstStructure* details = nullptr;
        gst_message_parse_error_details(msg.get(), &details);
        gint flowReturn = GST_FLOW_OK;
        const bool notNegotiated = details && gst_structure_get_int(details, "flow-return", &flowReturn) &&
                                   (flowReturn == GST_FLOW_NOT_NEGOTIATED);
        // The preflight already ran the VA elements. The sink holding the direct bin's DMABuf caps when the source
        // stops with not-negotiated means the driver couldn't allocate them LINEAR (iHD: "driver set a modifier
        // different from allowed list").
        if (notNegotiated && negotiatedStr.contains(QStringLiteral("memory:DMABuf"))) {
            QSKIP(qPrintable(QStringLiteral("VA driver exports only tiled (non-LINEAR) DMABuf, which the direct DMABuf "
                                            "bin can't import. Agreed caps: %1")
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
}

void GStreamerTest::_testDmaDrmCapsRejectNonLinearModifiers()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
    constexpr quint64 kIntelTileYModifier = 0x0100000000000009ULL;
    QVERIFY2(!GstHw::dmaDrmModifierAdvertisedForTest(kIntelTileYModifier),
             "Direct QGC DMABuf caps must not advertise tiled VA modifiers; Mesa/Gallium can segfault while "
             "binding the resulting EGLImage on Qt's render thread");
#else
    QSKIP("DMABuf DMA_DRM caps unavailable in this build");
#endif
}

void GStreamerTest::_testLinearDmaDrmCapsFollowFormatTable()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
    // Every format the table offers as DMA_DRM, in table order: YV12 imports exactly as I420 does.
    const std::vector<GstVideoFormat> formats = GstQgc::advertisedFormats(GstQgc::kOfferLinearDmaDrm);
    QCOMPARE(QString::fromStdString(GstHw::buildLinearDmaDrmCaps(formats)),
             QStringLiteral("video/x-raw(memory:DMABuf), format=DMA_DRM, "
                            "drm-format={ NV12, NV21, YU12, YV12, P010, AR24, AB24 }; "));
#else
    QSKIP("DMABuf DMA_DRM caps unavailable in this build");
#endif
}

void GStreamerTest::_testDmaBufRejectsNonLinearDirectImport()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    constexpr guint64 kLinearModifier = 0;
    constexpr guint64 kIntelTileYModifier = 0x0100000000000009ULL;

    QVERIFY(GstDmaBufVideoBuffer::directGlImportAllowedForTest(kLinearModifier));
    QVERIFY2(!GstDmaBufVideoBuffer::directGlImportAllowedForTest(kIntelTileYModifier),
             "Direct QGC DMABuf must reject tiled VA modifiers whether or not EGL advertises modifier support; binding "
             "the resulting EGLImage can segfault in Mesa/Gallium");
#else
    QSKIP("DMABuf unavailable in this build");
#endif
}

void GStreamerTest::_testDmaBufTiledImportAvoidsTexStorage()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QVERIFY(GstDmaBufVideoBuffer::texStorageImportAllowedForTest(false, true));
    QVERIFY(!GstDmaBufVideoBuffer::texStorageImportAllowedForTest(true, true));
    QVERIFY(!GstDmaBufVideoBuffer::texStorageImportAllowedForTest(false, false));
#else
    QSKIP("DMABuf unavailable in this build");
#endif
}

void GStreamerTest::_testHwBufferMapTexturesGuard()
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

void GStreamerTest::_testHwBufferFactoryCacheRejectsMemoryTypeChange()
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

void GStreamerTest::_testDmaBufSingleFdImportEnvGate()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    TestFixtures::EnvVarFixture gate("QGC_GST_DMABUF_SINGLE_EGLIMAGE");

    qunsetenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE");
    QVERIFY(HwBuffers::readHwBufferEnvConfig().dmaBufSingleEglImage);

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "0");
    QVERIFY(!HwBuffers::readHwBufferEnvConfig().dmaBufSingleEglImage);

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "false");
    QVERIFY(!HwBuffers::readHwBufferEnvConfig().dmaBufSingleEglImage);

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "off");
    QVERIFY(!HwBuffers::readHwBufferEnvConfig().dmaBufSingleEglImage);

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "1");
    QVERIFY(HwBuffers::readHwBufferEnvConfig().dmaBufSingleEglImage);

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "force");
    QVERIFY(HwBuffers::readHwBufferEnvConfig().dmaBufSingleEglImage);
#endif
}

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
namespace {

// Single-memory DMABuf sample backed by a memfd: enough for the streaming-thread validatePlaneHandles() gate.
GstSample* makeMemfdDmaBufSample(GstVideoFormat format, GstVideoInfo& info)
{
    gst_video_info_set_format(&info, format, 64, 64);
    const int fd = memfd_create("qgc-dmabuf-test", MFD_CLOEXEC);
    if (fd < 0) {
        return nullptr;
    }
    if (ftruncate(fd, static_cast<off_t>(GST_VIDEO_INFO_SIZE(&info))) != 0) {
        ::close(fd);
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
    GstCaps* caps = gst_video_info_to_caps(&info);
    GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr);
    gst_buffer_unref(buffer);
    gst_caps_unref(caps);
    return sample;
}

}  // namespace
#endif

void GStreamerTest::_testDmaBufEglBadMatchLatch_data()
{
    QTest::addColumn<bool>("singleFd");
    QTest::addColumn<bool>("expectDisabled");

    QTest::newRow("single-fd keeps per-plane import") << true << false;
    QTest::newRow("per-plane disables path") << false << true;
}

void GStreamerTest::_testDmaBufEglBadMatchLatch()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    QFETCH(bool, singleFd);
    QFETCH(bool, expectDisabled);

    GstDmaBufVideoBuffer::resetCachedState();
    const auto resetGuard = qScopeGuard([] { GstDmaBufVideoBuffer::resetCachedState(); });

    GstVideoInfo info;
    GstSample* sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    GstDmaBufVideoBuffer frame(sample, info, QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12),
                               EGL_NO_DISPLAY);
    gst_sample_unref(sample);
    QVERIFY(frame.validatePlaneHandles());

    if (expectDisabled) {
        expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg,
                         QRegularExpression(QStringLiteral("EGL_BAD_MATCH.*disabling DMABuf zero-copy")));
    }
    GstDmaBufVideoBuffer::recordEglBadMatchForTest(singleFd);
    if (expectDisabled) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(frame.validatePlaneHandles(), !expectDisabled);
#endif
}

void GStreamerTest::_testDmaBufVulkanImportRejectsMultiPlanar_data()
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

void GStreamerTest::_testDmaBufVulkanImportRejectsMultiPlanar()
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
