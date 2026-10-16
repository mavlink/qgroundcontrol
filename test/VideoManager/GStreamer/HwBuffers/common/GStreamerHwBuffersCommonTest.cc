#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <memory>
#include <rhi/qrhi.h>
#include <span>

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtQuick/QQuickWindow>

#include "CpuVideoFramePool.h"
#include "Fixtures/RAIIFixtures.h"
#include "GStreamerTestHelpers.h"
#include "GstContextBridgeBase.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"
#include "GstQgcFrameMap.h"
#include "HwBuffers/common/HwBuffers.h"
#include "gstqgc/GstQgcVideoFormats.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "GstHwImportPreflight.h"
#include "QGCRhiCapture.h"
#endif

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
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else

    testBridgeA().restart();
    testBridgeB().restart();
    const auto bridgeGuard = qScopeGuard([] {
        testBridgeA().restart();
        testBridgeB().restart();
    });
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestBridgePathA, &kTestBridgePathB});

    GstElement* const identity = gst_element_factory_make("identity", nullptr);
    QVERIFY(identity);
    const GStreamer::GstElementPtr owner = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(identity)));
    GstElement* const element = owner.get();

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
#if !defined(QGC_HAS_ANY_GPU_PATH)
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

    const GStreamer::GErrorPtr ordinaryError = GStreamer::adoptError(
        g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_FAILED, "ordinary stream error"));
    HwBuffers::onPipelineError(ordinaryError.get(), "debug");
    QCOMPARE(testBridgeA().clears, 1);
    QCOMPARE(s_testCacheResets, 1);

    const GStreamer::GErrorPtr deviceLostError =
        GStreamer::adoptError(g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_FAILED, "DEVICE_REMOVED"));
    HwBuffers::onPipelineError(deviceLostError.get(), "debug");
    QCOMPARE(testBridgeA().clears, 2);
    QCOMPARE(s_testCacheResets, 2);

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

    // videotestsrc produces system memory; no GPU path should match.
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 ! "
                                            "video/x-raw,format=BGRA,width=64,height=64,framerate=30/1 ! "
                                            "fakesink name=sink enable-last-sample=true sync=false")),
             qPrintable(pipeline.error()));
    QVERIFY(pipeline.play());
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    GstSample* sample = nullptr;
    g_object_get(pipeline.element("sink").get(), "last-sample", &sample, nullptr);
    QVERIFY2(sample, "fakesink kept no last sample");

    GstVideoInfo info;
    GstCaps* caps = gst_sample_get_caps(sample);
    QVERIFY(gst_video_info_from_caps(&info, caps));

    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_BGRA8888);
    HwVideoBufferPath path = HwVideoBufferPath::None;

    HwVideoBufferContext ctxGpu;
    ctxGpu.gpuEnabled = true;
    auto buf = makeHwVideoBuffer(gst_sample_get_buffer(sample), caps, info, fmt, ctxGpu, path);
    QVERIFY(!buf);
    QCOMPARE(path, HwVideoBufferPath::None);

    HwVideoBufferContext ctxCpu;
    ctxCpu.gpuEnabled = false;
    auto buf2 = makeHwVideoBuffer(gst_sample_get_buffer(sample), caps, info, fmt, ctxCpu, path);
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
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("Import preflight not compiled in this build (no GPU path enabled)");
#else
    std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
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

void GStreamerTest::_testGpuBinModeFromEnvironment_data()
{
    QTest::addColumn<QByteArray>("value");  // null: variable unset
    QTest::addColumn<int>("expectedMode");
    QTest::addColumn<bool>("warns");

    const int glUpload = int(HwBuffers::GpuBinMode::GlUpload);
    const int directDmaBuf = int(HwBuffers::GpuBinMode::DirectDmaBuf);
    QTest::newRow("unset") << QByteArray() << glUpload << false;
    QTest::newRow("empty") << QByteArray("") << glUpload << false;
    QTest::newRow("glupload") << QByteArray("glupload") << glUpload << false;
    QTest::newRow("dmabuf-padded-mixed-case") << QByteArray(" DmaBuf ") << directDmaBuf << false;
    QTest::newRow("unknown") << QByteArray("vulkan") << glUpload << true;
}

void GStreamerTest::_testGpuBinModeFromEnvironment()
{
#if !defined(QGC_GST_BIN_HAS_GLUPLOAD) || !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QSKIP("QGC_GST_GPU_BIN only selects a bin when both GPU bins are compiled");
#else
    QFETCH(QByteArray, value);
    QFETCH(int, expectedMode);
    QFETCH(bool, warns);

    const TestFixtures::EnvVarFixture guard("QGC_GST_GPU_BIN");
    if (value.isNull()) {
        qunsetenv("QGC_GST_GPU_BIN");
    } else {
        qputenv("QGC_GST_GPU_BIN", value);
    }

    if (warns) {
        expectLogMessage("Video.GStreamer.HwBuffers.Config", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Unknown QGC_GST_GPU_BIN value.*default glupload bin")));
    }
    QCOMPARE(int(HwBuffers::readGpuBinMode()), expectedMode);
    if (warns) {
        verifyExpectedLogMessage();
    }
#endif
}

void GStreamerTest::_testCpuLayoutSizesPlanesByComponent()
{
    // AV12's third plane holds component 3 (full-height alpha); component 2 is the half-height V in plane 1.
    // _testCpuCopyOddDimensions covers the formats whose planes and components line up.
    GstVideoInfo info;
    QVERIFY(gst_video_info_set_format(&info, GST_VIDEO_FORMAT_AV12, 64, 48));
    const CpuVideoFramePool::PlaneLayout layout = CpuVideoFramePool::computeLayout(info);
    QCOMPARE(layout.planeCount, 3);
    QCOMPARE(layout.height[0], 48);
    QCOMPARE(layout.height[1], 24);
    QCOMPARE(layout.height[2], 48);
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
    // Packed 4:2:2: the last macropixel's chroma ends past the last luma byte (YUY2 V at byte 11, UYVY V at 10).
    QTest::newRow("YUY2") << int(GST_VIDEO_FORMAT_YUY2) << int(QVideoFrameFormat::Format_YUYV) << QList<int>{3}
                          << QList<int>{12};
    QTest::newRow("UYVY") << int(GST_VIDEO_FORMAT_UYVY) << int(QVideoFrameFormat::Format_UYVY) << QList<int>{3}
                          << QList<int>{11};
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

#endif

}  // namespace

#if defined(QGC_HAS_ANY_GPU_PATH)
namespace {

/// Behaviour of one test path in _testHwPathRegistrySelection, plus what the factory did with it.
struct TestPathState
{
    bool matches = false;
    bool admits = true;
    bool constructs = true;
    int constructed = 0;
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

    // Path A (GL) is registered before path B (Vulkan); B always matches and imports.
    const int none = int(HwVideoBufferPath::None);
    const int gl = int(HwVideoBufferPath::GlMemory);
    const int vulkan = int(HwVideoBufferPath::Vulkan);
    QTest::newRow("first-registered-match-wins") << true << true << true << none << gl << 0;
    QTest::newRow("non-matching-path-skipped") << false << true << true << none << vulkan << 1;
    QTest::newRow("declining-path-sends-frame-to-cpu") << true << false << true << none << none << 0;
    QTest::newRow("failed-import-sends-frame-to-cpu") << true << true << false << none << none << 0;
    QTest::newRow("validated-path-tried-first") << true << true << true << vulkan << vulkan << 1;
}

void GStreamerTest::_testHwPathRegistrySelection()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("GPU path registry not compiled in this build (no GPU path enabled)");
#else
    QFETCH(bool, aMatches);
    QFETCH(bool, aAdmits);
    QFETCH(bool, aConstructs);
    QFETCH(int, cachedPath);
    QFETCH(int, expectedPath);
    QFETCH(int, expectedBConstructed);

    s_testPathA = {.matches = aMatches, .admits = aAdmits, .constructs = aConstructs};
    s_testPathB = {.matches = true};
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestSelectPathA, &kTestSelectPathB});

    GstVideoInfo info;
    GstSample* sample = GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, 16, 16, &info);
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
    const auto buffer =
        makeHwVideoBuffer(gst_sample_get_buffer(sample), gst_sample_get_caps(sample), info,
                          QVideoFrameFormat(QSize(16, 16), QVideoFrameFormat::Format_NV12), context, path, &cache);
    if (!aConstructs) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(int(path), expectedPath);
    QCOMPARE(bool(buffer), expectedPath != int(HwVideoBufferPath::None));
    QCOMPARE(cache.validated, expectedPath != int(HwVideoBufferPath::None));
    QCOMPARE(s_testPathB.constructed, expectedBConstructed);
#endif
}

void GStreamerTest::_testHwBufferCropMatrixIdentityWithoutMeta()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("GstHwVideoBuffer not compiled in this build (no GPU path enabled)");
#else
    GstVideoInfo info;
    GstSample* sample = GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, 320, 240, &info);
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
    GstSample* sample = GstTest::makeVideoSample(GST_VIDEO_FORMAT_BGRA, 4, 2, &info);
    QVERIFY(sample);
    QByteArray pixels(static_cast<qsizetype>(GST_VIDEO_INFO_SIZE(&info)), Qt::Uninitialized);
    for (qsizetype i = 0; i < pixels.size(); ++i) {
        pixels[i] = static_cast<char>(i);
    }
    gst_buffer_fill(gst_sample_get_buffer(sample), 0, pixels.constData(), static_cast<gsize>(pixels.size()));

    auto hw = std::make_unique<TestableHwVideoBuffer>(
        sample, info, QVideoFrameFormat(QSize(4, 2), QVideoFrameFormat::Format_BGRA8888));
    gst_sample_unref(sample);
    // A bundle from an earlier successful import takes its own sample ref; the CPU view must survive that.
    gst_sample_unref(hw->refSample());
    QVideoFrame frame(std::move(hw));

    // Qt's createTextures() falls back to uploading map() when mapTextures() fails.
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
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
    GstSample* sample = GstTest::makeVideoSample(GST_VIDEO_FORMAT_YUY2, 4, 2, &info);
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
    GstSample* sample = GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, 400, 200, &info);
    QVERIFY(sample);
    GstBuffer* buf = gst_sample_get_buffer(sample);
    GstVideoCropMeta* crop = gst_buffer_add_video_crop_meta(buf);
    crop->x = 100;
    crop->y = 50;
    crop->width = 200;
    crop->height = 100;

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
    (void) GstHwPathTelemetry::takeFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::MapFailed);
    (void) GstHwPathTelemetry::takeFallbackReason(HwVideoBufferPath::D3D12, HwFallbackReason::EglBadMatch);

    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch);
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch);

    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch),
             quint64(2));
    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::MapFailed), quint64(0));
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

    using GstHwPathTelemetry::Counter;
    for (const Counter counter :
         {Counter::CpuWaits, Counter::GpuWaits, Counter::ImageCacheHits, Counter::ImageCacheMisses}) {
        (void) GstHwPathTelemetry::take(path, counter);
    }

    GstHwPathTelemetry::record(path, Counter::CpuWaits);
    GstHwPathTelemetry::record(path, Counter::CpuWaits);
    GstHwPathTelemetry::record(path, Counter::GpuWaits);
    GstHwPathTelemetry::record(path, Counter::ImageCacheHits);
    GstHwPathTelemetry::record(path, Counter::ImageCacheHits);
    GstHwPathTelemetry::record(path, Counter::ImageCacheMisses);

    QCOMPARE(GstHwPathTelemetry::take(path, Counter::CpuWaits), quint64(2));
    QCOMPARE(GstHwPathTelemetry::take(path, Counter::GpuWaits), quint64(1));
    QCOMPARE(GstHwPathTelemetry::take(path, Counter::ImageCacheHits), quint64(2));
    QCOMPARE(GstHwPathTelemetry::take(path, Counter::ImageCacheMisses), quint64(1));

    QCOMPARE(GstHwPathTelemetry::take(path, Counter::CpuWaits), quint64(0));
    QCOMPARE(GstHwPathTelemetry::take(path, Counter::GpuWaits), quint64(0));
    QCOMPARE(GstHwPathTelemetry::take(path, Counter::ImageCacheHits), quint64(0));
    QCOMPARE(GstHwPathTelemetry::take(path, Counter::ImageCacheMisses), quint64(0));
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

    (void) HwBuffers::takeExtraPathStats();

    GstHwPathTelemetry::record(path, GstHwPathTelemetry::Counter::MapFailures);

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
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ProducerFenceTimeouts);
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::CpuWaits);
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::GpuWaits);
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ImageCacheHits);
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ImageCacheHits);
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ImageCacheMisses);

    const QString first = HwBuffers::takeExtraPathStats();
    QVERIFY2(first.contains(QStringLiteral("DMABuf-fence-timeouts:1")), qPrintable(first));
    QVERIFY2(first.contains(QStringLiteral("DMABuf-wait[gpu/cpu]:1/1")), qPrintable(first));
    QVERIFY2(first.contains(QStringLiteral("DMABuf-image-cache[hit/miss]:2/1")), qPrintable(first));

    const QString second = HwBuffers::takeExtraPathStats();
    QVERIFY2(!second.contains(QStringLiteral("fence-timeouts")), qPrintable(second));
    QVERIFY2(!second.contains(QStringLiteral("wait[gpu/cpu]")), qPrintable(second));
    QVERIFY2(!second.contains(QStringLiteral("image-cache")), qPrintable(second));
#endif
}

#endif
