#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <cstring>
#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <iterator>
#include <memory>
#include <vector>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "CpuVideoFramePool.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBufferFactory.h"
#include "GstQgcFrameMap.h"
#include "GstSourceFactory.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "QGCQVideoSinkController.h"
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
#include "GstContextBridgeBase.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwPathRegistry.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"
#include "gstqgc/gstqgcvideosinkbin.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "GstHwImportPreflight.h"
#include "QGCRhiCapture.h"
#endif
#include <rhi/qrhi.h>
#include <string_view>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>

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
    QCOMPARE(GstGlContextBridge::instance().handleSyncMessage(unrelated), GST_BUS_PASS);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QCOMPARE(GstD3D11ContextBridge::instance().handleSyncMessage(unrelated), GST_BUS_PASS);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QCOMPARE(GstD3D12ContextBridge::instance().handleSyncMessage(unrelated), GST_BUS_PASS);
#endif
    gst_message_unref(unrelated);

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    GstMessage* glReq = gst_message_new_need_context(GST_OBJECT(dummy), GST_GL_DISPLAY_CONTEXT_TYPE);
    QVERIFY(glReq);
    const GstBusSyncReply r = GstGlContextBridge::instance().handleSyncMessage(glReq);
    // Either PASS (couldn't prime — expected in CI without GL) or DROP (primed
    // and consumed). Both are valid; the contract is "never crash".
    QVERIFY(r == GST_BUS_PASS || r == GST_BUS_DROP);
    if (r == GST_BUS_PASS)
        gst_message_unref(glReq);
#endif

    gst_object_unref(dummy);
#endif
}

#if defined(QGC_HAS_ANY_GPU_PATH)
namespace {

Q_LOGGING_CATEGORY(TestContextBridgeLog, "Video.GStreamer.Test.ContextBridge")

/// Bridge handing out a plain GstObject under one context type; counts primes and resets.
class TestContextBridge final : public GstContextBridgeBase
{
public:
    explicit TestContextBridge(std::span<const char* const> contextTypes)
        : GstContextBridgeBase("Test", contextTypes, &TestContextBridgeLog)
    {}

    /// Unprimed, with zeroed counters.
    void restart()
    {
        reset();
        primes = 0;
        clears = 0;
    }

    int primes = 0;
    int clears = 0;

private:
    bool primeLocked() override
    {
        ++primes;
        _object = GST_OBJECT(gst_object_ref_sink(gst_element_factory_make("identity", nullptr)));
        return _object != nullptr;
    }

    GstObject* refObject(const char*) override { return _object ? GST_OBJECT(gst_object_ref(_object)) : nullptr; }

    GstContext* buildContext(const char* contextType, GstObject* object) override
    {
        GstContext* context = gst_context_new(contextType, TRUE);
        gst_structure_set(gst_context_writable_structure(context), "object", GST_TYPE_OBJECT, object, nullptr);
        return context;
    }

    void clearLocked() override
    {
        ++clears;
        gst_clear_object(&_object);
    }

    GstObject* _object = nullptr;
};

constexpr const char* kTestContextTypeA[] = {"qgc.test.context-a"};
constexpr const char* kTestContextTypeB[] = {"qgc.test.context-b"};

TestContextBridge& testBridgeA()
{
    static TestContextBridge bridge(kTestContextTypeA);
    return bridge;
}

TestContextBridge& testBridgeB()
{
    static TestContextBridge bridge(kTestContextTypeB);
    return bridge;
}

int s_testCacheResets = 0;

const HwPathDescriptor kTestBridgePathA = {
    .path = HwVideoBufferPath::GlMemory,
    .matches = [](GstMemory*, const HwVideoBufferContext&) { return false; },
    .resetCaches = [] { ++s_testCacheResets; },
    .bridge = []() -> GstContextBridgeBase& { return testBridgeA(); },
};

const HwPathDescriptor kTestBridgePathB = {
    .path = HwVideoBufferPath::Vulkan,
    .matches = [](GstMemory*, const HwVideoBufferContext&) { return false; },
    .bridge = []() -> GstContextBridgeBase& { return testBridgeB(); },
};

}  // namespace
#endif

void GStreamerTest::_testContextBridgeRegistry()
{
#if !defined(QGC_HAS_ANY_GPU_PATH) || !defined(QGC_GST_BUILD_TESTING)
    QSKIP("No GPU path compiled in this build");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    testBridgeA().restart();
    testBridgeB().restart();
    const auto bridgeGuard = qScopeGuard([] {
        testBridgeA().restart();
        testBridgeB().restart();
    });
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestBridgePathA, &kTestBridgePathB});

    GstElement* element = gst_element_factory_make("identity", nullptr);
    QVERIFY(element);
    const auto elementGuard = qScopeGuard([element] { gst_object_unref(element); });

    // NEED_CONTEXT reaches the bridge owning the context type, and only that bridge primes.
    GstMessage* needA = gst_message_new_need_context(GST_OBJECT(element), kTestContextTypeA[0]);
    QCOMPARE(HwBuffers::onBusSyncMessage(needA), GST_BUS_DROP);
    GstContext* deliveredA = gst_element_get_context(element, kTestContextTypeA[0]);
    QVERIFY2(deliveredA, "Bridge A's context was not set on the requesting element");
    gst_context_unref(deliveredA);
    QCOMPARE(testBridgeA().primes, 1);
    QCOMPARE(testBridgeB().primes, 0);

    GstMessage* unknown = gst_message_new_need_context(GST_OBJECT(element), "qgc.test.context-unknown");
    QCOMPARE(HwBuffers::onBusSyncMessage(unknown), GST_BUS_PASS);
    gst_message_unref(unknown);

    // A context query is answered through the same bridges; a primed bridge isn't primed again.
    GstQuery* queryB = gst_query_new_context(kTestContextTypeB[0]);
    QVERIFY(HwBuffers::answerSinkBinContextQuery(queryB));
    GstContext* answeredB = nullptr;
    gst_query_parse_context(queryB, &answeredB);
    QVERIFY(answeredB);
    QCOMPARE(QByteArray(gst_context_get_context_type(answeredB)), QByteArray(kTestContextTypeB[0]));
    gst_query_unref(queryB);

    GstQuery* queryA = gst_query_new_context(kTestContextTypeA[0]);
    QVERIFY(HwBuffers::answerSinkBinContextQuery(queryA));
    gst_query_unref(queryA);
    QCOMPARE(testBridgeA().primes, 1);
    QCOMPARE(testBridgeB().primes, 1);

    GstQuery* unknownQuery = gst_query_new_context("qgc.test.context-unknown");
    QVERIFY(!HwBuffers::answerSinkBinContextQuery(unknownQuery));
    gst_query_unref(unknownQuery);
#endif
}

void GStreamerTest::_testHwBufferLifecycleResetsNativeCaches()
{
#if !defined(QGC_HAS_ANY_GPU_PATH) || !defined(QGC_GST_BUILD_TESTING)
    QSKIP("HwBuffers not compiled in this build (no GPU path enabled)");
#else
    testBridgeA().restart();
    const auto bridgeGuard = qScopeGuard([] { testBridgeA().restart(); });
    s_testCacheResets = 0;
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestBridgePathA});

    QVERIFY(testBridgeA().prime());
    HwBuffers::resetCachedGpuResources();
    QCOMPARE(testBridgeA().clears, 1);
    QCOMPARE(s_testCacheResets, 1);

    GstElement* dummy = gst_element_factory_make("identity", nullptr);
    QVERIFY(dummy);
    GstMessage* ordinaryError = gst_message_new_error(
        GST_OBJECT(dummy), g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_FAILED, "ordinary stream error"),
        "debug");
    QVERIFY(ordinaryError);
    HwBuffers::dispatchBusMessage(ordinaryError);
    QCOMPARE(testBridgeA().clears, 1);
    QCOMPARE(s_testCacheResets, 1);
    gst_message_unref(ordinaryError);

    GstMessage* deviceLostError = gst_message_new_error(
        GST_OBJECT(dummy), g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_FAILED, "DEVICE_REMOVED"),
        "debug");
    QVERIFY(deviceLostError);
    HwBuffers::dispatchBusMessage(deviceLostError);
    QCOMPARE(testBridgeA().clears, 2);
    QCOMPARE(s_testCacheResets, 2);
    gst_message_unref(deviceLostError);
    gst_object_unref(dummy);

    // A (re)starting pipeline must leave shared bridge objects and caches alone: another stream may be using them.
    QVERIFY(testBridgeA().prime());
    HwBuffers::onPipelineRestart();
    QCOMPARE(testBridgeA().clears, 2);
    QCOMPARE(s_testCacheResets, 2);
    QVERIFY(testBridgeA().prime());
    QCOMPARE(testBridgeA().primes, 2);
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

void GStreamerTest::_testPreflightVerdictsDroppedWithRhi()
{
#if !defined(QGC_HAS_ANY_GPU_PATH) || !defined(QGC_GST_BUILD_TESTING)
    QSKIP("Import preflight not compiled in this build (no GPU path enabled)");
#else
    QRhiNullInitParams nullParams;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, &nullParams));
    QVERIFY(rhi);
    const QRhi* const address = rhi.get();
    QVERIFY(GstHwImportPreflight::preflightOrRecord(rhi.get(), HwVideoBufferPath::DmaBuf,
                                                    QVideoFrameFormat::Format_NV12, QSize(64, 64)));
    QCOMPARE(GstHwImportPreflight::memoizedVerdictCountForTest(address), std::size_t(1));

    // The next QRhi may be allocated at the same address; it must not inherit this one's verdicts.
    rhi.reset();
    QCOMPARE(GstHwImportPreflight::memoizedVerdictCountForTest(address), std::size_t(0));
#endif
}

void GStreamerTest::_testHwBufferEnvConfigParsing_data()
{
    QTest::addColumn<QByteArray>("value");  // null: variable unset
    QTest::addColumn<bool>("expectedDefaultOff");
    QTest::addColumn<bool>("expectedDefaultOn");

    QTest::newRow("unset") << QByteArray() << false << true;
    QTest::newRow("empty") << QByteArray("") << false << true;
    QTest::newRow("zero") << QByteArray("0") << false << false;
    QTest::newRow("false-padded-mixed-case") << QByteArray(" False ") << false << false;
    QTest::newRow("off") << QByteArray("OFF") << false << false;
    QTest::newRow("no") << QByteArray("no") << false << false;
    QTest::newRow("one") << QByteArray("1") << true << true;
    QTest::newRow("other-word") << QByteArray("force") << true << true;
}

void GStreamerTest::_testHwBufferEnvConfigParsing()
{
    QFETCH(QByteArray, value);
    QFETCH(bool, expectedDefaultOff);
    QFETCH(bool, expectedDefaultOn);

    // QGC_GST_DMABUF_CACHE defaults off, QGC_GST_DMABUF_SINGLE_EGLIMAGE defaults on.
    const TestFixtures::EnvVarFixture defaultOffGuard("QGC_GST_DMABUF_CACHE");
    const TestFixtures::EnvVarFixture defaultOnGuard("QGC_GST_DMABUF_SINGLE_EGLIMAGE");
    for (const char* const name : {"QGC_GST_DMABUF_CACHE", "QGC_GST_DMABUF_SINGLE_EGLIMAGE"}) {
        if (value.isNull()) {
            qunsetenv(name);
        } else {
            qputenv(name, value);
        }
    }

    const HwBuffers::HwBufferEnvConfig config = HwBuffers::readHwBufferEnvConfig();
    QCOMPARE(config.dmaBufCache, expectedDefaultOff);
    QCOMPARE(config.dmaBufSingleEglImage, expectedDefaultOn);
}

void GStreamerTest::_testCpuLayoutSizesPlanesByComponent_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QList<int>>("planeHeights");

    QTest::newRow("NV12") << int(GST_VIDEO_FORMAT_NV12) << QList<int>{48, 24};
    QTest::newRow("YV12") << int(GST_VIDEO_FORMAT_YV12) << QList<int>{48, 24, 24};
    // Third plane holds component 3 (full-height alpha); component 2 is the half-height V in plane 1.
    QTest::newRow("AV12") << int(GST_VIDEO_FORMAT_AV12) << QList<int>{48, 24, 48};
}

void GStreamerTest::_testCpuLayoutSizesPlanesByComponent()
{
    QFETCH(int, format);
    QFETCH(QList<int>, planeHeights);

    GstVideoInfo info;
    QVERIFY(gst_video_info_set_format(&info, static_cast<GstVideoFormat>(format), 64, 48));
    const CpuVideoFramePool::PlaneLayout layout = CpuVideoFramePool::computeLayout(info);
    QCOMPARE(layout.planeCount, planeHeights.size());
    for (int plane = 0; plane < layout.planeCount; ++plane) {
        QCOMPARE(layout.height[plane], planeHeights[plane]);
    }
}

void GStreamerTest::_testCpuCopyOddDimensions_data()
{
    QTest::addColumn<int>("gstFormat");
    QTest::addColumn<int>("qtFormat");
    QTest::addColumn<QList<int>>("planeRows");
    QTest::addColumn<QList<int>>("planeRowBytes");

    // 5x3: chroma rounds up to 3x2.
    QTest::newRow("I420") << int(GST_VIDEO_FORMAT_I420) << int(QVideoFrameFormat::Format_YUV420P) << QList<int>{3, 2, 2}
                          << QList<int>{5, 3, 3};
    QTest::newRow("YV12") << int(GST_VIDEO_FORMAT_YV12) << int(QVideoFrameFormat::Format_YV12) << QList<int>{3, 2, 2}
                          << QList<int>{5, 3, 3};
    QTest::newRow("NV12") << int(GST_VIDEO_FORMAT_NV12) << int(QVideoFrameFormat::Format_NV12) << QList<int>{3, 2}
                          << QList<int>{5, 6};
}

void GStreamerTest::_testCpuCopyOddDimensions()
{
    QFETCH(int, gstFormat);
    QFETCH(int, qtFormat);
    QFETCH(QList<int>, planeRows);
    QFETCH(QList<int>, planeRowBytes);

    GstVideoInfo info;
    QVERIFY(gst_video_info_set_format(&info, static_cast<GstVideoFormat>(gstFormat), 5, 3));
    GstBuffer* const buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    const auto bufferGuard = qScopeGuard([&] { gst_buffer_unref(buffer); });
    GstMapInfo source;
    QVERIFY(gst_buffer_map(buffer, &source, GST_MAP_READWRITE));
    const auto sourceGuard = qScopeGuard([&] { gst_buffer_unmap(buffer, &source); });
    for (gsize i = 0; i < source.size; ++i) {
        source.data[i] = static_cast<guint8>((i * 7) + 1);
    }

    QVideoFrame frame = CpuVideoFramePool::copyFromBuffer(
        buffer, info, QVideoFrameFormat(QSize(5, 3), static_cast<QVideoFrameFormat::PixelFormat>(qtFormat)));
    QVERIFY(frame.isValid());
    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    const auto frameGuard = qScopeGuard([&] { frame.unmap(); });
    QCOMPARE(frame.planeCount(), planeRows.size());
    for (int plane = 0; plane < frame.planeCount(); ++plane) {
        const int stride = frame.bytesPerLine(plane);
        QVERIFY(frame.mappedBytes(plane) >= ((planeRows[plane] - 1) * stride) + planeRowBytes[plane]);
        const auto* const src = reinterpret_cast<const char*>(source.data + GST_VIDEO_INFO_PLANE_OFFSET(&info, plane));
        const auto* const dst = reinterpret_cast<const char*>(frame.bits(plane));
        for (int row = 0; row < planeRows[plane]; ++row) {
            QCOMPARE(QByteArray(dst + (row * stride), planeRowBytes[plane]),
                     QByteArray(src + (row * GST_VIDEO_INFO_PLANE_STRIDE(&info, plane)), planeRowBytes[plane]));
        }
    }
}

namespace {

#if defined(QGC_HAS_ANY_GPU_PATH)
// Concrete subclass for unit-testing GstHwVideoBuffer's protected/base behavior.
class TestableHwVideoBuffer : public GstHwVideoBuffer
{
public:
    TestableHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat fmt)
        : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, info, std::move(fmt))
    {}

    /// Always fails, like a GPU import the active QRhi rejects.
    QVideoFrameTexturesUPtr mapTextures(QRhi&, QVideoFrameTexturesUPtr&) override { return {}; }
};

/// Advertises @p advertised while its memory holds the format it was built with, as the DMABuf eglfs quirk does.
class AdvertisingHwVideoBuffer final : public TestableHwVideoBuffer
{
public:
    AdvertisingHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat memoryFormat,
                             QVideoFrameFormat::PixelFormat advertised)
        : TestableHwVideoBuffer(sample, info, std::move(memoryFormat))
    {
        _format = QVideoFrameFormat(_format.frameSize(), advertised);
    }
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

#if defined(QGC_HAS_ANY_GPU_PATH)
namespace {

/// Behaviour of one test path in _testHwPathRegistrySelection, plus what the factory did with it.
struct TestPathState
{
    bool matches = false;
    bool admits = true;
    bool constructs = true;
    int constructed = 0;
    int importFailures = 0;
};

TestPathState s_testPathA;
TestPathState s_testPathB;

std::unique_ptr<GstHwVideoBuffer> constructTestBuffer(TestPathState& state, GstSample* sample, const GstVideoInfo& info,
                                                      QVideoFrameFormat format)
{
    ++state.constructed;
    return state.constructs ? std::make_unique<TestableHwVideoBuffer>(sample, info, std::move(format)) : nullptr;
}

const HwPathDescriptor kTestSelectPathA = {
    .path = HwVideoBufferPath::GlMemory,
    .matches = [](GstMemory*, const HwVideoBufferContext&) { return s_testPathA.matches; },
    .admits = [] { return s_testPathA.admits; },
    .construct = [](GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                    const HwVideoBufferContext&) { return constructTestBuffer(s_testPathA, sample, info, format); },
    .onImportFailed = [] { ++s_testPathA.importFailures; },
};

const HwPathDescriptor kTestSelectPathB = {
    .path = HwVideoBufferPath::Vulkan,
    .matches = [](GstMemory*, const HwVideoBufferContext&) { return s_testPathB.matches; },
    .construct = [](GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                    const HwVideoBufferContext&) { return constructTestBuffer(s_testPathB, sample, info, format); },
};

}  // namespace
#endif

void GStreamerTest::_testHwPathRegistrySelection_data()
{
    QTest::addColumn<bool>("aMatches");
    QTest::addColumn<bool>("aAdmits");
    QTest::addColumn<bool>("aConstructs");
    QTest::addColumn<int>("cachedPath");
    QTest::addColumn<int>("expectedPath");
    QTest::addColumn<int>("expectedBConstructed");
    QTest::addColumn<int>("expectedAImportFailures");

    // Path A (GL) is registered before path B (Vulkan); B always matches and imports.
    const int none = int(HwVideoBufferPath::None);
    const int gl = int(HwVideoBufferPath::GlMemory);
    const int vulkan = int(HwVideoBufferPath::Vulkan);
    QTest::newRow("first-registered-match-wins") << true << true << true << none << gl << 0 << 0;
    QTest::newRow("non-matching-path-skipped") << false << true << true << none << vulkan << 1 << 0;
    QTest::newRow("declining-path-sends-frame-to-cpu") << true << false << true << none << none << 0 << 0;
    QTest::newRow("failed-import-notifies-its-path") << true << true << false << none << none << 0 << 1;
    QTest::newRow("validated-path-tried-first") << true << true << true << vulkan << vulkan << 1 << 0;
}

void GStreamerTest::_testHwPathRegistrySelection()
{
#if !defined(QGC_HAS_ANY_GPU_PATH) || !defined(QGC_GST_BUILD_TESTING)
    QSKIP("GPU path registry not compiled in this build (no GPU path enabled)");
#else
    QFETCH(bool, aMatches);
    QFETCH(bool, aAdmits);
    QFETCH(bool, aConstructs);
    QFETCH(int, cachedPath);
    QFETCH(int, expectedPath);
    QFETCH(int, expectedBConstructed);
    QFETCH(int, expectedAImportFailures);

    s_testPathA = {.matches = aMatches, .admits = aAdmits, .constructs = aConstructs};
    s_testPathB = {.matches = true};
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestSelectPathA, &kTestSelectPathB});

    GstVideoInfo info;
    GstSample* sample = makeNv12Sample(16, 16, &info, false, 0, 0, 0, 0);
    QVERIFY(sample);
    const auto sampleGuard = qScopeGuard([sample] { gst_sample_unref(sample); });

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwResolvedPathCache cache;
    cache.path = static_cast<HwVideoBufferPath>(cachedPath);
    cache.validated = (cache.path != HwVideoBufferPath::None);
    HwVideoBufferPath path = HwVideoBufferPath::None;

    if (!aConstructs) {
        expectLogMessage("Video.GStreamer.HwBuffers.Factory", QtWarningMsg,
                         QRegularExpression(QStringLiteral("failed plane-handle validation")));
    }
    const auto buffer = makeHwVideoBuffer(
        sample, info, QVideoFrameFormat(QSize(16, 16), QVideoFrameFormat::Format_NV12), context, path, &cache);
    if (!aConstructs) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(int(path), expectedPath);
    QCOMPARE(bool(buffer), expectedPath != int(HwVideoBufferPath::None));
    QCOMPARE(cache.validated, expectedPath != int(HwVideoBufferPath::None));
    QCOMPARE(s_testPathB.constructed, expectedBConstructed);
    QCOMPARE(s_testPathA.importFailures, expectedAImportFailures);
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

void GStreamerTest::_testHwVideoBufferCpuMapFallback()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("GstHwVideoBuffer not compiled in this build (no GPU path enabled)");
#else
    GstVideoInfo info;
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 4, 2);
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    QByteArray pixels(static_cast<qsizetype>(GST_VIDEO_INFO_SIZE(&info)), Qt::Uninitialized);
    for (qsizetype i = 0; i < pixels.size(); ++i) {
        pixels[i] = static_cast<char>(i);
    }
    gst_buffer_fill(buffer, 0, pixels.constData(), static_cast<gsize>(pixels.size()));
    GstCaps* caps = gst_video_info_to_caps(&info);
    GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr);
    gst_caps_unref(caps);
    gst_buffer_unref(buffer);
    QVERIFY(sample);

    auto hw = std::make_unique<TestableHwVideoBuffer>(
        sample, info, QVideoFrameFormat(QSize(4, 2), QVideoFrameFormat::Format_BGRA8888));
    gst_sample_unref(sample);
    // A bundle from an earlier successful import takes its own sample ref; the CPU view must survive that.
    gst_sample_unref(hw->refSample());
    QVideoFrame frame(std::move(hw));

    // Qt's createTextures() falls back to uploading map() when mapTextures() fails.
    QRhiNullInitParams nullParams;
    const std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, &nullParams));
    QVERIFY(rhi);
    QRhiResourceUpdateBatch* updates = rhi->nextResourceUpdateBatch();
    QVideoFrameTexturesUPtr oldTextures;
    const QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTextures(frame, *rhi, *updates, oldTextures);
    updates->release();
    QVERIFY2(textures, "a failed GPU import must still produce textures through the CPU upload");

    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    const auto unmapGuard = qScopeGuard([&] { frame.unmap(); });
    QCOMPARE(frame.planeCount(), 1);
    QCOMPARE(frame.bytesPerLine(0), GST_VIDEO_INFO_PLANE_STRIDE(&info, 0));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(frame.bits(0)), frame.mappedBytes(0)), pixels);
#endif
}

void GStreamerTest::_testHwVideoBufferCpuMapRefusesForeignLayout_data()
{
    QTest::addColumn<int>("memoryFormat");
    QTest::addColumn<int>("advertisedFormat");
    QTest::addColumn<bool>("expectMapped");

    // The sample is always YUY2: a 4x2 frame holds 8-byte rows, which Qt would read as 16-byte RGBA rows.
    QTest::newRow("advertises-its-memory")
        << int(QVideoFrameFormat::Format_YUYV) << int(QVideoFrameFormat::Format_YUYV) << true;
    QTest::newRow("eglfs-yuyv-as-rgba") << int(QVideoFrameFormat::Format_YUYV)
                                        << int(QVideoFrameFormat::Format_RGBA8888) << false;
    QTest::newRow("external-oes") << int(QVideoFrameFormat::Format_SamplerExternalOES)
                                  << int(QVideoFrameFormat::Format_SamplerExternalOES) << false;
}

void GStreamerTest::_testHwVideoBufferCpuMapRefusesForeignLayout()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("GstHwVideoBuffer not compiled in this build (no GPU path enabled)");
#else
    QFETCH(int, memoryFormat);
    QFETCH(int, advertisedFormat);
    QFETCH(bool, expectMapped);

    GstVideoInfo info;
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_YUY2, 4, 2);
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    GstCaps* caps = gst_video_info_to_caps(&info);
    GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr);
    gst_caps_unref(caps);
    gst_buffer_unref(buffer);
    QVERIFY(sample);
    const auto sampleGuard = qScopeGuard([sample] { gst_sample_unref(sample); });

    AdvertisingHwVideoBuffer hw(sample, info,
                                QVideoFrameFormat(QSize(4, 2), QVideoFrameFormat::PixelFormat(memoryFormat)),
                                QVideoFrameFormat::PixelFormat(advertisedFormat));
    const QAbstractVideoBuffer::MapData mapped = hw.map(QVideoFrame::ReadOnly);
    QCOMPARE(mapped.planeCount > 0, expectMapped);
    hw.unmap();
#endif
}

void GStreamerTest::_testHwBufferCropMatrixFromVideoCropMeta()
{
    // Regression: GstVideoCropMeta is propagated via QVideoFrameFormat::viewport() in
    // applyCropMeta (GstQgcFrameMap), NOT via externalTextureMatrix. Verify the format
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

void GStreamerTest::_testTelemetryMapFailuresReportedInExtraStats()
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

    // takeExtraPathStats() reports registered paths only; the qgc plugin registers them when it loads.
    HwBuffers::initializeOnce();
    (void) HwBuffers::takeExtraPathStats();

    GstHwPathTelemetry::recordMapFailure(path);

    const QString failures = QStringLiteral(" %1-failures:1").arg(QLatin1String(GstHw::toString(path)));
    const QString first = HwBuffers::takeExtraPathStats();
    QVERIFY2(first.contains(failures), qPrintable(first));
    const QString second = HwBuffers::takeExtraPathStats();
    QVERIFY2(!second.contains(QStringLiteral("-failures:")), qPrintable(second));
#endif
}

void GStreamerTest::_testTelemetryDmaBufExtraStatsDrain()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    (void) HwBuffers::takeExtraPathStats();
    GstHwPathTelemetry::recordProducerFenceTimeout(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordCpuBarrierWait(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordExplicitFenceWait(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordImageCacheHit(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordImageCacheHit(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordImageCacheMiss(HwVideoBufferPath::DmaBuf);

    const QString first = HwBuffers::takeExtraPathStats();
    QVERIFY(first.contains(QStringLiteral("DMABuf-fence-timeouts:")));
    QVERIFY(first.contains(QStringLiteral("DMABuf-cpu-barriers:")));
    QVERIFY(first.contains(QStringLiteral("DMABuf-explicit-fence-waits:")));
    QVERIFY2(first.contains(QStringLiteral("DMABuf-image-cache[hit/miss]:2/1")), qPrintable(first));

    const QString second = HwBuffers::takeExtraPathStats();
    QVERIFY(second.contains(QStringLiteral("DMABuf-fence-timeouts:0")));
    QVERIFY(second.contains(QStringLiteral("DMABuf-cpu-barriers:0")));
    QVERIFY(second.contains(QStringLiteral("DMABuf-explicit-fence-waits:0")));
    QVERIFY(!second.contains(QStringLiteral("image-cache")));
#endif
}

#endif
