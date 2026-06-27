#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QScopeGuard>
#include <QtMultimedia/QVideoFrameFormat>
#include <memory>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video-info.h>

#include "GStreamer.h"
#include "GStreamerLogging.h"
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
    GstElement* pipeline = nullptr;
    GstElement* sink = nullptr;
    GstSample* sample = nullptr;
    GstVideoInfo info;

    ~D3DSample()
    {
        if (sample) {
            gst_sample_unref(sample);
        }
        if (sink) {
            gst_object_unref(sink);
        }
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }
    }
};

void pullD3DSample(D3DSample& result, const char* inputCaps, const char* uploadElement, const char* capsFilter,
                   HwVideoBufferPath bridgePath = HwVideoBufferPath::None)
{
    GstElementFactory* uploadFactory = gst_element_factory_find(uploadElement);
    if (!uploadFactory) {
        QSKIP(qPrintable(QStringLiteral("%1 factory unavailable").arg(QString::fromUtf8(uploadElement))));
    }
    gst_object_unref(uploadFactory);

    const QString launch = QStringLiteral(
                               "videotestsrc num-buffers=1 ! "
                               "%1 ! "
                               "%2 ! %3 ! appsink name=sink sync=false")
                               .arg(QString::fromUtf8(inputCaps))
                               .arg(QString::fromUtf8(uploadElement))
                               .arg(QString::fromUtf8(capsFilter));

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(launch.toUtf8().constData(), &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QSKIP(qPrintable(QStringLiteral("D3D pipeline parse skipped: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create D3D dispatch test pipeline");

    result.pipeline = pipeline;

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sink, "Could not find appsink");
    result.sink = sink;

    if (bridgePath != HwVideoBufferPath::None) {
        GstBus* bus = gst_element_get_bus(pipeline);
        QVERIFY2(bus, "Could not get D3D test pipeline bus");
        gst_bus_set_sync_handler(
            bus,
            [](GstBus*, GstMessage* message, gpointer userData) -> GstBusSyncReply {
                const HwVideoBufferPath path = static_cast<HwVideoBufferPath>(GPOINTER_TO_INT(userData));
                switch (path) {
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
                    case HwVideoBufferPath::D3D11:
                        return GstD3D11ContextBridge::handleSyncMessage(message);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
                    case HwVideoBufferPath::D3D12:
                        return GstD3D12ContextBridge::handleSyncMessage(message);
#endif
                    default:
                        break;
                }
                return GST_BUS_PASS;
            },
            GINT_TO_POINTER(static_cast<int>(bridgePath)), nullptr);
        gst_object_unref(bus);
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        QSKIP("D3D pipeline failed to PLAY");
    }

    GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 5 * GST_SECOND);
    if (!sample) {
        QSKIP("D3D pipeline produced no sample");
    }
    result.sample = sample;

    gst_video_info_init(&result.info);
    QVERIFY2(gst_video_info_from_caps(&result.info, gst_sample_get_caps(sample)), "Could not parse D3D sample caps");
}

void testD3DMemoryDispatch(const char* uploadElement, const char* capsFilter, HwVideoBufferPath expectedPath)
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    D3DSample sample;
    pullD3DSample(sample, "video/x-raw,format=RGBA,width=64,height=64,framerate=30/1", uploadElement, capsFilter);

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwResolvedPathCache cache;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&sample.info), GST_VIDEO_INFO_HEIGHT(&sample.info)),
                             QVideoFrameFormat::Format_RGBA8888);

    auto buffer = makeHwVideoBuffer(sample.sample, sample.info, format, context, path, &cache);

    QVERIFY2(buffer, "D3D memory sample did not create a hardware video buffer");
    QCOMPARE(path, expectedPath);
    QVERIFY(cache.validated);
    QCOMPARE(cache.path, expectedPath);
}

void testD3DMapTextures(QRhi& rhi, const char* inputCaps, const char* uploadElement, const char* capsFilter,
                        QVideoFrameFormat::PixelFormat pixelFormat, int expectedPlanes, HwVideoBufferPath expectedPath)
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    D3DSample sample;
    pullD3DSample(sample, inputCaps, uploadElement, capsFilter, expectedPath);

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwResolvedPathCache cache;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&sample.info), GST_VIDEO_INFO_HEIGHT(&sample.info)),
                             pixelFormat);

    auto buffer = makeHwVideoBuffer(sample.sample, sample.info, format, context, path, &cache);

    QVERIFY2(buffer, "D3D memory sample did not create a hardware video buffer");
    QCOMPARE(path, expectedPath);

    QVideoFrameTexturesUPtr oldTextures;
    QVideoFrameTexturesUPtr textures = buffer->mapTextures(rhi, oldTextures);
    QVERIFY2(textures, "D3D hardware video buffer did not import into the active QRhi");
    for (int plane = 0; plane < expectedPlanes; ++plane) {
        QVERIFY2(textures->texture(static_cast<uint>(plane)),
                 qPrintable(QStringLiteral("D3D QRhi texture bundle has no plane %1 texture").arg(plane)));
    }
    auto* base = dynamic_cast<GstHwFrameTexturesBase*>(textures.get());
    QVERIFY2(base, "D3D texture bundle does not use the HW frame texture base");
    QCOMPARE(base->sourcePath(), expectedPath);
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

void GStreamerTest::_testD3D11MapTexturesWithQRhi()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QRhiD3D11InitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::D3D11, &params));
    if (!rhi) {
        QSKIP("Could not create D3D11 QRhi");
    }
    auto* handles = static_cast<const QRhiD3D11NativeHandles*>(rhi->nativeHandles());
    if (!handles || !handles->dev) {
        QSKIP("D3D11 QRhi exposes no native device handle");
    }

    SnapshotGuard snapshotGuard;
    Q_UNUSED(snapshotGuard)
    QGCRhiCapture::deviceSnapshot().d3d11Device.store(handles->dev, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().d3d12Device.store(nullptr, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().adapterLuid.store(composeLuid(handles->adapterLuidHigh, handles->adapterLuidLow),
                                                      std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().backend.store(static_cast<int>(QRhi::D3D11), std::memory_order_release);

    GstD3D11ContextBridge::reset();
    auto bridgeGuard = qScopeGuard([] { GstD3D11ContextBridge::reset(); });
    QVERIFY2(GstD3D11ContextBridge::prime(), "Could not prime D3D11 context bridge from QRhi snapshot");

    testD3DMapTextures(*rhi, "video/x-raw,format=RGBA,width=64,height=64,framerate=30/1", "d3d11upload",
                       "video/x-raw(memory:D3D11Memory)", QVideoFrameFormat::Format_RGBA8888, 1,
                       HwVideoBufferPath::D3D11);
#else
    QSKIP("D3D11 GPU path not compiled in this build");
#endif
}

void GStreamerTest::_testD3D11MapNv12TexturesWithQRhi()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QRhiD3D11InitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::D3D11, &params));
    if (!rhi) {
        QSKIP("Could not create D3D11 QRhi");
    }
    auto* handles = static_cast<const QRhiD3D11NativeHandles*>(rhi->nativeHandles());
    if (!handles || !handles->dev) {
        QSKIP("D3D11 QRhi exposes no native device handle");
    }

    SnapshotGuard snapshotGuard;
    Q_UNUSED(snapshotGuard)
    QGCRhiCapture::deviceSnapshot().d3d11Device.store(handles->dev, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().d3d12Device.store(nullptr, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().adapterLuid.store(composeLuid(handles->adapterLuidHigh, handles->adapterLuidLow),
                                                      std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().backend.store(static_cast<int>(QRhi::D3D11), std::memory_order_release);

    GstD3D11ContextBridge::reset();
    auto bridgeGuard = qScopeGuard([] { GstD3D11ContextBridge::reset(); });
    QVERIFY2(GstD3D11ContextBridge::prime(), "Could not prime D3D11 context bridge from QRhi snapshot");

    testD3DMapTextures(*rhi, "video/x-raw,format=NV12,width=64,height=64,framerate=30/1", "d3d11upload",
                       "video/x-raw(memory:D3D11Memory),format=NV12", QVideoFrameFormat::Format_NV12, 2,
                       HwVideoBufferPath::D3D11);
#else
    QSKIP("D3D11 GPU path not compiled in this build");
#endif
}

void GStreamerTest::_testD3D12MapTexturesWithQRhi()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QRhiD3D12InitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::D3D12, &params));
    if (!rhi) {
        QSKIP("Could not create D3D12 QRhi");
    }
    auto* handles = static_cast<const QRhiD3D12NativeHandles*>(rhi->nativeHandles());
    if (!handles || !handles->dev) {
        QSKIP("D3D12 QRhi exposes no native device handle");
    }

    SnapshotGuard snapshotGuard;
    Q_UNUSED(snapshotGuard)
    QGCRhiCapture::deviceSnapshot().d3d11Device.store(nullptr, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().d3d12Device.store(handles->dev, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().adapterLuid.store(composeLuid(handles->adapterLuidHigh, handles->adapterLuidLow),
                                                      std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().backend.store(static_cast<int>(QRhi::D3D12), std::memory_order_release);

    GstD3D12ContextBridge::reset();
    auto bridgeGuard = qScopeGuard([] { GstD3D12ContextBridge::reset(); });
    QVERIFY2(GstD3D12ContextBridge::prime(), "Could not prime D3D12 context bridge from QRhi snapshot");

    testD3DMapTextures(*rhi, "video/x-raw,format=RGBA,width=64,height=64,framerate=30/1", "d3d12upload",
                       "video/x-raw(memory:D3D12Memory)", QVideoFrameFormat::Format_RGBA8888, 1,
                       HwVideoBufferPath::D3D12);
#else
    QSKIP("D3D12 GPU path not compiled in this build");
#endif
}

void GStreamerTest::_testD3D12MapNv12TexturesWithQRhi()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QRhiD3D12InitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::D3D12, &params));
    if (!rhi) {
        QSKIP("Could not create D3D12 QRhi");
    }
    auto* handles = static_cast<const QRhiD3D12NativeHandles*>(rhi->nativeHandles());
    if (!handles || !handles->dev) {
        QSKIP("D3D12 QRhi exposes no native device handle");
    }

    SnapshotGuard snapshotGuard;
    Q_UNUSED(snapshotGuard)
    QGCRhiCapture::deviceSnapshot().d3d11Device.store(nullptr, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().d3d12Device.store(handles->dev, std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().adapterLuid.store(composeLuid(handles->adapterLuidHigh, handles->adapterLuidLow),
                                                      std::memory_order_release);
    QGCRhiCapture::deviceSnapshot().backend.store(static_cast<int>(QRhi::D3D12), std::memory_order_release);

    GstD3D12ContextBridge::reset();
    auto bridgeGuard = qScopeGuard([] { GstD3D12ContextBridge::reset(); });
    QVERIFY2(GstD3D12ContextBridge::prime(), "Could not prime D3D12 context bridge from QRhi snapshot");

    testD3DMapTextures(*rhi, "video/x-raw,format=NV12,width=64,height=64,framerate=30/1", "d3d12upload",
                       "video/x-raw(memory:D3D12Memory),format=NV12", QVideoFrameFormat::Format_NV12, 2,
                       HwVideoBufferPath::D3D12);
#else
    QSKIP("D3D12 GPU path not compiled in this build");
#endif
}

#endif
