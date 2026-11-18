#include "HwBuffers/GstHwBuffersPipelineTest.h"
#include "HwBuffers/GstHwBuffersTest.h"

#include <atomic>
#include <cerrno>
#include <gst/gst.h>
#include <limits>
#include <memory>
#include <optional>
#include <rhi/qrhi.h>
#include <span>
#include <thread>
#include <vector>
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <gst/allocators/gstdmabuf.h>
#include <gst/app/gstappsrc.h>
#include <unistd.h>
#endif

#include <QtCore/QMutex>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QThread>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBufferFactory.h"
#include "GstQgcFrameMap.h"
#include "HwBuffers/HwBuffers.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcRenderTarget.h"
#include "gstqgc/GstQgcSinkStats.h"
#include "gstqgc/GstQgcTestSupport.h"
#include "gstqgc/GstQgcVideoFormats.h"
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#include "GstDmaBufVulkanImport.h"
#include "GstQtDmaBufImport.h"
#endif
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCRhiCapture.h"
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
namespace {

// Single-memory 64x64 DMABuf sample backed by a memfd: enough for the streaming-thread gates and CPU map. @p capsString
// replaces the system-memory caps (e.g. DMA_DRM caps).
GstTest::SamplePtr makeMemfdDmaBufSample(GstVideoFormat format, GstVideoInfo& info, const char* capsString = nullptr)
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
    GstTest::SamplePtr sample(gst_sample_new(buffer, caps, nullptr, nullptr));
    gst_buffer_unref(buffer);
    gst_caps_unref(caps);
    return sample;
}

/// What _testDmaBufImportFailureLatch imports between a stream's failed imports.
enum class ImportedBetween
{
    Nothing,
    OtherStream,
    SameStream,
};

}  // namespace
#endif

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
namespace {

/// What stands in for a Vulkan device where the import's bookkeeping is tested without one: it creates images unless
/// told to refuse them, its fences signal once the test lets the GPU be done, and it counts what is destroyed.
struct StandInVulkanDevice
{
    std::atomic<bool> refusesImages{false};
    std::atomic<bool> gpuDone{false};
    std::atomic<int> imagesDestroyed{0};
    std::atomic<int> memoriesFreed{0};
    std::atomic<int> fencesDestroyed{0};
    std::atomic<Qt::HANDLE> imageDestroyedOn{nullptr};

    void restart()
    {
        refusesImages = false;
        gpuDone = false;
        imagesDestroyed = 0;
        memoriesFreed = 0;
        fencesDestroyed = 0;
        imageDestroyedOn = nullptr;
    }

    void finishGpuWork()
    {
        gpuDone = true;
        gpuDone.notify_all();
    }
};

// Process-wide: the entry points below are plain functions, and a waiter thread may outlive a failed test.
StandInVulkanDevice s_standInVulkanDevice;

/// The entry points of s_standInVulkanDevice that importing a plane up to its fd and releasing a frame call.
GstDmaBufVulkan::DeviceFns standInVulkanDeviceFns()
{
    GstDmaBufVulkan::DeviceFns fns;
    fns.createImage = [](VkDevice, const VkImageCreateInfo*, const VkAllocationCallbacks*, VkImage* image) {
        if (s_standInVulkanDevice.refusesImages) {
            return VK_ERROR_FORMAT_NOT_SUPPORTED;
        }
        *image = VkImage(quintptr(1));
        return VK_SUCCESS;
    };
    fns.destroyImage = [](VkDevice, VkImage, const VkAllocationCallbacks*) {
        ++s_standInVulkanDevice.imagesDestroyed;
        s_standInVulkanDevice.imageDestroyedOn = QThread::currentThreadId();
    };
    fns.freeMemory = [](VkDevice, VkDeviceMemory, const VkAllocationCallbacks*) {
        ++s_standInVulkanDevice.memoriesFreed;
    };
    fns.createFence = [](VkDevice, const VkFenceCreateInfo*, const VkAllocationCallbacks*, VkFence* fence) {
        *fence = VkFence(quintptr(1));
        return VK_SUCCESS;
    };
    fns.destroyFence = [](VkDevice, VkFence, const VkAllocationCallbacks*) { ++s_standInVulkanDevice.fencesDestroyed; };
    fns.getFenceStatus = [](VkDevice, VkFence) { return s_standInVulkanDevice.gpuDone ? VK_SUCCESS : VK_NOT_READY; };
    fns.waitForFences = [](VkDevice, uint32_t, const VkFence*, VkBool32, uint64_t) {
        s_standInVulkanDevice.gpuDone.wait(false);
        return VK_SUCCESS;
    };
    fns.queueSubmit = [](VkQueue, uint32_t, const VkSubmitInfo*, VkFence) { return VK_SUCCESS; };
    fns.queueWaitIdle = [](VkQueue) { return VK_SUCCESS; };
    return fns;
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
        return GstQgc::frameStatsField(GstQgc::frameStats(qvideosink.get()).get(),
                                       GstQgc::sinkFramesField(HwVideoBufferPath::DmaBuf));
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

void GstHwBuffersPipelineTest::_testDmaBufOrGlUploadBinRoutesByMemory()
{
#if !defined(QGC_GST_BIN_HAS_GLUPLOAD) || !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QSKIP("The DMABuf-or-glupload bin needs both GPU bins compiled");
#else
    if (!GstQgcTest::hasMemorySwitch()) {
        QSKIP("No switchbin of GStreamer 1.24 or later: the bin is the glupload variant");
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "glupload", "glcolorconvert");
    // A stand-in for Qt's EGLDisplay makes the sink offer DMABuf. Nothing imports with it: no QVideoSink is bound, so
    // the sink drops each frame once it has arrived.
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = GstQgc::RenderBackend::OpenGL, .egl = true},
         .gpuBin = {.mode = GstQgc::GpuBinMode::DmaBufOrGlUpload},
         .dmaBufDisplay = reinterpret_cast<void*>(quintptr(1))});

    GstTest::TestPipeline pipeline;
    QVERIFY2(
        pipeline.launch(QStringLiteral("appsrc name=src format=time ! qgcvideosinkbin name=sink gpu-zerocopy=true")),
        qPrintable(pipeline.error()));
    const GStreamer::GstElementPtr source = pipeline.element("src");
    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(source && sinkBin);
    const GStreamer::GstElementPtr qvideosink = GstQgcTest::qvideosink(sinkBin.get());
    QVERIFY(qvideosink);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(qvideosink.get(), "sink"));
    QVERIFY(sinkPad);

    // The memory of each buffer that reaches the sink: 'D' a dma-buf, 'G' GLMemory, '?' anything else.
    struct Arrivals
    {
        QMutex mutex;
        QByteArray memory;
    } arrivals;

    GStreamer::PadProbe arrivalProbe;
    QVERIFY(arrivalProbe.add(
        sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER,
        +[](GstPad*, GstPadProbeInfo* info, gpointer userData) -> GstPadProbeReturn {
            auto* const seen = static_cast<Arrivals*>(userData);
            GstMemory* const memory = gst_buffer_peek_memory(GST_PAD_PROBE_INFO_BUFFER(info), 0);
            const char* const type = (memory && memory->allocator) ? memory->allocator->mem_type : nullptr;
            const QMutexLocker lock(&seen->mutex);
            seen->memory +=
                gst_is_dmabuf_memory(memory) ? 'D' : ((type && g_str_has_prefix(type, "GLMemory")) ? 'G' : '?');
            return GST_PAD_PROBE_OK;
        },
        &arrivals));

    const auto push = [&source](char memory) {
        GstTest::SamplePtr sample;
        GstVideoInfo info;
        if (memory == 'D') {
            sample =
                makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info,
                                      "video/x-raw(memory:DMABuf), format=NV12, width=64, height=64, framerate=30/1");
        } else {
            gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 64, 64);
            GstBuffer* const buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
            gst_buffer_memset(buffer, 0, 0x80, GST_VIDEO_INFO_SIZE(&info));
            const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(
                gst_caps_from_string("video/x-raw, format=BGRA, width=64, height=64, framerate=30/1"));
            sample.reset(gst_sample_new(buffer, caps.get(), nullptr, nullptr));
            gst_buffer_unref(buffer);
        }
        return sample && (gst_app_src_push_sample(GST_APP_SRC(source.get()), sample.get()) == GST_FLOW_OK);
    };

    // Twice: the bin outlives a stream, and the second starts in the memory the first one ended in, then in the other.
    for (const char* const stream : {"GGDDGG", "GDDG"}) {
        {
            const QMutexLocker lock(&arrivals.mutex);
            arrivals.memory.clear();
        }
        QVERIFY(pipeline.play());
        for (const char* memory = stream; *memory != '\0'; ++memory) {
            QVERIFY2(push(*memory), "appsrc refused a frame");
        }
        QCOMPARE(gst_app_src_end_of_stream(GST_APP_SRC(source.get())), GST_FLOW_OK);
        const GStreamer::GstMessagePtr message = pipeline.waitForEosOrError();
        QVERIFY2(message, "the pipeline neither ended nor failed");
        if (GST_MESSAGE_TYPE(message.get()) == GST_MESSAGE_ERROR) {
            const QString failure = GstTest::errorText(message.get());
            // glupload needs a GL context, which a headless runner may not have.
            if (failure.contains(QStringLiteral("egl"), Qt::CaseInsensitive) ||
                failure.contains(QStringLiteral("gl context"), Qt::CaseInsensitive)) {
                QSKIP(qPrintable(QStringLiteral("glupload needs a GL context (headless env): %1").arg(failure)));
            }
            QFAIL(qPrintable(failure));
        }
        {
            const QMutexLocker lock(&arrivals.mutex);
            QCOMPARE(arrivals.memory, QByteArray(stream));
        }
        QCOMPARE(gst_element_set_state(pipeline.get(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
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
    context.dmaBufImporter = HwBuffers::dmaBufImportDisplay();
    QVERIFY(!context.dmaBufImporter);

    // Without Qt's EGLDisplay no import can succeed: DMABuf memory must take the CPU path before any import attempt.
    GstVideoInfo info;
    const GstTest::SamplePtr sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    HwVideoBufferPath path = HwVideoBufferPath::DmaBuf;
    const auto hwBuffer =
        makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), gst_sample_get_caps(sample.get()), info,
                          QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12), context, path, nullptr);
    QVERIFY(!hwBuffer);
    QCOMPARE(path, HwVideoBufferPath::None);
#endif
}

void GstHwBuffersTest::_testDmaBufPathResumesOnceDisplayIsKnown()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    GstDmaBufVideoBuffer::resetCachedState();
    GstVideoInfo info;
    const GstTest::SamplePtr sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    const QVideoFrameFormat format(QSize(64, 64), QVideoFrameFormat::Format_NV12);
    HwResolvedPathCache cache;
    const auto pathTaken = [&](const HwVideoBufferContext& context) {
        HwVideoBufferPath path = HwVideoBufferPath::None;
        const auto hwBuffer = makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), gst_sample_get_caps(sample.get()),
                                                info, format, context, path, &cache);
        return path;
    };

    // Within one caps epoch: a frame arrives before Qt's EGLDisplay is known, the next one after.
    HwVideoBufferContext context;
    context.gpuEnabled = true;
    QCOMPARE(pathTaken(context), HwVideoBufferPath::None);
    context.dmaBufImporter = reinterpret_cast<EGLDisplay>(quintptr(1));
    QCOMPARE(pathTaken(context), HwVideoBufferPath::DmaBuf);
#endif
}

void GstHwBuffersTest::_testDmaBufMapTexturesRejectsForeignRhi()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);
    GstVideoInfo info;
    GstTest::SamplePtr sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    GstDmaBufVideoBuffer frame(sample.get(), info, QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12),
                               EGL_NO_DISPLAY);
    sample.reset();
    const quint64 failuresBefore =
        GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures);

    // Warned once, and once more after each reset: a recreated scene graph reports its failures again.
    QVideoFrameTexturesUPtr none;
    for (int sceneGraph = 0; sceneGraph < 2; ++sceneGraph) {
        GstDmaBufVideoBuffer::resetCachedState();
        expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg,
                         QRegularExpression(QStringLiteral("QRhi backend is")));
        QVERIFY2(!frame.mapTextures(*rhi, none), "Only a GL or Vulkan QRhi can import a DMABuf frame");
        verifyExpectedLogMessage();
        QVERIFY(!frame.mapTextures(*rhi, none));
    }
    QCOMPARE(GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures),
             failuresBefore + 4);
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
    context.dmaBufImporter = reinterpret_cast<EGLDisplay>(quintptr(1));

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

    GstTest::SamplePtr linearSample = makeMemfdDmaBufSample(
        GST_VIDEO_FORMAT_NV12, info, "video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=NV12,width=64,height=64");
    QVERIFY(linearSample);
    GstDmaBufVideoBuffer linear(linearSample.get(), info, format, EGL_NO_DISPLAY);
    linearSample.reset();
    QCOMPARE(linear.map(QVideoFrame::ReadOnly).planeCount, 2);
    linear.unmap();

    // Reading Y-tiled memory as rows would show scrambled pixels.
    GstTest::SamplePtr tiledSample =
        makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info,
                              "video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=NV12:0x0100000000000002,"
                              "width=64,height=64");
    QVERIFY(tiledSample);
    GstDmaBufVideoBuffer tiled(tiledSample.get(), info, format, EGL_NO_DISPLAY);
    tiledSample.reset();
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
    const GstTest::SamplePtr sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info, caps.constData());
    QVERIFY(sample);

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    // Never reaches EGL: the disabled importer refuses the frame first.
    context.dmaBufImporter = qtDisplay ? reinterpret_cast<EGLDisplay>(quintptr(1)) : EGL_NO_DISPLAY;
    const MappedFrame mapped = mapSampleToFrame(gst_sample_get_buffer(sample.get()), gst_sample_get_caps(sample.get()),
                                                info, QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12),
                                                context, std::make_shared<std::atomic<int>>(0));

    QCOMPARE(mapped.gpuPath, HwVideoBufferPath::None);
    QCOMPARE(mapped.frame.isValid(), cpuFrame);
    QVERIFY2(mapped.noCpuLayout != cpuFrame, "a tiled frame must be dropped rather than read as linear rows");
#endif
}

void GstHwBuffersTest::_testDmaBufImportFailureLatch_data()
{
    QTest::addColumn<int>("outcome");
    QTest::addColumn<bool>("tiled");
    QTest::addColumn<int>("importedBetween");
    QTest::addColumn<int>("failuresToDisable");
    QTest::addColumn<QString>("warning");

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    using Outcome = GstDmaBufVideoBuffer::ImportOutcome;
    const int limit = GstDmaBufVideoBuffer::kMaxConsecutiveImportFailures;
    const int never = 0;
    const QString failed = QStringLiteral("Every recent DMABuf import of a stream failed");
    QTest::newRow("EGL_BAD_MATCH disables at once")
        << int(Outcome::Unrecoverable) << false << int(ImportedBetween::Nothing) << 1
        << QStringLiteral("EGL_BAD_MATCH");
    QTest::newRow("a run of rejected imports disables")
        << int(Outcome::Rejected) << false << int(ImportedBetween::Nothing) << limit << failed;
    // Two streams import in turn on the render thread.
    QTest::newRow("another stream's imports don't end the run")
        << int(Outcome::Rejected) << false << int(ImportedBetween::OtherStream) << limit << failed;
    QTest::newRow("the stream's own import ends the run")
        << int(Outcome::Rejected) << false << int(ImportedBetween::SameStream) << never << QString();
    QTest::newRow("a LINEAR frame the QRhi can't import is left to the CPU map")
        << int(Outcome::NotImportable) << false << int(ImportedBetween::Nothing) << never << QString();
    // The process is out of file descriptors, which is not the driver's refusal.
    QTest::newRow("a LINEAR frame whose fd can't be duplicated is left to the CPU map")
        << int(Outcome::FdDupFailed) << false << int(ImportedBetween::Nothing) << never << QString();
#if GST_CHECK_VERSION(1, 24, 0)
    QTest::newRow("a tiled frame the QRhi can't import has no other way out")
        << int(Outcome::NotImportable) << true << int(ImportedBetween::Nothing) << limit << failed;
    QTest::newRow("a tiled frame whose fd can't be duplicated has no other way out either")
        << int(Outcome::FdDupFailed) << true << int(ImportedBetween::Nothing) << limit
        << QStringLiteral("could not be duplicated");
#endif
#endif
}

void GstHwBuffersTest::_testDmaBufImportFailureLatch()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    QFETCH(int, outcome);
    QFETCH(bool, tiled);
    QFETCH(int, importedBetween);
    QFETCH(int, failuresToDisable);
    QFETCH(QString, warning);
    using Outcome = GstDmaBufVideoBuffer::ImportOutcome;

    GstDmaBufVideoBuffer::resetCachedState();
    const auto resetGuard = qScopeGuard([] { GstDmaBufVideoBuffer::resetCachedState(); });

    GstVideoInfo info;
    GstTest::SamplePtr sample = makeMemfdDmaBufSample(
        GST_VIDEO_FORMAT_NV12, info,
        tiled ? "video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=NV12:0x0100000000000002,width=64,height=64"
              : nullptr);
    QVERIFY(sample);
    const GstDmaBufVideoBuffer frame(sample.get(), info,
                                     QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12), EGL_NO_DISPLAY);
    sample.reset();
    GstVideoInfo otherInfo;
    GstTest::SamplePtr otherSample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_RGBA, otherInfo);
    QVERIFY(otherSample);
    const GstDmaBufVideoBuffer otherStream(otherSample.get(), otherInfo,
                                           QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_RGBA8888),
                                           EGL_NO_DISPLAY);
    otherSample.reset();

    const int attempts =
        (failuresToDisable > 0) ? failuresToDisable : (2 * GstDmaBufVideoBuffer::kMaxConsecutiveImportFailures);
    for (int attempt = 1; attempt <= attempts; ++attempt) {
        const bool disables = (attempt == failuresToDisable);
        if (disables) {
            expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg, QRegularExpression(warning));
        }
        frame.noteImportOutcomeForTest(static_cast<Outcome>(outcome));
        if (disables) {
            verifyExpectedLogMessage();
        }
        QCOMPARE(GstDmaBufVideoBuffer::importDisabled(), disables);
        if (importedBetween == int(ImportedBetween::OtherStream)) {
            otherStream.noteImportOutcomeForTest(Outcome::Imported);
        } else if (importedBetween == int(ImportedBetween::SameStream)) {
            frame.noteImportOutcomeForTest(Outcome::Imported);
        }
    }
    if (failuresToDisable > 0) {
        QVERIFY2(!frame.validatePlaneHandles(), "A disabled importer must send frames to the CPU path");
    }
#endif
}

void GstHwBuffersTest::_testDmaBufProducerWaitNeedsSignalledFences_data()
{
    QTest::addColumn<bool>("hungUp");

    QTest::newRow("readable fd") << false;
    // poll() returns for it too, with POLLHUP instead of POLLIN.
    QTest::newRow("hung-up fd") << true;
}

void GstHwBuffersTest::_testDmaBufProducerWaitNeedsSignalledFences()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    QFETCH(bool, hungUp);

    // What poll() reports for a dma-buf whose write fences signalled, and for one in error: a memfd is always
    // readable, the read end of a pipe without a writer only hangs up.
    int fd = -1;
    if (hungUp) {
        int ends[2] = {-1, -1};
        QCOMPARE(::pipe(ends), 0);
        ::close(ends[1]);
        fd = ends[0];
    } else {
        fd = GstTest::makeMemfd(QByteArray(4096, '\0'));
    }
    QVERIFY(fd >= 0);
    GstAllocator* const allocator = gst_dmabuf_allocator_new();
    GstBuffer* const buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, gst_dmabuf_allocator_alloc(allocator, fd, 4096));
    gst_object_unref(allocator);
    const auto bufferGuard = qScopeGuard([buffer] { gst_buffer_unref(buffer); });

    GstDmaBufVideoBuffer::resetCachedState();
    if (hungUp) {
        expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg,
                         QRegularExpression(QStringLiteral("producer fence wait failed")));
    }
    QCOMPARE(GstDmaBufVideoBuffer::waitForProducerWritesForTest(buffer), !hungUp);
    if (hungUp) {
        verifyExpectedLogMessage();
    }
#endif
}

void GstHwBuffersTest::_testDmaBufFdCheckRejectsOtherFds()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    // GStreamer's dmabuf allocator calls any fd a dma-buf, as the memfd-backed samples of these tests rely on.
    GstVideoInfo info;
    const GstTest::SamplePtr sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    GstMemory* const memory = gst_buffer_peek_memory(gst_sample_get_buffer(sample.get()), 0);
    QVERIFY(gst_is_dmabuf_memory(memory));

    QVERIFY(!GstDmaBufVideoBuffer::isDmaBufFd(gst_dmabuf_memory_get_fd(memory)));
    QVERIFY(!GstDmaBufVideoBuffer::isDmaBufFd(-1));
#endif
}

void GstHwBuffersTest::_testDmaBufVulkanImportNeedsProbedDevice_data()
{
    QTest::addColumn<int>("rhiBackend");
    QTest::addColumn<bool>("expectAccepted");

    // No Vulkan device was probed here, so nothing says what it would import.
    QTest::newRow("Vulkan") << int(QRhi::Vulkan) << false;
    QTest::newRow("OpenGL") << int(QRhi::OpenGLES2) << true;
}

void GstHwBuffersTest::_testDmaBufVulkanImportNeedsProbedDevice()
{
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    QSKIP("DMABuf import under Vulkan not compiled in this build");
#else
    QFETCH(int, rhiBackend);
    QFETCH(bool, expectAccepted);

    auto& snapshotBackend = QGCRhiCapture::deviceSnapshot().backend;
    const int previousBackend = snapshotBackend.load();
    const auto restoreBackend = qScopeGuard([&] { snapshotBackend.store(previousBackend); });
    snapshotBackend.store(rhiBackend);

    GstVideoInfo info;
    GstTest::SamplePtr sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    const GstDmaBufVideoBuffer frame(sample.get(), info,
                                     QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12), EGL_NO_DISPLAY);
    sample.reset();

    QCOMPARE(frame.validatePlaneHandles(), expectAccepted);
    if (rhiBackend == int(QRhi::Vulkan)) {
        QVERIFY2(!HwBuffers::dmaBufImportDisplay(), "the sink must not offer DMABuf to a device that can't import it");
    }
#endif
}

void GstHwBuffersTest::_testDmaBufVulkanRequestsDeviceExtensions_data()
{
    QTest::addColumn<QByteArray>("before");
    QTest::addColumn<QByteArray>("after");

    const QByteArray required =
        "VK_KHR_external_memory_fd;VK_EXT_external_memory_dma_buf;"
        "VK_EXT_image_drm_format_modifier";
    QTest::newRow("unset") << QByteArray() << required;
    // The user's stay, and one of ours they already named isn't repeated.
    QTest::newRow("set by the user") << QByteArray("VK_EXT_external_memory_dma_buf;VK_KHR_maintenance1")
                                     << QByteArray(
                                            "VK_EXT_external_memory_dma_buf;VK_KHR_maintenance1;"
                                            "VK_KHR_external_memory_fd;VK_EXT_image_drm_format_modifier");
}

void GstHwBuffersTest::_testDmaBufVulkanRequestsDeviceExtensions()
{
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    QSKIP("DMABuf import under Vulkan not compiled in this build");
#else
    QFETCH(QByteArray, before);
    QFETCH(QByteArray, after);

    // What QRhi reads when it creates a Vulkan device.
    constexpr char kVariable[] = "QT_VULKAN_DEVICE_EXTENSIONS";
    const TestFixtures::EnvVarFixture restore(kVariable);
    if (before.isEmpty()) {
        qunsetenv(kVariable);
    } else {
        qputenv(kVariable, before);
    }

    GstDmaBufVulkan::requestDeviceExtensions();
    QCOMPARE(qgetenv(kVariable), after);
#endif
}

void GstHwBuffersTest::_testDmaBufVulkanFdDupFailureIsNotARefusal()
{
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    QSKIP("DMABuf import under Vulkan not compiled in this build");
#else
    using GstDmaBufVulkan::ImportFailure;
    const auto resetGuard = qScopeGuard([] { GstDmaBufVideoBuffer::resetCachedState(); });
    const GstDmaBufVulkan::DeviceFns fns = standInVulkanDeviceFns();
    // No open descriptor has this number: fcntl() fails on it with EBADF, as it fails on any with EMFILE once the
    // process has none left.
    const GstQtDmaBufImport::Plane plane{.fd = (std::numeric_limits<int>::max)(), .pitch = 64};

    // For comparison, an image the driver refuses.
    GstDmaBufVideoBuffer::resetCachedState();
    s_standInVulkanDevice.restart();
    s_standInVulkanDevice.refusesImages = true;
    expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBufVulkan", QtWarningMsg,
                     QRegularExpression(QStringLiteral("vkCreateImage failed")));
    QVERIFY(GstDmaBufVulkan::importPlaneForTest(fns, plane) == ImportFailure::Rejected);
    verifyExpectedLogMessage();

    // Warned about once, with the errno, until the scene graph is recreated.
    GstDmaBufVideoBuffer::resetCachedState();
    s_standInVulkanDevice.restart();
    expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBufVulkan", QtWarningMsg,
                     QRegularExpression(QStringLiteral("dma-buf fd failed: errno %1 ").arg(EBADF)));
    QVERIFY(GstDmaBufVulkan::importPlaneForTest(fns, plane) == ImportFailure::FdDupFailed);
    verifyExpectedLogMessage();
    QVERIFY(GstDmaBufVulkan::importPlaneForTest(fns, plane) == ImportFailure::FdDupFailed);

    // And counted apart from the imports a driver refused.
    GstVideoInfo info;
    GstTest::SamplePtr sample = makeMemfdDmaBufSample(GST_VIDEO_FORMAT_NV12, info);
    QVERIFY(sample);
    const GstDmaBufVideoBuffer frame(sample.get(), info,
                                     QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_NV12), EGL_NO_DISPLAY);
    sample.reset();
    const GstHwPathTelemetry::Snapshot before = GstHwPathTelemetry::snapshot();
    frame.noteImportOutcomeForTest(GstDmaBufVideoBuffer::ImportOutcome::FdDupFailed);
    const QString stats = HwBuffers::extraPathStatsSince(before);
    QVERIFY2(stats.contains(QStringLiteral("DMABuf-fallback[fd-dup-failed=1]")), qPrintable(stats));
#endif
}

void GstHwBuffersTest::_testDmaBufVulkanRetiredBufferNeedsNoLaterFrame_data()
{
    QTest::addColumn<bool>("rhiDestroyedFirst");

    QTest::newRow("the GPU finishes while the QRhi lives") << false;
    QTest::newRow("the QRhi is destroyed before the GPU finishes") << true;
}

void GstHwBuffersTest::_testDmaBufVulkanRetiredBufferNeedsNoLaterFrame()
{
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    QSKIP("DMABuf import under Vulkan not compiled in this build");
#else
    QFETCH(bool, rhiDestroyedFirst);

    std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);
    StandInVulkanDevice& device = s_standInVulkanDevice;
    device.restart();
    // Lets the buffer's waiter go when the test fails before the GPU is done.
    const auto gpuGuard = qScopeGuard([&device] { device.finishGpuWork(); });
    GstBuffer* const buffer = gst_buffer_new();
    const auto bufferGuard = qScopeGuard([buffer] { gst_buffer_unref(buffer); });
    const auto bufferRefs = [buffer] { return GST_MINI_OBJECT_REFCOUNT_VALUE(buffer); };

    // Qt destroys a frame's bundle while the GPU still samples its images: the decoder must not get the buffer yet.
    GstDmaBufVulkan::deferredReleaseForTest(*rhi, standInVulkanDeviceFns(), buffer)();
    QCOMPARE(bufferRefs(), 2);

    if (rhiDestroyedFirst) {
        // The teardown waits for the GPU, which finishes meanwhile.
        std::thread gpu;
        rhi->addCleanupCallback([&gpu, &device](QRhi*) { gpu = std::thread([&device] { device.finishGpuWork(); }); });
        rhi.reset();
        gpu.join();
        QCOMPARE(bufferRefs(), 1);
    } else {
        // The GPU finishes. The stream has ended: no other frame is imported or released, and the QRhi lives on.
        device.finishGpuWork();
        QTRY_COMPARE_WITH_TIMEOUT(bufferRefs(), 1, TestTimeout::mediumMs());
        QCOMPARE(device.imagesDestroyed.load(), 0);
        rhi.reset();
    }
    // The images, their memory and the fence are the QRhi's thread's to destroy, at its teardown at the latest.
    QCOMPARE(device.imagesDestroyed.load(), 1);
    QCOMPARE(device.memoriesFreed.load(), 1);
    QCOMPARE(device.fencesDestroyed.load(), 1);
    QCOMPARE(device.imageDestroyedOn.load(), QThread::currentThreadId());
#endif
}

void GstHwBuffersTest::_testVulkanOwnedFrameTexturesReleaseOnDestroy()
{
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    QSKIP("DMABuf->Vulkan frame bundles not compiled in this build");
#else
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);

    // Any non-null handle: the Null QRhi wraps it without touching Vulkan, while a null one is left unwrapped.
    const auto image = VkImage(quintptr(1));
    auto bundle = std::make_unique<GstVulkanOwnedFrameTextures>(
        rhi.get(), QSize(64, 64), QVideoFrameFormat::Format_RGBA8888, std::span<const VkImage>(&image, 1));
    QVERIFY(bundle->allPlanesValid());
    int releases = 0;
    bundle->adoptVulkanResources([&releases] { ++releases; });
    QRhiTexture* wrapped = bundle->texture(0);

    // Qt keeps sampling the current bundle across window frames; frame end must not release the imported image.
    for (int frame = 0; frame < 3; ++frame) {
        bundle->onFrameEndInvoked();
    }
    QCOMPARE(bundle->texture(0), wrapped);
    QCOMPARE(releases, 0);

    bundle.reset();
    QCOMPARE(releases, 1);
#endif
}
