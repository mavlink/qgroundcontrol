#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <atomic>
#include <cstring>
#include <gst/gst.h>
#include <iterator>
#include <memory>
#include <vector>

#include "Fixtures/RAIIFixtures.h"
#include "Fact.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstVideoReceiver.h"

#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <gst/video/gstvideometa.h>

#include "GStreamer.h"
#include "GStreamerFrameMap.h"
#include "CpuVideoFramePool.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBufferFactory.h"
#include "GstSourceFactory.h"
#include "QGCQVideoSinkController.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "gstqgc/GstQgcAllocation.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcVideoFormats.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include <gst/gl/gl.h>

#include "GstGlContextBridge.h"
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include "GstD3D11ContextBridge.h"
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include "GstD3D12ContextBridge.h"
#endif
#include "GstContextBridgeRegistry.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"
#include "gstqgc/gstqgcvideosinkbin.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCRhiCapture.h"
#endif
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <rhi/qrhi.h>
#include <string_view>

void GStreamerTest::_testBridgeDispatcherFanout()
{
#if !defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_HAS_GST_D3D11_GPU_PATH) && \
    !defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QSKIP("No GPU bridge compiled in this build");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement* dummy = gst_element_factory_make("identity", nullptr);
    QVERIFY(dummy);
    GstMessage* unrelated = gst_message_new_need_context(GST_OBJECT(dummy), "totally.unrelated.context");
    QVERIFY(unrelated);
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    QCOMPARE(GstGlContextBridge::handleSyncMessage(unrelated), GST_BUS_PASS);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QCOMPARE(GstD3D11ContextBridge::handleSyncMessage(unrelated), GST_BUS_PASS);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QCOMPARE(GstD3D12ContextBridge::handleSyncMessage(unrelated), GST_BUS_PASS);
#endif
    gst_message_unref(unrelated);

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    GstMessage* glReq = gst_message_new_need_context(GST_OBJECT(dummy), GST_GL_DISPLAY_CONTEXT_TYPE);
    QVERIFY(glReq);
    const GstBusSyncReply r = GstGlContextBridge::handleSyncMessage(glReq);
    // Either PASS (couldn't prime — expected in CI without GL) or DROP (primed
    // and consumed). Both are valid; the contract is "never crash".
    QVERIFY(r == GST_BUS_PASS || r == GST_BUS_DROP);
    if (r == GST_BUS_PASS)
        gst_message_unref(glReq);
#endif

    gst_object_unref(dummy);
#endif
}

void GStreamerTest::_testContextBridgeRegistry()
{
#if !defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_HAS_GST_D3D11_GPU_PATH) && \
    !defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QSKIP("No GPU context bridge compiled in this build");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstContextBridgeRegistry::clearForTest();

    static bool s_invoked = false;
    s_invoked = false;

    GstContextBridgeRegistry::registerBridgeHandler([](GstMessage* msg) -> GstBusSyncReply {
        const gchar* type = nullptr;
        gst_message_parse_context_type(msg, &type);
        if (type && std::string_view(type) == "test-bridge-only") {
            s_invoked = true;
            return GST_BUS_DROP;
        }
        return GST_BUS_PASS;
    });

    GstElement* dummy = gst_element_factory_make("identity", nullptr);
    QVERIFY(dummy);

    GstMessage* hit = gst_message_new_need_context(GST_OBJECT(dummy), "test-bridge-only");
    QVERIFY(hit);
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(hit), GST_BUS_DROP);
    QVERIFY(s_invoked);
    gst_message_unref(hit);

    s_invoked = false;
    GstMessage* miss = gst_message_new_need_context(GST_OBJECT(dummy), "other-context-type");
    QVERIFY(miss);
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(miss), GST_BUS_PASS);
    QVERIFY(!s_invoked);
    gst_message_unref(miss);

    // Reset-callback round-trip: registerResetCallback + resetAllBridges must invoke every cb.
    GstContextBridgeRegistry::clearForTest();
    static int s_resetCount = 0;
    s_resetCount = 0;
    GstContextBridgeRegistry::registerResetCallback([]() { ++s_resetCount; });
    GstContextBridgeRegistry::registerResetCallback([]() { s_resetCount += 10; });
    GstContextBridgeRegistry::resetAllBridges();
    QCOMPARE(s_resetCount, 11);
    // clearForTest must invoke pending reset callbacks before zeroing the slots so cached
    // bridge state can't leak across test cases. Drop the prior round's callbacks first so we
    // measure exactly the new callback's invocation count, not 1+10+1=12 from leftovers.
    GstContextBridgeRegistry::clearForTest();
    s_resetCount = 0;
    GstContextBridgeRegistry::registerResetCallback([]() { ++s_resetCount; });
    GstContextBridgeRegistry::clearForTest();
    QCOMPARE(s_resetCount, 1);
    GstContextBridgeRegistry::resetAllBridges();  // post-clear: no callbacks → no-op
    QCOMPARE(s_resetCount, 1);

    // Coexistence: two bridges with different context types must not consume each other's messages.
    GstContextBridgeRegistry::clearForTest();
    static bool s_aHit = false;
    static bool s_bHit = false;
    s_aHit = s_bHit = false;
    GstContextBridgeRegistry::registerBridgeHandler([](GstMessage* m) -> GstBusSyncReply {
        const gchar* t = nullptr;
        gst_message_parse_context_type(m, &t);
        if (t && std::string_view(t) == "type-A") {
            s_aHit = true;
            return GST_BUS_DROP;
        }
        return GST_BUS_PASS;
    });
    GstContextBridgeRegistry::registerBridgeHandler([](GstMessage* m) -> GstBusSyncReply {
        const gchar* t = nullptr;
        gst_message_parse_context_type(m, &t);
        if (t && std::string_view(t) == "type-B") {
            s_bHit = true;
            return GST_BUS_DROP;
        }
        return GST_BUS_PASS;
    });
    GstMessage* msgA = gst_message_new_need_context(GST_OBJECT(dummy), "type-A");
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(msgA), GST_BUS_DROP);
    QVERIFY(s_aHit);
    QVERIFY(!s_bHit);
    gst_message_unref(msgA);
    GstMessage* msgB = gst_message_new_need_context(GST_OBJECT(dummy), "type-B");
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(msgB), GST_BUS_DROP);
    QVERIFY(s_bHit);
    gst_message_unref(msgB);

    gst_object_unref(dummy);
#endif
}

void GStreamerTest::_testHwBufferLifecycleResetsNativeCaches()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("HwBuffers not compiled in this build (no GPU path enabled)");
#else
    GstContextBridgeRegistry::clearForTest();
    auto guard = qScopeGuard([] { GstContextBridgeRegistry::clearForTest(); });

    static int s_resetCount = 0;
    s_resetCount = 0;
    GstContextBridgeRegistry::registerResetCallback([]() { ++s_resetCount; });

    HwBuffers::resetCachedGpuResources();
    QCOMPARE(s_resetCount, 1);

    GstElement* dummy = gst_element_factory_make("identity", nullptr);
    QVERIFY(dummy);
    GstMessage* ordinaryError = gst_message_new_error(
        GST_OBJECT(dummy), g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_FAILED, "ordinary stream error"),
        "debug");
    QVERIFY(ordinaryError);
    HwBuffers::dispatchBusMessage(ordinaryError);
    QCOMPARE(s_resetCount, 1);
    gst_message_unref(ordinaryError);

    GstMessage* deviceLostError = gst_message_new_error(
        GST_OBJECT(dummy), g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_FAILED, "DEVICE_REMOVED"),
        "debug");
    QVERIFY(deviceLostError);
    HwBuffers::dispatchBusMessage(deviceLostError);
    QCOMPARE(s_resetCount, 2);
    gst_message_unref(deviceLostError);
    gst_object_unref(dummy);

    HwBuffers::onPipelineRestart();
    QCOMPARE(s_resetCount, 3);

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) ||    \
    defined(QGC_HAS_GST_D3D12_GPU_PATH) || defined(QGC_HAS_GST_IOSURFACE_GPU_PATH) || \
    defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    GstContextBridgeRegistry::clearForTest();
    static int s_cacheReset = 0;
    s_cacheReset = 0;
    GstContextBridgeRegistry::registerCacheReset([]() { ++s_cacheReset; });
    GstContextBridgeRegistry::resetAllCaches();
    QCOMPARE(s_cacheReset, 1);
    s_cacheReset = 0;
    GstContextBridgeRegistry::resetAllBridges();
    QCOMPARE(s_cacheReset, 0);
    GstContextBridgeRegistry::clearForTest();
#endif
#endif
}

void GStreamerTest::_testHwBufferFactoryDispatchSystemMemory()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("HwVideoBuffer factory not compiled in this build (no GPU path enabled)");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GError* err = nullptr;
    // videotestsrc produces system memory; no GPU path should match.
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=1 ! "
        "video/x-raw,format=BGRA,width=64,height=64,framerate=30/1 ! "
        "fakesink name=sink enable-last-sample=true sync=false",
        &err);
    if (err) {
        g_clear_error(&err);
    }
    QVERIFY2(pipeline, "Pipeline construction failed");

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    gst_object_unref(bus);
    if (msg)
        gst_message_unref(msg);

    GstElement* fakesink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(fakesink);
    GstSample* sample = nullptr;
    g_object_get(fakesink, "last-sample", &sample, nullptr);
    gst_object_unref(fakesink);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    if (!sample)
        QSKIP("No sample produced (fakesink enable-last-sample may be off)");

    GstVideoInfo info;
    GstCaps* caps = gst_sample_get_caps(sample);
    QVERIFY(gst_video_info_from_caps(&info, caps));

    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_BGRA8888);
    HwVideoBufferPath path = HwVideoBufferPath::None;

    HwVideoBufferContext ctxGpu;
    ctxGpu.gpuEnabled = true;
    auto buf = makeHwVideoBuffer(sample, info, fmt, ctxGpu, path);
    QVERIFY(!buf);
    QCOMPARE(path, HwVideoBufferPath::None);

    HwVideoBufferContext ctxCpu;
    ctxCpu.gpuEnabled = false;
    auto buf2 = makeHwVideoBuffer(sample, info, fmt, ctxCpu, path);
    QVERIFY(!buf2);

    gst_sample_unref(sample);
#endif
}

void GStreamerTest::_testQGCRhiCaptureCacheLifecycle()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("QGCRhiCapture not compiled in this build (no GPU path enabled)");
#else
    // cachedRhi() returns nullptr before any window has been connected.
    QVERIFY(!QGCRhiCapture::cachedRhi());

    // Create an offscreen window and connect it. The scene graph is never
    // initialized here so cachedRhi() remains nullptr.
    auto* window = new QQuickWindow();
    QGCRhiCapture::connectWindow(window);
    QVERIFY(!QGCRhiCapture::cachedRhi());

    // Destroying the window must clear the cache (no dangling QRhi*).
    delete window;
    QVERIFY(!QGCRhiCapture::cachedRhi());
#endif
}

namespace {

#if defined(QGC_HAS_ANY_GPU_PATH)
// Concrete subclass for unit-testing GstHwVideoBuffer's protected/base behavior.
class TestableHwVideoBuffer : public GstHwVideoBuffer
{
public:
    TestableHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat fmt)
        : GstHwVideoBuffer(QVideoFrame::NoHandle, sample, info, std::move(fmt))
    {}

    MapData map(QVideoFrame::MapMode) override { return {}; }

    QVideoFrameTexturesUPtr mapTextures(QRhi&, QVideoFrameTexturesUPtr&) override { return {}; }
};

class TestFrameTextures : public GstHwFrameTexturesBase
{
public:
    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::GlMemory; }
};

class ForeignFrameTextures : public QVideoFrameTextures
{
public:
    QRhiTexture* texture(uint) const override { return nullptr; }
};
#endif

// Minimal GstSample factory: NV12 buffer at given size, optionally with a crop meta.
GstSample* makeNv12Sample(int width, int height, GstVideoInfo* outInfo, bool addCrop, int cx, int cy, int cw, int ch)
{
    GstVideoInfo info;
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_NV12, width, height);
    GstCaps* caps = gst_video_info_to_caps(&info);
    GstBuffer* buf = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    if (addCrop) {
        GstVideoCropMeta* crop = gst_buffer_add_video_crop_meta(buf);
        crop->x = cx;
        crop->y = cy;
        crop->width = cw;
        crop->height = ch;
    }
    GstSample* sample = gst_sample_new(buf, caps, nullptr, nullptr);
    gst_buffer_unref(buf);
    gst_caps_unref(caps);
    if (outInfo)
        *outInfo = info;
    return sample;
}

}  // namespace

void GStreamerTest::_testHwFrameTexturesRejectsForeignOldTextures()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("GstHwFrameTexturesBase not compiled in this build (no GPU path enabled)");
#else
    QVideoFrameTexturesUPtr foreign = std::make_unique<ForeignFrameTextures>();
    QVERIFY(!GstHwFrameTexturesBase::reusableBundle<TestFrameTextures>(foreign, HwVideoBufferPath::GlMemory));
    QVERIFY(foreign);

    QVideoFrameTexturesUPtr own = std::make_unique<TestFrameTextures>();
    QVERIFY(GstHwFrameTexturesBase::reusableBundle<TestFrameTextures>(own, HwVideoBufferPath::GlMemory));
    QVERIFY(own);
#endif
}

void GStreamerTest::_testHwBufferCropMatrixIdentityWithoutMeta()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("GstHwVideoBuffer not compiled in this build (no GPU path enabled)");
#else
    GstVideoInfo info;
    GstSample* sample = makeNv12Sample(320, 240, &info, /*addCrop=*/false, 0, 0, 0, 0);
    QVERIFY(sample);
    QVideoFrameFormat fmt(QSize(320, 240), QVideoFrameFormat::Format_NV12);
    TestableHwVideoBuffer hw(sample, info, fmt);
    // Default externalTextureMatrix from QHwVideoBuffer is identity; we don't override it
    // for crop because Qt only consults externalTextureMatrix for Format_SamplerExternalOES.
    QCOMPARE(hw.externalTextureMatrix(), QMatrix4x4());
    gst_sample_unref(sample);
#endif
}

void GStreamerTest::_testHwBufferCropMatrixFromVideoCropMeta()
{
    // Regression: GstVideoCropMeta is propagated via QVideoFrameFormat::viewport() in
    // GStreamerFrameMap::applyCropMeta, NOT via externalTextureMatrix. Verify the format
    // path round-trips a crop rect into viewport().
    GstVideoInfo info;
    GstSample* sample = makeNv12Sample(400, 200, &info, /*addCrop=*/true,
                                       /*cx=*/100, /*cy=*/50, /*cw=*/200, /*ch=*/100);
    QVERIFY(sample);
    GstBuffer* buf = gst_sample_get_buffer(sample);

    QVideoFrameFormat in(QSize(400, 200), QVideoFrameFormat::Format_NV12);
    QVideoFrameFormat out = applyCropMeta(in, buf);
    QCOMPARE(out.viewport(), QRect(100, 50, 200, 100));
    QCOMPARE(out.frameSize(), QSize(400, 200));  // unchanged

    gst_sample_unref(sample);
}

void GStreamerTest::_testTelemetryMapDurationEwma()
{
    const HwVideoBufferPath path = HwVideoBufferPath::Vulkan;

    GstHwPathTelemetry::recordMapDuration(path, 80000);
    const quint64 seeded = GstHwPathTelemetry::peekMapDurationUsEwma(path);
    QVERIFY(seeded > 0);

    GstHwPathTelemetry::recordMapDuration(path, -5);
    QCOMPARE(GstHwPathTelemetry::peekMapDurationUsEwma(path), seeded);

    GstHwPathTelemetry::recordMapDuration(path, 160000);
    const quint64 expected = seeded - (seeded >> 3) + (160u >> 3);
    QCOMPARE(GstHwPathTelemetry::peekMapDurationUsEwma(path), expected);
}

void GStreamerTest::_testTelemetryFallbackReasonMatrix()
{
    using GstHwPathTelemetry::HwFallbackReason;

    (void) GstHwPathTelemetry::takeFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch);
    (void) GstHwPathTelemetry::takeFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::FenceTimeout);
    (void) GstHwPathTelemetry::takeFallbackReason(HwVideoBufferPath::D3D12, HwFallbackReason::EglBadMatch);

    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch);
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch);

    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch),
             quint64(2));
    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::FenceTimeout),
             quint64(0));
    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::D3D12, HwFallbackReason::EglBadMatch),
             quint64(0));

    QCOMPARE(GstHwPathTelemetry::takeFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch),
             quint64(2));
    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch),
             quint64(0));
}

void GStreamerTest::_testTelemetrySyncWaitSplit()
{
    const HwVideoBufferPath path = HwVideoBufferPath::IOSurface;

    quint64 drainGpu = 0;
    (void) GstHwPathTelemetry::takeSyncWaitCounts(path, drainGpu);
    (void) GstHwPathTelemetry::takeImageCacheHits(path);
    (void) GstHwPathTelemetry::takeImageCacheMisses(path);

    GstHwPathTelemetry::recordSyncWait(path, false);
    GstHwPathTelemetry::recordSyncWait(path, false);
    GstHwPathTelemetry::recordSyncWait(path, true);
    GstHwPathTelemetry::recordImageCacheHit(path);
    GstHwPathTelemetry::recordImageCacheHit(path);
    GstHwPathTelemetry::recordImageCacheMiss(path);

    quint64 gpuWaits = 0;
    const quint64 cpuWaits = GstHwPathTelemetry::takeSyncWaitCounts(path, gpuWaits);
    QCOMPARE(cpuWaits, quint64(2));
    QCOMPARE(gpuWaits, quint64(1));
    QCOMPARE(GstHwPathTelemetry::takeImageCacheHits(path), quint64(2));
    QCOMPARE(GstHwPathTelemetry::takeImageCacheMisses(path), quint64(1));

    quint64 gpuWaits2 = 99;
    QCOMPARE(GstHwPathTelemetry::takeSyncWaitCounts(path, gpuWaits2), quint64(0));
    QCOMPARE(gpuWaits2, quint64(0));
    QCOMPARE(GstHwPathTelemetry::takeImageCacheHits(path), quint64(0));
    QCOMPARE(GstHwPathTelemetry::takeImageCacheMisses(path), quint64(0));
}

void GStreamerTest::_testTelemetryPathStatsFailuresAreNotDelivered()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    constexpr HwVideoBufferPath path = HwVideoBufferPath::DmaBuf;
#elif defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    constexpr HwVideoBufferPath path = HwVideoBufferPath::GlMemory;
#elif defined(QGC_HAS_GST_D3D11_GPU_PATH)
    constexpr HwVideoBufferPath path = HwVideoBufferPath::D3D11;
#elif defined(QGC_HAS_GST_D3D12_GPU_PATH)
    constexpr HwVideoBufferPath path = HwVideoBufferPath::D3D12;
#elif defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    constexpr HwVideoBufferPath path = HwVideoBufferPath::IOSurface;
#elif defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    constexpr HwVideoBufferPath path = HwVideoBufferPath::AHardwareBuffer;
#else
    constexpr HwVideoBufferPath path = HwVideoBufferPath::Vulkan;
#endif

    (void) GstHwPathTelemetry::takeDeliveredCount(path);
    (void) GstHwPathTelemetry::takeMapFailureCount(path);

    GstHwPathTelemetry::recordMapFailure(path);

    const HwBuffers::PathStats stats = HwBuffers::formatPathStats(/*reset=*/true);
    QVERIFY2(stats.line.contains(QStringLiteral("failures:1")),
             qPrintable(QStringLiteral("Expected failure count in stats line, got:%1").arg(stats.line)));
    QCOMPARE(stats.totalDelivered, quint64(0));
    QCOMPARE(GstHwPathTelemetry::peekMapFailureCount(path), quint64(0));
#endif
}

void GStreamerTest::_testTelemetryDmaBufExtraStatsDrain()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    GstHwPathTelemetry::recordFenceTimeout(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordMmapBarrierHit(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordExplicitFenceWait(HwVideoBufferPath::DmaBuf);

    const QString first = HwBuffers::takeExtraPathStats();
    QVERIFY(first.contains(QStringLiteral("DMABuf-fence-timeouts:")));
    QVERIFY(first.contains(QStringLiteral("DMABuf-mmap-barriers:")));
    QVERIFY(first.contains(QStringLiteral("DMABuf-explicit-fence-waits:")));

    const QString second = HwBuffers::takeExtraPathStats();
    QVERIFY(second.contains(QStringLiteral("DMABuf-fence-timeouts:0")));
    QVERIFY(second.contains(QStringLiteral("DMABuf-mmap-barriers:0")));
    QVERIFY(second.contains(QStringLiteral("DMABuf-explicit-fence-waits:0")));
#endif
}

#endif
