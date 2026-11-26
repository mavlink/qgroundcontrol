#include "HwBuffers/d3d/GstHwBuffersD3DTest.h"

#include <algorithm>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <thread>

#include <QtCore/QScopeGuard>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"
#include "GstD3D11ContextBridge.h"
#include "GstD3D12ContextBridge.h"
#include "GstHwPathRegistry.h"
#include "GstHwVideoBuffer.h"
#include "HwBuffers/GstHwImageReadback.h"
#include "QGCRhiCapture.h"
#include "gstqgc/GstQgcFrameMap.h"
#include "gstqgc/GstQgcTestSupport.h"

namespace {

/// A QRhi on @p api, on the software adapter (WARP) when QSG_RHI_PREFER_SOFTWARE_RENDERER is set, as it is in CI, or
/// when DXGI lists no hardware adapter, as on a runner without a GPU.
QRhi* createD3DRhi(QRhi::Implementation api, QRhiInitParams* params)
{
    bool software = qEnvironmentVariableIntValue("QSG_RHI_PREFER_SOFTWARE_RENDERER") != 0;
    if (!software) {
        const QRhi::AdapterList adapters = QRhi::enumerateAdapters(api, params);
        software = std::ranges::none_of(adapters, [](const QRhiAdapter* adapter) {
            return adapter->info().deviceType != QRhiDriverInfo::CpuDevice;
        });
        qDeleteAll(adapters);
    }
    return QRhi::create(api, params, software ? QRhi::Flags(QRhi::PreferSoftwareRenderer) : QRhi::Flags());
}

/// A QRhi on @p api, published in QGCRhiCapture's device snapshot as the render thread publishes it. Null, with
/// @p failure set, when this runner can't create one.
std::unique_ptr<QRhi> createPublishedRhi(QRhi::Implementation api, QString& failure)
{
    std::unique_ptr<QRhi> rhi;
    switch (api) {
        case QRhi::D3D11: {
            QRhiD3D11InitParams params;
            rhi.reset(createD3DRhi(QRhi::D3D11, &params));
            break;
        }
        case QRhi::D3D12: {
            QRhiD3D12InitParams params;
            rhi.reset(createD3DRhi(QRhi::D3D12, &params));
            break;
        }
        default:
            break;
    }
    if (!rhi) {
        failure = QStringLiteral("Could not create QRhi");
        return {};
    }

    QGCRhiCapture::publishDeviceSnapshotForTest(rhi.get());
    // A Direct3D 12 QRhi always has a device; Qt's Direct3D 11 one is published only once its context is protected.
    if ((api == QRhi::D3D11) && !QGCRhiCapture::deviceSnapshot().d3d11Device.load(std::memory_order_acquire)) {
        failure = QStringLiteral("QRhi exposes no native device handle");
        return {};
    }
    return rhi;
}

struct SnapshotGuard
{
    SnapshotGuard()
        : backend(QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire))
        , d3d11Device(QGCRhiCapture::deviceSnapshot().d3d11Device.load(std::memory_order_acquire))
        , adapterLuid(QGCRhiCapture::deviceSnapshot().adapterLuid.load(std::memory_order_acquire))
        , d3d11OpensD3D12(QGCRhiCapture::deviceSnapshot().d3d11OpensD3D12.load(std::memory_order_acquire))
    {}

    ~SnapshotGuard()
    {
        QGCRhiCapture::deviceSnapshot().d3d11Device.store(d3d11Device, std::memory_order_release);
        QGCRhiCapture::deviceSnapshot().adapterLuid.store(adapterLuid, std::memory_order_release);
        QGCRhiCapture::deviceSnapshot().d3d11OpensD3D12.store(d3d11OpensD3D12, std::memory_order_release);
        QGCRhiCapture::deviceSnapshot().backend.store(backend, std::memory_order_release);
    }

    int backend = -1;
    void* d3d11Device = nullptr;
    qint64 adapterLuid = 0;
    bool d3d11OpensD3D12 = false;
};

/// The device @p path's context bridge shares now; null when it isn't primed.
GStreamer::GstObjectPtr bridgeDevice(HwVideoBufferPath path)
{
    GstObject* device = nullptr;
    switch (path) {
        case HwVideoBufferPath::D3D11:
            device = GST_OBJECT_CAST(GstD3D11ContextBridge::currentDevice());
            break;
        case HwVideoBufferPath::D3D12:
            device = GST_OBJECT_CAST(GstD3D12ContextBridge::currentDevice());
            break;
        default:
            break;
    }
    return GStreamer::adoptObject(device);
}

struct D3DSample
{
    GstTest::TestPipeline pipeline;
    GStreamer::GstElementPtr sink;
    GstSample* sample = nullptr;
    GstVideoInfo info;

    ~D3DSample()
    {
        if (sample) {
            gst_sample_unref(sample);
        }
    }
};

/// @p path's context bridge; null for a path that isn't a Direct3D one.
GstContextBridgeBase* d3dContextBridge(HwVideoBufferPath path)
{
    if (path == HwVideoBufferPath::D3D11) {
        return &GstD3D11ContextBridge::instance();
    }
    if (path == HwVideoBufferPath::D3D12) {
        return &GstD3D12ContextBridge::instance();
    }
    return nullptr;
}

void pullD3DSample(D3DSample& result, const char* inputCaps, const char* uploadElement, const char* capsFilter,
                   HwVideoBufferPath bridgePath = HwVideoBufferPath::None)
{
    GSTTEST_SKIP_UNLESS_FACTORIES(uploadElement);

    const QString launch = QStringLiteral(
                               "videotestsrc num-buffers=1 ! "
                               "%1 ! "
                               "%2 ! %3 ! appsink name=sink sync=false")
                               .arg(QString::fromUtf8(inputCaps))
                               .arg(QString::fromUtf8(uploadElement))
                               .arg(QString::fromUtf8(capsFilter));
    if (!result.pipeline.launch(launch)) {
        QSKIP(qPrintable(QStringLiteral("D3D pipeline parse skipped: %1").arg(result.pipeline.error())));
    }

    result.sink = result.pipeline.element("sink");
    QVERIFY2(result.sink, "Could not find appsink");

    if (GstContextBridgeBase* const bridge = d3dContextBridge(bridgePath)) {
        bridge->provideContexts(result.pipeline.get());
    }

    if (!result.pipeline.play()) {
        QSKIP("D3D pipeline failed to PLAY");
    }

    GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(result.sink.get()),
                                                     GstTest::clockTime(TestTimeout::mediumDuration()));
    if (!sample) {
        QSKIP("D3D pipeline produced no sample");
    }
    result.sample = sample;

    gst_video_info_init(&result.info);
    QVERIFY2(gst_video_info_from_caps(&result.info, gst_sample_get_caps(sample)), "Could not parse D3D sample caps");
}

void testD3DMemoryDispatch(const char* uploadElement, const char* capsFilter, HwVideoBufferPath expectedPath)
{
    D3DSample sample;
    pullD3DSample(sample, "video/x-raw,format=RGBA,width=64,height=64,framerate=30/1", uploadElement, capsFilter);
    if (!sample.sample) {
        return;
    }

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwResolvedPathCache cache;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&sample.info), GST_VIDEO_INFO_HEIGHT(&sample.info)),
                             QVideoFrameFormat::Format_RGBA8888);

    auto buffer = makeHwVideoBuffer(gst_sample_get_buffer(sample.sample), gst_sample_get_caps(sample.sample),
                                    sample.info, format, context, path, &cache);

    QVERIFY2(buffer, "D3D memory sample did not create a hardware video buffer");
    QCOMPARE(path, expectedPath);
    QVERIFY(cache.validated);
    QCOMPARE(cache.path, expectedPath);
}

void testD3DMapTextures(QRhi& rhi, const char* inputCaps, const char* uploadElement, const char* capsFilter,
                        QVideoFrameFormat::PixelFormat pixelFormat, int expectedPlanes, HwVideoBufferPath expectedPath)
{
    D3DSample sample;
    pullD3DSample(sample, inputCaps, uploadElement, capsFilter, expectedPath);
    if (!sample.sample) {
        return;
    }

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwResolvedPathCache cache;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&sample.info), GST_VIDEO_INFO_HEIGHT(&sample.info)),
                             pixelFormat);

    auto buffer = makeHwVideoBuffer(gst_sample_get_buffer(sample.sample), gst_sample_get_caps(sample.sample),
                                    sample.info, format, context, path, &cache);

    QVERIFY2(buffer, "D3D memory sample did not create a hardware video buffer");
    QCOMPARE(path, expectedPath);

    QVideoFrameTexturesUPtr oldTextures;
    QVideoFrameTexturesUPtr textures = buffer->mapTextures(rhi, oldTextures);
    QVERIFY2(textures, "D3D hardware video buffer did not import into the active QRhi");
    for (int plane = 0; plane < expectedPlanes; ++plane) {
        QVERIFY2(textures->texture(static_cast<uint>(plane)),
                 qPrintable(QStringLiteral("D3D QRhi texture bundle has no plane %1 texture").arg(plane)));
    }
    QVERIFY2(textures->takeHandles(), "D3D textures are not Qt's wraps of the frame's native handles");
}

/// The caps filter after the upload: @p api's memory in @p outputFormat, converted by d3d12convert first when it
/// differs from @p format.
QByteArray d3dMemoryCaps(const QString& api, const QString& format, const QString& outputFormat)
{
    QByteArray caps = QStringLiteral("video/x-raw(memory:%1Memory),format=%2").arg(api, outputFormat).toUtf8();
    if (outputFormat != format) {
        caps.prepend(QByteArrayLiteral("d3d12convert ! "));
    }
    return caps;
}

/// Converts the uploaded frame with QVideoFrame::toImage() on @p rhi and compares it with the frame before the upload,
/// passed through @p cpuElement into @p cpuCaps. @p subsampledInput: the source has one chroma sample per 2x2 pixels,
/// which the GPU interpolates up to two pixels into a neighbor bar.
void testD3DFrameToImage(QRhi& rhi, const char* inputCaps, const char* uploadElement, const char* capsFilter,
                         const char* cpuElement, const char* cpuCaps, QVideoFrameFormat::PixelFormat pixelFormat,
                         bool subsampledInput, HwVideoBufferPath expectedPath)
{
    D3DSample gpu;
    pullD3DSample(gpu, inputCaps, uploadElement, capsFilter, expectedPath);
    if (!gpu.sample) {
        return;
    }
    D3DSample cpu;
    pullD3DSample(cpu, inputCaps, cpuElement, cpuCaps);
    if (!cpu.sample) {
        return;
    }

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    const QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&gpu.info), GST_VIDEO_INFO_HEIGHT(&gpu.info)),
                                   pixelFormat);
    std::unique_ptr<QHwVideoBuffer> buffer = makeHwVideoBuffer(
        gst_sample_get_buffer(gpu.sample), gst_sample_get_caps(gpu.sample), gpu.info, format, context, path);
    QVERIFY2(buffer, "D3D memory sample did not create a hardware video buffer");
    QCOMPARE(path, expectedPath);

    const QImage actual =
        QVideoFrame(std::make_unique<GstTest::RhiBoundVideoBuffer>(std::move(buffer), &rhi)).toImage();
    const QImage expected = GstTest::cpuImage(mapCpuFrame(gst_sample_get_buffer(cpu.sample), cpu.info, format));
    const int chromaEdgeRadius = subsampledInput ? 2 : 0;
    const QString mismatch = GstTest::firstImageMismatch(actual, expected, /*tolerance=*/8, chromaEdgeRadius);
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
}

}  // namespace

void GstHwBuffersD3DTest::_testD3D11MemoryDispatch()
{
    testD3DMemoryDispatch("d3d11upload", "video/x-raw(memory:D3D11Memory)", HwVideoBufferPath::D3D11);
}

void GstHwBuffersD3DTest::_testD3D12MemoryDispatch()
{
    testD3DMemoryDispatch("d3d12upload", "video/x-raw(memory:D3D12Memory)", HwVideoBufferPath::D3D12);
}

void GstHwBuffersD3DTest::_testD3DMapTexturesWithQRhi_data()
{
    QTest::addColumn<int>("path");
    QTest::addColumn<int>("rhiApi");            ///< What Qt renders with.
    QTest::addColumn<QString>("format");        ///< Uploaded.
    QTest::addColumn<QString>("outputFormat");  ///< Imported.
    QTest::addColumn<int>("pixelFormat");
    QTest::addColumn<int>("planes");

    const QString rgba = QStringLiteral("RGBA");
    const QString nv12 = QStringLiteral("NV12");
    const int rgba8888 = int(QVideoFrameFormat::Format_RGBA8888);
    const int nv12Frame = int(QVideoFrameFormat::Format_NV12);
    const int d3d11 = int(HwVideoBufferPath::D3D11);
    const int d3d12 = int(HwVideoBufferPath::D3D12);
    QTest::newRow("d3d11-rgba") << d3d11 << int(QRhi::D3D11) << rgba << rgba << rgba8888 << 1;
    QTest::newRow("d3d11-nv12") << d3d11 << int(QRhi::D3D11) << nv12 << nv12 << nv12Frame << 2;
    QTest::newRow("d3d12-rgba") << d3d12 << int(QRhi::D3D12) << rgba << rgba << rgba8888 << 1;
    // As the D3D12Convert bin does: Qt's D3D12 QRhi can't sample NV12's planes, so d3d12convert makes it RGBA.
    QTest::newRow("d3d12-nv12") << d3d12 << int(QRhi::D3D12) << nv12 << rgba << rgba8888 << 1;
    // A Direct3D 11 QRhi samples the texture a D3D12 resource opens as on its device, planes and all.
    QTest::newRow("d3d12-rgba-on-d3d11") << d3d12 << int(QRhi::D3D11) << rgba << rgba << rgba8888 << 1;
    QTest::newRow("d3d12-nv12-on-d3d11") << d3d12 << int(QRhi::D3D11) << nv12 << nv12 << nv12Frame << 2;
}

void GstHwBuffersD3DTest::_testD3DMapTexturesWithQRhi()
{
    QFETCH(int, path);
    QFETCH(int, rhiApi);
    QFETCH(QString, format);
    QFETCH(QString, outputFormat);
    QFETCH(int, pixelFormat);
    QFETCH(int, planes);

    const auto hwPath = static_cast<HwVideoBufferPath>(path);
    const bool d3d11 = (hwPath == HwVideoBufferPath::D3D11);
    GstContextBridgeBase* const bridge = d3dContextBridge(hwPath);
    QVERIFY(bridge);

    const SnapshotGuard snapshotGuard;
    // A runner whose WARP adapter can't create this device skips the test; QRhi warns first.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("^Failed to create D3D1[12] device")));
    QString failure;
    const std::unique_ptr<QRhi> rhi = createPublishedRhi(static_cast<QRhi::Implementation>(rhiApi), failure);
    if (!rhi) {
        QSKIP(qPrintable(failure));
    }
    if (!d3d11 && (rhiApi == int(QRhi::D3D11)) &&
        !QGCRhiCapture::deviceSnapshot().d3d11OpensD3D12.load(std::memory_order_acquire)) {
        QSKIP("This Direct3D 11 device doesn't open Direct3D 12 resources");
    }

    bridge->reset();
    const auto bridgeGuard = qScopeGuard([bridge] { bridge->reset(); });
    QVERIFY2(bridge->prime(), "Could not prime the D3D context bridge from the QRhi snapshot");

    const QString api = d3d11 ? QStringLiteral("D3D11") : QStringLiteral("D3D12");
    const QByteArray inputCaps =
        QStringLiteral("video/x-raw,format=%1,width=64,height=64,framerate=30/1").arg(format).toUtf8();
    const QByteArray upload = (api.toLower() + QStringLiteral("upload")).toUtf8();
    const QByteArray memoryCaps = d3dMemoryCaps(api, format, outputFormat);
    testD3DMapTextures(*rhi, inputCaps.constData(), upload.constData(), memoryCaps.constData(),
                       static_cast<QVideoFrameFormat::PixelFormat>(pixelFormat), planes, hwPath);
}

void GstHwBuffersD3DTest::_testD3DFrameToImage_data()
{
    _testD3DMapTexturesWithQRhi_data();
}

void GstHwBuffersD3DTest::_testD3DFrameToImage()
{
    QFETCH(int, path);
    QFETCH(int, rhiApi);
    QFETCH(QString, format);
    QFETCH(QString, outputFormat);
    QFETCH(int, pixelFormat);

    const auto hwPath = static_cast<HwVideoBufferPath>(path);
    const bool d3d11 = (hwPath == HwVideoBufferPath::D3D11);
    GstContextBridgeBase* const bridge = d3dContextBridge(hwPath);
    QVERIFY(bridge);

    const SnapshotGuard snapshotGuard;
    // A runner whose WARP adapter can't create this device skips the test; QRhi warns first.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("^Failed to create D3D1[12] device")));
    QString failure;
    const std::unique_ptr<QRhi> rhi = createPublishedRhi(static_cast<QRhi::Implementation>(rhiApi), failure);
    if (!rhi) {
        QSKIP(qPrintable(failure));
    }
    if (!d3d11 && (rhiApi == int(QRhi::D3D11)) &&
        !QGCRhiCapture::deviceSnapshot().d3d11OpensD3D12.load(std::memory_order_acquire)) {
        QSKIP("This Direct3D 11 device doesn't open Direct3D 12 resources");
    }
    // WARP says it does, and then opens an NV12 resource with its luma plane only. No decoder runs on WARP.
    if (!d3d11 && (rhiApi == int(QRhi::D3D11)) && (format == QStringLiteral("NV12")) &&
        (rhi->driverInfo().deviceType == QRhiDriverInfo::CpuDevice)) {
        QSKIP("WARP opens a D3D12 device's NV12 resource without its chroma plane");
    }

    bridge->reset();
    const auto bridgeGuard = qScopeGuard([bridge] { bridge->reset(); });
    QVERIFY2(bridge->prime(), "Could not prime the D3D context bridge from the QRhi snapshot");

    // An SMPTE pattern: a frame sampled with the wrong pitch, plane offset or padding shows in its bars.
    const QString api = d3d11 ? QStringLiteral("D3D11") : QStringLiteral("D3D12");
    const QByteArray inputCaps =
        QStringLiteral("video/x-raw,format=%1,width=64,height=64,framerate=30/1").arg(format).toUtf8();
    const QByteArray upload = (api.toLower() + QStringLiteral("upload")).toUtf8();
    const QByteArray memoryCaps = d3dMemoryCaps(api, format, outputFormat);
    // A converted frame's reference is GStreamer's CPU conversion of the same frame.
    const QByteArray cpuCaps = QStringLiteral("video/x-raw,format=%1").arg(outputFormat).toUtf8();
    const bool converted = (outputFormat != format);
    testD3DFrameToImage(
        *rhi, inputCaps.constData(), upload.constData(), memoryCaps.constData(),
        converted ? "videoconvert" : "identity", converted ? cpuCaps.constData() : inputCaps.constData(),
        static_cast<QVideoFrameFormat::PixelFormat>(pixelFormat), (format == QStringLiteral("NV12")), hwPath);
}

void GstHwBuffersD3DTest::_testD3DBridgeDropsRemovedDevice_data()
{
    QTest::addColumn<int>("path");

    QTest::newRow("d3d11") << int(HwVideoBufferPath::D3D11);
    QTest::newRow("d3d12") << int(HwVideoBufferPath::D3D12);
}

void GstHwBuffersD3DTest::_testD3DBridgeDropsRemovedDevice()
{
    QFETCH(int, path);

    const auto hwPath = static_cast<HwVideoBufferPath>(path);
    const bool d3d11 = (hwPath == HwVideoBufferPath::D3D11);
    GstContextBridgeBase* const bridge = d3dContextBridge(hwPath);
    QVERIFY(bridge);

    const SnapshotGuard snapshotGuard;
    // A runner whose WARP adapter can't create this device skips the test; QRhi warns first.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("^Failed to create D3D1[12] device")));
    QString failure;
    const std::unique_ptr<QRhi> rhi = createPublishedRhi(d3d11 ? QRhi::D3D11 : QRhi::D3D12, failure);
    if (!rhi) {
        QSKIP(qPrintable(failure));
    }

    bridge->reset();
    const auto bridgeGuard = qScopeGuard([bridge] { bridge->reset(); });
    QVERIFY2(bridge->prime(), "Could not prime the D3D context bridge from the QRhi snapshot");

    const GStreamer::GstObjectPtr device = bridgeDevice(hwPath);
    QVERIFY(device);
    if (!g_object_class_find_property(G_OBJECT_GET_CLASS(device.get()), "device-removed-reason")) {
        QSKIP("This GStreamer's D3D devices don't report their removal (they do from 1.26)");
    }

    // As GStreamer reports a removed device: on a thread of its own.
    std::thread([&device] { g_object_notify(G_OBJECT(device.get()), "device-removed-reason"); }).join();
    QTRY_VERIFY2_WITH_TIMEOUT(!bridgeDevice(hwPath), "The bridge still shares the device GStreamer reported removed",
                              TestTimeout::mediumMs());
}

void GstHwBuffersD3DTest::_testWindowsGpuSinkPreservesSoftwarePixels()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc");
    // Mapping the center pixel converts on the CPU when Qt Multimedia finds no RHI for this thread, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVideoSink videoSink;
    const auto result = GstQgcTest::runPipelineThroughQVideoSink(
        videoSink, "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1",
        /*numBuffers*/ 5, {.gpuZeroCopy = true}, "pattern=red",
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
}

void GstHwBuffersD3DTest::_testWindowsGpuSinkPreservesDirectD3D11Memory()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc", "d3d11upload");

    QVideoSink videoSink;
    auto result = GstQgcTest::runPipelineThroughQVideoSink(
        videoSink,
        "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! d3d11upload ! "
        "video/x-raw(memory:D3D11Memory),format=NV12",
        /*numBuffers*/ 5, {.gpuZeroCopy = true});
    if (!result.eos && result.errorMessage.contains(QStringLiteral("d3d11"), Qt::CaseInsensitive)) {
        QSKIP(qPrintable(QStringLiteral("D3D11 device unavailable in this runner: %1").arg(result.errorMessage)));
    }

    QVERIFY2(result.eos, qUtf8Printable(QStringLiteral("D3D11 pipeline: %1").arg(result.errorMessage)));
    QVERIFY(result.frameCount > 0);
    QCOMPARE(result.lastFormat.pixelFormat(), QVideoFrameFormat::Format_NV12);
}

UT_REGISTER_TEST(GstHwBuffersD3DTest, TestLabel::Integration)
