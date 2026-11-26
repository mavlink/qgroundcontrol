#include "HwBuffers/GstHwBuffersTest.h"

#include <gst/gst.h>
#include <memory>
#include <rhi/qrhi.h>
#include <span>
#include <thread>

#include <QtCore/QLoggingCategory>
#include <QtCore/QRect>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtQuick/QQuickWindow>

#include "GStreamerTestHelpers.h"
#include "GstContextBridgeBase.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "HwBuffers/HwBuffers.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
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

    /// As a device that reports its removal has its bridge do, on the thread it notifies on.
    void removeDevice() { noteDeviceRemoved(); }

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
    .matches = [](GstMemory*) { return false; },
    .resetCaches = [] { ++s_testCacheResets; },
    .bridge = []() -> GstContextBridgeBase& { return testBridgeA(); },
};

const HwPathDescriptor kTestBridgePathB = {
    .path = HwVideoBufferPath::AHardwareBuffer,
    .matches = [](GstMemory*) { return false; },
    .bridge = []() -> GstContextBridgeBase& { return testBridgeB(); },
};

/// True when @p element holds a context of @p contextType that a TestContextBridge built.
bool holdsTestBridgeContext(GstElement* element, const char* contextType = kTestContextTypeA[0])
{
    const GStreamer::GstContextPtr context = GStreamer::adoptContext(gst_element_get_context(element, contextType));
    return context && gst_structure_has_field(gst_context_get_structure(context.get()), "object");
}

GStreamer::GstElementPtr newPipeline()
{
    return GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_pipeline_new(nullptr))));
}

/// Takes the next element message off @p pipeline's bus: whether it is the pipeline's own message @p name.
bool nextElementMessageIs(GstElement* pipeline, const char* name)
{
    const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(pipeline));
    const GStreamer::GstMessagePtr message =
        GStreamer::adoptMessage(gst_bus_pop_filtered(bus.get(), GST_MESSAGE_ELEMENT));
    return message && gst_message_has_name(message.get(), name) &&
           (GST_MESSAGE_SRC(message.get()) == GST_OBJECT(pipeline));
}

/// A new identity element in @p bin, which owns it; null when it couldn't be added.
GstElement* addIdentityTo(GstElement* bin)
{
    GstElement* const element = gst_element_factory_make("identity", nullptr);
    return (element && gst_bin_add(GST_BIN(bin), element)) ? element : nullptr;
}

}  // namespace

void GstHwBuffersTest::_testContextBridgeRegistry()
{
    testBridgeA().restart();
    testBridgeB().restart();
    const auto bridgeGuard = qScopeGuard([] {
        testBridgeA().restart();
        testBridgeB().restart();
    });
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestBridgePathA, &kTestBridgePathB});

    // Every bridge sets its contexts on each new pipeline, and primes for the first only.
    for (int i = 0; i < 2; ++i) {
        const GStreamer::GstElementPtr pipeline = newPipeline();
        HwBuffers::provideGraphicsContexts(pipeline.get());
        QVERIFY(holdsTestBridgeContext(pipeline.get(), kTestContextTypeA[0]));
        QVERIFY(holdsTestBridgeContext(pipeline.get(), kTestContextTypeB[0]));
    }
    QCOMPARE(testBridgeA().primes, 1);
    QCOMPARE(testBridgeB().primes, 1);
}

void GstHwBuffersTest::_testPipelineHandsBridgeContextsToItsElements()
{
    testBridgeA().restart();
    const auto bridgeGuard = qScopeGuard([] { testBridgeA().restart(); });
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestBridgePathA});

    const GStreamer::GstElementPtr pipeline = newPipeline();
    HwBuffers::provideGraphicsContexts(pipeline.get());

    // A bin built before it joins the pipeline, as the sink bin is, and an element added to it later, as decodebin3
    // plugs a decoder: each takes the device from the pipeline, without asking the bridge.
    GstElement* const bin = gst_bin_new(nullptr);
    GstElement* const builtFirst = addIdentityTo(bin);
    QVERIFY(builtFirst);
    QVERIFY(gst_bin_add(GST_BIN(pipeline.get()), bin));
    GstElement* const pluggedLater = addIdentityTo(bin);
    GstElement* const sibling = addIdentityTo(pipeline.get());
    QVERIFY(pluggedLater && sibling);
    QVERIFY(holdsTestBridgeContext(builtFirst));
    QVERIFY(holdsTestBridgeContext(pluggedLater));
    QVERIFY(holdsTestBridgeContext(sibling));
    QCOMPARE(testBridgeA().primes, 1);
}

void GstHwBuffersTest::_testRemovedDeviceIsToldToThePipelinesThatGotIt()
{
    testBridgeA().restart();
    const auto bridgeGuard = qScopeGuard([] { testBridgeA().restart(); });
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestBridgePathA});

    // Two pipelines got the device when they were built, one did not.
    const GStreamer::GstElementPtr first = newPipeline();
    HwBuffers::provideGraphicsContexts(first.get());
    const GStreamer::GstElementPtr second = newPipeline();
    HwBuffers::provideGraphicsContexts(second.get());
    const GStreamer::GstElementPtr bystander = newPipeline();
    QVERIFY(!HwBuffers::graphicsDeviceLost());

    // A device notifies of its removal on a thread of its own.
    std::thread([] { testBridgeA().removeDevice(); }).join();
    QVERIFY(HwBuffers::graphicsDeviceLost());
    // The GUI thread drops the device and tells the pipelines, which hold elements that work on it.
    QTRY_COMPARE_WITH_TIMEOUT(testBridgeA().clears, 1, TestTimeout::mediumMs());
    QVERIFY(nextElementMessageIs(first.get(), HwBuffers::kGraphicsDeviceLostMessage));
    QVERIFY(nextElementMessageIs(second.get(), HwBuffers::kGraphicsDeviceLostMessage));
    QVERIFY(!nextElementMessageIs(bystander.get(), HwBuffers::kGraphicsDeviceLostMessage));

    // Lost until the bridge has a device again.
    QVERIFY(HwBuffers::graphicsDeviceLost());
    QVERIFY(testBridgeA().prime());
    QVERIFY(!HwBuffers::graphicsDeviceLost());
}

void GstHwBuffersTest::_testHwBufferLifecycleResetsNativeCaches()
{
    testBridgeA().restart();
    const auto bridgeGuard = qScopeGuard([] { testBridgeA().restart(); });
    s_testCacheResets = 0;
    const GstHwPathRegistry::ScopedPathsForTest paths({&kTestBridgePathA});

    QVERIFY(testBridgeA().prime());
    HwBuffers::resetCachedGpuResources();
    QCOMPARE(testBridgeA().clears, 1);
    QCOMPARE(s_testCacheResets, 1);
}

void GstHwBuffersTest::_testGraphicsDeviceSettledUnderSoftwareSceneGraph()
{
    // Without a main window nothing waits for its device.
    QVERIFY(HwBuffers::graphicsDeviceSettled());

    // The scene graph backend is known once a window exists.
    QQuickWindow window;
    if (QQuickWindow::sceneGraphBackend() != QLatin1String("software")) {
        QSKIP("Qt Quick renders through a QRhi here");
    }
    // The software scene graph publishes no device, and its threaded render loop never reports that it is up.
    HwBuffers::connectMainWindow(&window);
    QVERIFY(HwBuffers::graphicsDeviceSettled());
}

void GstHwBuffersTest::_testQGCRhiCaptureCacheLifecycle()
{
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);

    // The scene graph is never initialized here, so the Null QRhi's device stands in for the one its render thread
    // would publish.
    auto* window = new QQuickWindow();
    QGCRhiCapture::connectWindow(window);
    QGCRhiCapture::publishDeviceSnapshotForTest(rhi.get());
    QCOMPARE(QGCRhiCapture::deviceSnapshot().backend.load(), static_cast<int>(QRhi::Null));

    // Destroying the window must withdraw the device (no dangling handles).
    delete window;
    QCOMPARE(QGCRhiCapture::deviceSnapshot().backend.load(), -1);
}
#endif

#if defined(QGC_HAS_ANY_GPU_PATH)
namespace {

class TestableHwVideoBuffer : public GstHwVideoBuffer
{
public:
    TestableHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat fmt)
        : GstHwVideoBuffer(sample, info, std::move(fmt))
    {}

    HwVideoBufferPath path() const override { return HwVideoBufferPath::GlMemory; }

    /// Always fails, like a GPU import the active QRhi rejects.
    QVideoFrameTexturesUPtr importTextures(QRhi&, QVideoFrameTexturesUPtr&) override { return {}; }
};

/// Advertises @p advertised while its memory holds the format it was built with, as the DMABuf eglfs quirk does.
class AdvertisingHwVideoBuffer final : public TestableHwVideoBuffer
{
public:
    AdvertisingHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat memoryFormat,
                             QVideoFrameFormat advertised)
        : TestableHwVideoBuffer(sample, info, std::move(memoryFormat))
    {
        _format = std::move(advertised);
    }
};

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
    .matches = [](GstMemory*) { return s_testPathA.matches; },
    .admits = [](GstMemory*, const HwVideoBufferContext&) { return s_testPathA.admits; },
    .construct = [](GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                    const HwVideoBufferContext&) { return constructTestBuffer(s_testPathA, sample, info, format); },
};

const HwPathDescriptor kTestSelectPathB = {
    .path = HwVideoBufferPath::AHardwareBuffer,
    .matches = [](GstMemory*) { return s_testPathB.matches; },
    .construct = [](GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                    const HwVideoBufferContext&) { return constructTestBuffer(s_testPathB, sample, info, format); },
};

}  // namespace

void GstHwBuffersTest::_testHwPathRegistrySelection_data()
{
    QTest::addColumn<bool>("aMatches");
    QTest::addColumn<bool>("aAdmits");
    QTest::addColumn<bool>("aConstructs");
    QTest::addColumn<int>("cachedPath");
    QTest::addColumn<int>("expectedPath");
    QTest::addColumn<int>("expectedBConstructed");

    // Path A (GL) is registered before path B (AHardwareBuffer); B always matches and imports.
    const int none = int(HwVideoBufferPath::None);
    const int gl = int(HwVideoBufferPath::GlMemory);
    const int ahb = int(HwVideoBufferPath::AHardwareBuffer);
    QTest::newRow("first-registered-match-wins") << true << true << true << none << gl << 0;
    QTest::newRow("non-matching-path-skipped") << false << true << true << none << ahb << 1;
    QTest::newRow("declining-path-sends-frame-to-cpu") << true << false << true << none << none << 0;
    QTest::newRow("failed-import-sends-frame-to-cpu") << true << true << false << none << none << 0;
    QTest::newRow("validated-path-tried-first") << true << true << true << ahb << ahb << 1;
}

void GstHwBuffersTest::_testHwPathRegistrySelection()
{
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
    const GstTest::SamplePtr sample(GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, 16, 16, &info));
    QVERIFY(sample);

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
        makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), gst_sample_get_caps(sample.get()), info,
                          QVideoFrameFormat(QSize(16, 16), QVideoFrameFormat::Format_NV12), context, path, &cache);
    if (!aConstructs) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(int(path), expectedPath);
    QCOMPARE(bool(buffer), expectedPath != int(HwVideoBufferPath::None));
    QCOMPARE(cache.validated, expectedPath != int(HwVideoBufferPath::None));
    QCOMPARE(s_testPathB.constructed, expectedBConstructed);
}

void GstHwBuffersTest::_testHwVideoBufferCpuMapFallback()
{
    GstVideoInfo info;
    GstTest::SamplePtr sample(GstTest::makeVideoSample(GST_VIDEO_FORMAT_BGRA, 4, 2, &info));
    QVERIFY(sample);
    QByteArray pixels(static_cast<qsizetype>(GST_VIDEO_INFO_SIZE(&info)), Qt::Uninitialized);
    for (qsizetype i = 0; i < pixels.size(); ++i) {
        pixels[i] = static_cast<char>(i);
    }
    gst_buffer_fill(gst_sample_get_buffer(sample.get()), 0, pixels.constData(), static_cast<gsize>(pixels.size()));

    auto hw = std::make_unique<TestableHwVideoBuffer>(
        sample.get(), info, QVideoFrameFormat(QSize(4, 2), QVideoFrameFormat::Format_BGRA8888));
    sample.reset();
    // The textures of an earlier successful import take their own sample ref; the CPU view must survive that.
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
}

void GstHwBuffersTest::_testHwVideoBufferCpuMapRefusesForeignLayout_data()
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

void GstHwBuffersTest::_testHwVideoBufferCpuMapRefusesForeignLayout()
{
    QFETCH(int, memoryFormat);
    QFETCH(int, advertisedFormat);
    QFETCH(bool, expectMapped);

    GstVideoInfo info;
    const GstTest::SamplePtr sample(GstTest::makeVideoSample(GST_VIDEO_FORMAT_YUY2, 4, 2, &info));
    QVERIFY(sample);

    AdvertisingHwVideoBuffer hw(sample.get(), info,
                                QVideoFrameFormat(QSize(4, 2), QVideoFrameFormat::PixelFormat(memoryFormat)),
                                QVideoFrameFormat(QSize(4, 2), QVideoFrameFormat::PixelFormat(advertisedFormat)));
    const QAbstractVideoBuffer::MapData mapped = hw.map(QVideoFrame::ReadOnly);
    QCOMPARE(mapped.planeCount > 0, expectMapped);
    hw.unmap();
}

void GstHwBuffersTest::_testWithPixelFormatKeepsOtherProperties()
{
    // Every property QVideoFrameFormat has a setter for (Qt 6.12), each away from its default.
    QVideoFrameFormat format(QSize(64, 48), QVideoFrameFormat::Format_YUYV);
    format.setViewport(QRect(2, 4, 60, 40));
    format.setScanLineDirection(QVideoFrameFormat::BottomToTop);
    format.setStreamFrameRate(25.0);
    format.setColorSpace(QVideoFrameFormat::ColorSpace_BT2020);
    format.setColorTransfer(QVideoFrameFormat::ColorTransfer_ST2084);
    format.setColorRange(QVideoFrameFormat::ColorRange_Full);
    format.setMirrored(true);
    format.setRotation(QtVideo::Rotation::Clockwise90);
    format.setMaxLuminance(1000.0f);

    const QVideoFrameFormat rgba = GstHw::withPixelFormat(format, QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(rgba.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(rgba.frameSize(), format.frameSize());
    QCOMPARE(rgba.viewport(), format.viewport());
    QCOMPARE(rgba.scanLineDirection(), format.scanLineDirection());
    QCOMPARE(rgba.streamFrameRate(), format.streamFrameRate());
    QCOMPARE(rgba.colorSpace(), format.colorSpace());
    QCOMPARE(rgba.colorTransfer(), format.colorTransfer());
    QCOMPARE(rgba.colorRange(), format.colorRange());
    QCOMPARE(rgba.isMirrored(), format.isMirrored());
    QCOMPARE(rgba.rotation(), format.rotation());
    QCOMPARE(rgba.maxLuminance(), format.maxLuminance());
}

void GstHwBuffersTest::_testTexturePadsPicture_data()
{
    QTest::addColumn<QSize>("textureSize");
    QTest::addColumn<QSize>("picture");
    QTest::addColumn<bool>("evenSizes");
    QTest::addColumn<bool>("pads");

    const QSize picture(1920, 1080);
    // H.264 aligns to 16 rows, HEVC to 128: 8 and 72 rows the decoder never wrote.
    QTest::newRow("h264-padded") << QSize(1920, 1088) << picture << true << true;
    QTest::newRow("hevc-padded") << QSize(1920, 1152) << picture << true << true;
    QTest::newRow("width-padded") << QSize(1936, 1080) << picture << true << true;
    QTest::newRow("exact-texture") << picture << picture << true << false;
    QTest::newRow("smaller-texture") << QSize(1920, 1072) << picture << true << false;
    // A 4:2:0 texture holds an odd picture in the next even size.
    QTest::newRow("odd-picture-even-texture") << QSize(642, 482) << QSize(641, 481) << true << false;
    QTest::newRow("odd-picture-rgba-texture") << QSize(642, 482) << QSize(641, 481) << false << true;
}

void GstHwBuffersTest::_testTexturePadsPicture()
{
    QFETCH(QSize, textureSize);
    QFETCH(QSize, picture);
    QFETCH(bool, evenSizes);
    QFETCH(bool, pads);

    QCOMPARE(GstHw::texturePadsPicture(textureSize, picture, evenSizes), pads);
}
#endif

void GstHwBuffersTest::_testTelemetryMapDurationEwma()
{
    const HwVideoBufferPath path = HwVideoBufferPath::AHardwareBuffer;

    GstHwPathTelemetry::recordMapDuration(path, 80000);
    const quint64 seeded = GstHwPathTelemetry::peekMapDurationUsEwma(path);
    QVERIFY(seeded > 0);

    GstHwPathTelemetry::recordMapDuration(path, -5);
    QCOMPARE(GstHwPathTelemetry::peekMapDurationUsEwma(path), seeded);

    GstHwPathTelemetry::recordMapDuration(path, 160000);
    const quint64 expected = seeded - (seeded >> 3) + (160u >> 3);
    QCOMPARE(GstHwPathTelemetry::peekMapDurationUsEwma(path), expected);
}

void GstHwBuffersTest::_testTelemetryCountsPerPathAndKind()
{
    using GstHwPathTelemetry::Counter;
    using GstHwPathTelemetry::HwFallbackReason;

    const GstHwPathTelemetry::Snapshot before = GstHwPathTelemetry::snapshot();
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch);
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch);
    GstHwPathTelemetry::record(HwVideoBufferPath::IOSurface, Counter::CpuWaits);
    GstHwPathTelemetry::record(HwVideoBufferPath::IOSurface, Counter::CpuWaits);
    GstHwPathTelemetry::record(HwVideoBufferPath::IOSurface, Counter::GpuWaits);
    const GstHwPathTelemetry::Snapshot after = GstHwPathTelemetry::snapshot();

    const auto delta = [&](HwVideoBufferPath path, auto kind) {
        return after.count(path, kind) - before.count(path, kind);
    };
    QCOMPARE(delta(HwVideoBufferPath::D3D11, HwFallbackReason::EglBadMatch), quint64(2));
    QCOMPARE(delta(HwVideoBufferPath::D3D11, HwFallbackReason::MapFailed), quint64(0));
    QCOMPARE(delta(HwVideoBufferPath::D3D12, HwFallbackReason::EglBadMatch), quint64(0));
    QCOMPARE(delta(HwVideoBufferPath::IOSurface, Counter::CpuWaits), quint64(2));
    QCOMPARE(delta(HwVideoBufferPath::IOSurface, Counter::GpuWaits), quint64(1));
}

#if defined(QGC_HAS_ANY_GPU_PATH)
void GstHwBuffersTest::_testTelemetryMapFailuresReportedInExtraStats()
{
    // Any compiled path: those are the ones HwBuffers::extraPathStatsSince() reports.
    const HwVideoBufferPath path = GstHwPathRegistry::paths().front()->path;

    const GstHwPathTelemetry::Snapshot receiverBaseline = GstHwPathTelemetry::snapshot();
    GstHwPathTelemetry::record(path, GstHwPathTelemetry::Counter::MapFailures);
    const GstHwPathTelemetry::Snapshot laterBaseline = GstHwPathTelemetry::snapshot();

    // Reading never resets the counters: each receiver reports what changed since its own baseline.
    const QString failures = QStringLiteral(" %1-failures:1").arg(QLatin1String(GstHw::toString(path)));
    for (int read = 0; read < 2; ++read) {
        const QString sinceBaseline = HwBuffers::extraPathStatsSince(receiverBaseline);
        QVERIFY2(sinceBaseline.contains(failures), qPrintable(sinceBaseline));
    }
    const QString sinceLater = HwBuffers::extraPathStatsSince(laterBaseline);
    QVERIFY2(!sinceLater.contains(QStringLiteral("-failures:")), qPrintable(sinceLater));
}

void GstHwBuffersTest::_testHwBufferFactoryDispatchSystemMemory()
{
    // A frame in system memory, as a software decoder sends it: no GPU path should match.
    GstVideoInfo info;
    const GstTest::SamplePtr sample(GstTest::makeVideoSample(GST_VIDEO_FORMAT_BGRA, 64, 64, &info));
    QVERIFY(sample);
    GstCaps* caps = gst_sample_get_caps(sample.get());

    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_BGRA8888);
    HwVideoBufferPath path = HwVideoBufferPath::None;

    HwVideoBufferContext ctxGpu;
    ctxGpu.gpuEnabled = true;
    auto buf = makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), caps, info, fmt, ctxGpu, path);
    QVERIFY(!buf);
    QCOMPARE(path, HwVideoBufferPath::None);

    HwVideoBufferContext ctxCpu;
    ctxCpu.gpuEnabled = false;
    auto buf2 = makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), caps, info, fmt, ctxCpu, path);
    QVERIFY(!buf2);
}
#endif

UT_REGISTER_TEST(GstHwBuffersTest, TestLabel::Unit)
