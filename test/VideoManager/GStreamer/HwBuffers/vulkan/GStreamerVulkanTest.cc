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
#include <string_view>

void GStreamerTest::_testVulkanDispatchDemotesToCpu()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstElementFactory* vkUpload = gst_element_factory_find("vulkanupload");
    if (!vkUpload) {
        QSKIP("vulkanupload factory unavailable — gst-vulkan not registered in this build");
    }
    gst_object_unref(vkUpload);

    QVideoSink videoSink;
    int frameCount = 0;

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=RGBA,width=320,height=240,framerate=30/1 ! "
        "vulkanupload ! "
        "video/x-raw(memory:VulkanImage) ! "
        "qgcvideosinkbin name=sink gpu-zerocopy=true",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QSKIP(qPrintable(QStringLiteral("Vulkan pipeline parse skipped: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create Vulkan test pipeline");
    auto pipelineGuard = qScopeGuard([&] {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    });

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");
    auto sinkBinGuard = qScopeGuard([&] { gst_object_unref(sinkBin); });

    auto owner = std::make_unique<QObject>();
    if (!GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, owner.get())) {
        QSKIP("setupQVideoSinkElement() failed (no GPU context for Vulkan)");
    }

    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, owner.get(),
                     [&frameCount](const QVideoFrame&) { ++frameCount; });

    GstElement* qvideosink = gst_bin_get_by_name(GST_BIN(sinkBin), "qgcqvideosink");
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    auto qvideosinkGuard = qScopeGuard([&] { gst_object_unref(qvideosink); });

    GstPad* appsinkPad = gst_element_get_static_pad(qvideosink, "sink");
    QVERIFY2(appsinkPad, "qgcqvideosink has no sink pad");
    auto appsinkPadGuard = qScopeGuard([&] { gst_object_unref(appsinkPad); });

    std::atomic<int> bufferCount{0};
    auto probeCb = +[](GstPad* /*pad*/, GstPadProbeInfo* info, gpointer userData) -> GstPadProbeReturn {
        auto* c = static_cast<std::atomic<int>*>(userData);
        if (GST_PAD_PROBE_INFO_BUFFER(info)) {
            c->fetch_add(1, std::memory_order_relaxed);
        }
        return GST_PAD_PROBE_OK;
    };
    const gulong probeId = gst_pad_add_probe(appsinkPad, GST_PAD_PROBE_TYPE_BUFFER, probeCb, &bufferCount, nullptr);
    QVERIFY(probeId);
    auto probeGuard = qScopeGuard([&] { gst_pad_remove_probe(appsinkPad, probeId); });

    using GstHwPathTelemetry::HwFallbackReason;
    const quint64 vkDeliveredBefore = GstHwPathTelemetry::peekDeliveredCount(HwVideoBufferPath::Vulkan);
    const quint64 noSyncBefore =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::VulkanNoSync);
    const quint64 noExtBefore =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::NoExt);
    (void) GstHwPathTelemetry::takeStreamDemotions(HwVideoBufferPath::Vulkan);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        QSKIP("Vulkan pipeline failed to PLAY (no Vulkan device in headless env?)");
    }

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    QString errMsg;
    bool sawError = false;
    if (msg && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError* err = nullptr;
        gchar* debug = nullptr;
        gst_message_parse_error(msg, &err, &debug);
        errMsg = QStringLiteral("%1 (%2)")
                     .arg(err ? QString::fromUtf8(err->message) : QStringLiteral("?"))
                     .arg(debug ? QString::fromUtf8(debug) : QString());
        g_clear_error(&err);
        g_free(debug);
        sawError = true;
    }
    if (msg)
        gst_message_unref(msg);
    gst_object_unref(bus);

    // Demotion telemetry is recorded on the QRhi render path, which can land after the bus EOS; poll up
    // to 2s (not a fixed window) so a slow demotion is observed rather than spuriously skipped.
    quint64 streamDemotions = 0;
    bool demotionSignalled = false;
    {
        QElapsedTimer drain;
        drain.start();
        while (drain.elapsed() < 2000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            streamDemotions += GstHwPathTelemetry::takeStreamDemotions(HwVideoBufferPath::Vulkan);
            const quint64 noSyncNow =
                GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::VulkanNoSync);
            const quint64 noExtNow =
                GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::NoExt);
            if (streamDemotions > 0 || noSyncNow > noSyncBefore || noExtNow > noExtBefore) {
                demotionSignalled = true;
                break;
            }
        }
    }

    GstCaps* negotiated = gst_pad_get_current_caps(appsinkPad);
    QString negotiatedStr;
    if (negotiated) {
        gchar* s = gst_caps_to_string(negotiated);
        negotiatedStr = QString::fromUtf8(s ? s : "");
        g_free(s);
        gst_caps_unref(negotiated);
    }

    const int probedBuffers = bufferCount.load();
    const quint64 vkDeliveredAfter = GstHwPathTelemetry::peekDeliveredCount(HwVideoBufferPath::Vulkan);

    if (sawError) {
        QSKIP(qPrintable(QStringLiteral("Vulkan pipeline error (likely no Vulkan device): %1").arg(errMsg)));
    }
    if (probedBuffers == 0 || !negotiatedStr.contains(QStringLiteral("memory:VulkanImage"))) {
        QSKIP(qPrintable(QStringLiteral("Vulkan memory not negotiated headless. Buffers: %1, caps: %2")
                             .arg(probedBuffers)
                             .arg(negotiatedStr)));
    }

    // Vulkan is compiled but runtime-dormant: QGC pins the GL RHI, so the per-frame VkDevice-match
    // guard demotes every foreign-device frame to CPU. Assert delivery still works, never zero-copy.
    QVERIFY2(frameCount > 0 || probedBuffers > 0, "No frames delivered through the sink under Vulkan caps");
    QVERIFY2(vkDeliveredAfter == vkDeliveredBefore,
             qPrintable(QStringLiteral("Vulkan zero-copy delivery was recorded but the device-match guard "
                                       "should demote (before=%1 after=%2)")
                            .arg(vkDeliveredBefore)
                            .arg(vkDeliveredAfter)));

    if (!demotionSignalled) {
        QSKIP(qPrintable(QStringLiteral("Vulkan frames negotiated VulkanImage but the device-match demotion "
                                        "did not fire headless (mapTextures() needs a QRhi render thread). "
                                        "caps: %1")
                             .arg(negotiatedStr)));
    }
}

#endif
