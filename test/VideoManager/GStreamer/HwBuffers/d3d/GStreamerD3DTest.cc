#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include <QtCore/QScopeGuard>
#include <QtMultimedia/QVideoFrameFormat>

#include "GStreamer.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

#include "QGCRhiCapture.h"
#include "GstHwFrameTexturesBase.h"

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include "GstD3D11ContextBridge.h"
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include "GstD3D12ContextBridge.h"
#endif

namespace {

qint64 composeLuid(qint32 high, quint32 low)
{
    return (static_cast<qint64>(high) << 32) | (static_cast<qint64>(low) & 0xFFFFFFFFLL);
}

/// A QRhi on @p api, published in QGCRhiCapture's device snapshot as the render thread publishes it. Null, with
/// @p failure set, when this runner can't create one.
std::unique_ptr<QRhi> createPublishedRhi(QRhi::Implementation api, QString& failure)
{
    std::unique_ptr<QRhi> rhi;
    void* device = nullptr;
    qint64 adapterLuid = 0;
    switch (api) {
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        case QRhi::D3D11: {
            QRhiD3D11InitParams params;
            rhi.reset(QRhi::create(QRhi::D3D11, &params));
            if (const auto* handles =
                    rhi ? static_cast<const QRhiD3D11NativeHandles*>(rhi->nativeHandles()) : nullptr) {
                device = handles->dev;
                adapterLuid = composeLuid(handles->adapterLuidHigh, handles->adapterLuidLow);
            }
            break;
        }
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        case QRhi::D3D12: {
            QRhiD3D12InitParams params;
            rhi.reset(QRhi::create(QRhi::D3D12, &params));
            if (const auto* handles =
                    rhi ? static_cast<const QRhiD3D12NativeHandles*>(rhi->nativeHandles()) : nullptr) {
                device = handles->dev;
                adapterLuid = composeLuid(handles->adapterLuidHigh, handles->adapterLuidLow);
            }
            break;
        }
#endif
        default:
            break;
    }
    if (!rhi || !device) {
        failure =
            rhi ? QStringLiteral("QRhi exposes no native device handle") : QStringLiteral("Could not create QRhi");
        return {};
    }

    QGCRhiCapture::DeviceSnapshot& snapshot = QGCRhiCapture::deviceSnapshot();
    snapshot.d3d11Device.store((api == QRhi::D3D11) ? device : nullptr, std::memory_order_release);
    snapshot.d3d12Device.store((api == QRhi::D3D12) ? device : nullptr, std::memory_order_release);
    snapshot.adapterLuid.store(adapterLuid, std::memory_order_release);
    snapshot.backend.store(static_cast<int>(api), std::memory_order_release);
    return rhi;
}

struct SnapshotGuard
{
    SnapshotGuard()
        : backend(QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire)),
          d3d11Device(QGCRhiCapture::deviceSnapshot().d3d11Device.load(std::memory_order_acquire)),
          d3d12Device(QGCRhiCapture::deviceSnapshot().d3d12Device.load(std::memory_order_acquire)),
          adapterLuid(QGCRhiCapture::deviceSnapshot().adapterLuid.load(std::memory_order_acquire))
    {}

    ~SnapshotGuard()
    {
        QGCRhiCapture::deviceSnapshot().d3d11Device.store(d3d11Device, std::memory_order_release);
        QGCRhiCapture::deviceSnapshot().d3d12Device.store(d3d12Device, std::memory_order_release);
        QGCRhiCapture::deviceSnapshot().adapterLuid.store(adapterLuid, std::memory_order_release);
        QGCRhiCapture::deviceSnapshot().backend.store(backend, std::memory_order_release);
    }

    int backend = -1;
    void* d3d11Device = nullptr;
    void* d3d12Device = nullptr;
    qint64 adapterLuid = 0;
};

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

void pullD3DSample(D3DSample& result, const char* inputCaps, const char* uploadElement, const char* capsFilter,
                   HwVideoBufferPath bridgePath = HwVideoBufferPath::None)
{
    if (!GstTest::firstMissingFactory({uploadElement}).isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 factory unavailable").arg(QString::fromUtf8(uploadElement))));
    }

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

    if (bridgePath != HwVideoBufferPath::None) {
        const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(result.pipeline.get()));
        QVERIFY2(bus, "Could not get D3D test pipeline bus");
        gst_bus_set_sync_handler(
            bus.get(),
            [](GstBus*, GstMessage* message, gpointer userData) -> GstBusSyncReply {
                const HwVideoBufferPath path = static_cast<HwVideoBufferPath>(GPOINTER_TO_INT(userData));
                switch (path) {
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
                    case HwVideoBufferPath::D3D11:
                        return GstD3D11ContextBridge::instance().handleSyncMessage(message);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
                    case HwVideoBufferPath::D3D12:
                        return GstD3D12ContextBridge::instance().handleSyncMessage(message);
#endif
                    default:
                        break;
                }
                return GST_BUS_PASS;
            },
            GINT_TO_POINTER(static_cast<int>(bridgePath)), nullptr);
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
    QVERIFY2(dynamic_cast<GstHwFrameTexturesBase*>(textures.get()),
             "D3D texture bundle does not use the HW frame texture base");
}

} // namespace
#endif

void GStreamerTest::_testD3D11MemoryDispatch()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    testD3DMemoryDispatch("d3d11upload", "video/x-raw(memory:D3D11Memory)", HwVideoBufferPath::D3D11);
#else
    QSKIP("D3D11 GPU path not compiled in this build");
#endif
}

void GStreamerTest::_testD3D12MemoryDispatch()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    testD3DMemoryDispatch("d3d12upload", "video/x-raw(memory:D3D12Memory)", HwVideoBufferPath::D3D12);
#else
    QSKIP("D3D12 GPU path not compiled in this build");
#endif
}

void GStreamerTest::_testD3DMapTexturesWithQRhi_data()
{
    QTest::addColumn<int>("path");
    QTest::addColumn<QString>("format");
    QTest::addColumn<int>("pixelFormat");
    QTest::addColumn<int>("planes");

    for (const HwVideoBufferPath path : {HwVideoBufferPath::D3D11, HwVideoBufferPath::D3D12}) {
        const char* const api = (path == HwVideoBufferPath::D3D11) ? "d3d11" : "d3d12";
        QTest::addRow("%s-rgba", api) << int(path) << QStringLiteral("RGBA") << int(QVideoFrameFormat::Format_RGBA8888)
                                      << 1;
        QTest::addRow("%s-nv12", api) << int(path) << QStringLiteral("NV12") << int(QVideoFrameFormat::Format_NV12)
                                      << 2;
    }
}

void GStreamerTest::_testD3DMapTexturesWithQRhi()
{
#if !defined(Q_OS_WIN) || (!defined(QGC_HAS_GST_D3D11_GPU_PATH) && !defined(QGC_HAS_GST_D3D12_GPU_PATH))
    QSKIP("D3D GPU paths not compiled in this build");
#else
    QFETCH(int, path);
    QFETCH(QString, format);
    QFETCH(int, pixelFormat);
    QFETCH(int, planes);

    const auto hwPath = static_cast<HwVideoBufferPath>(path);
    const bool d3d11 = (hwPath == HwVideoBufferPath::D3D11);
    GstContextBridgeBase* bridge = nullptr;
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (d3d11) {
        bridge = &GstD3D11ContextBridge::instance();
    }
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (!d3d11) {
        bridge = &GstD3D12ContextBridge::instance();
    }
#endif
    if (!bridge) {
        QSKIP("This D3D GPU path is not compiled in this build");
    }

    const SnapshotGuard snapshotGuard;
    QString failure;
    const std::unique_ptr<QRhi> rhi = createPublishedRhi(d3d11 ? QRhi::D3D11 : QRhi::D3D12, failure);
    if (!rhi) {
        QSKIP(qPrintable(failure));
    }

    bridge->reset();
    const auto bridgeGuard = qScopeGuard([bridge] { bridge->reset(); });
    QVERIFY2(bridge->prime(), "Could not prime the D3D context bridge from the QRhi snapshot");

    const QString api = d3d11 ? QStringLiteral("D3D11") : QStringLiteral("D3D12");
    const QByteArray inputCaps =
        QStringLiteral("video/x-raw,format=%1,width=64,height=64,framerate=30/1").arg(format).toUtf8();
    const QByteArray upload = (api.toLower() + QStringLiteral("upload")).toUtf8();
    const QByteArray memoryCaps = QStringLiteral("video/x-raw(memory:%1Memory),format=%2").arg(api, format).toUtf8();
    testD3DMapTextures(*rhi, inputCaps.constData(), upload.constData(), memoryCaps.constData(),
                       static_cast<QVideoFrameFormat::PixelFormat>(pixelFormat), planes, hwPath);
#endif
}

#endif
