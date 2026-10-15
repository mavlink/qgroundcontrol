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
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <array>
#include <limits>
#include <rhi/qrhi.h>

#include "GstGlFrameTextures.h"
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

void GStreamerTest::_testGlMemoryDispatch()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    ignoreLogMessage("default", QtWarningMsg,
                     QRegularExpression(QStringLiteral("This plugin does not support createPlatformOpenGLContext")));
#endif
#ifdef Q_OS_MACOS
    // GStreamer-GL emits an NSApplication warning on macOS when running outside the main thread.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("An NSApplication needs to be running")));
#endif
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstElementFactory* gluploadFactory = gst_element_factory_find("glupload");
    if (!gluploadFactory) {
        QSKIP("glupload factory unavailable — gst-gl not registered in this build");
    }
    gst_object_unref(gluploadFactory);

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=RGBA,width=320,height=240,framerate=30/1 ! "
        "glupload ! "
        "video/x-raw(memory:GLMemory) ! "
        "qgcvideosinkbin name=sink gpu-zerocopy=true",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QSKIP(qPrintable(QStringLiteral("GLMemory pipeline parse skipped: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create GLMemory test pipeline");

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");
    GstElement* qvideosink = gst_bin_get_by_name(GST_BIN(sinkBin), "qgcqvideosink");
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    GstPad* appsinkPad = gst_element_get_static_pad(qvideosink, "sink");
    QVERIFY2(appsinkPad, "qgcqvideosink has no sink pad");

    struct ProbeState
    {
        std::atomic<int> bufferCount{0};
        std::atomic<int> glMemoryCount{0};
    } probe;

    auto probeCb = +[](GstPad* /*pad*/, GstPadProbeInfo* info, gpointer userData) -> GstPadProbeReturn {
        auto* st = static_cast<ProbeState*>(userData);
        GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
        if (buf) {
            st->bufferCount.fetch_add(1, std::memory_order_relaxed);
            GstMemory* mem = gst_buffer_peek_memory(buf, 0);
            if (mem && mem->allocator && mem->allocator->mem_type &&
                g_str_has_prefix(mem->allocator->mem_type, "GLMemory")) {
                st->glMemoryCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return GST_PAD_PROBE_OK;
    };
    const gulong probeId = gst_pad_add_probe(appsinkPad, GST_PAD_PROBE_TYPE_BUFFER, probeCb, &probe, nullptr);
    QVERIFY(probeId);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        gst_pad_remove_probe(appsinkPad, probeId);
        gst_object_unref(appsinkPad);
        gst_object_unref(qvideosink);
        gst_object_unref(sinkBin);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QSKIP("GLMemory pipeline failed to PLAY (no GL context available?)");
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

    GstCaps* negotiated = gst_pad_get_current_caps(appsinkPad);
    QString negotiatedStr;
    if (negotiated) {
        gchar* s = gst_caps_to_string(negotiated);
        negotiatedStr = QString::fromUtf8(s ? s : "");
        g_free(s);
        gst_caps_unref(negotiated);
    }

    gst_pad_remove_probe(appsinkPad, probeId);
    gst_object_unref(appsinkPad);
    gst_object_unref(qvideosink);
    gst_object_unref(sinkBin);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    if (sawError) {
        // Common in headless envs without an EGL/GLX-capable display. Treat as
        // skip so the test is informative when GL is available, harmless when not.
        QSKIP(qPrintable(QStringLiteral("GLMemory pipeline error (likely no GL context): %1").arg(errMsg)));
    }
    QVERIFY2(probe.bufferCount.load() > 0, "No buffers reached qgcqvideosink under GLMemory caps");
    QVERIFY2(probe.glMemoryCount.load() > 0,
             qPrintable(QStringLiteral("GLMemory negotiated but buffers carried non-GL allocator. "
                                       "Buffers: %1, negotiated caps: %2")
                            .arg(probe.bufferCount.load())
                            .arg(negotiatedStr)));
    QVERIFY2(negotiatedStr.contains(QStringLiteral("memory:GLMemory")),
             qPrintable(QStringLiteral("Appsink negotiated caps lack memory:GLMemory: %1").arg(negotiatedStr)));
}

void GStreamerTest::_testHwFrameTexturesKeepResourcesUntilDestroyed()
{
#if !defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("GL-texture frame bundles not compiled in this build");
#else
    QRhiNullInitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, &params));
    QVERIFY(rhi);

    GstVideoInfo info;
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_NV12, 64, 64);
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    GstCaps* caps = gst_video_info_to_caps(&info);
    GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr);
    gst_buffer_unref(buffer);
    gst_caps_unref(caps);
    QVERIFY(sample);

    bool sampleReleased = false;
    gst_mini_object_weak_ref(
        GST_MINI_OBJECT(sample), [](gpointer data, GstMiniObject*) { *static_cast<bool*>(data) = true; },
        &sampleReleased);

    constexpr int kPlanes = 2;
    auto bundle = std::make_unique<GstGlFrameTextures>(rhi.get(), QSize(64, 64), QVideoFrameFormat::Format_NV12,
                                                       std::array<GLuint, GstHw::kMaxPlanes>{11, 12}, kPlanes);
    bundle->setSourceSample(sample);
    std::array<QRhiTexture*, kPlanes> planes{bundle->texture(0), bundle->texture(1)};
    QVERIFY(planes[0] && planes[1]);
    QVERIFY(!bundle->texture(kPlanes));
    QVERIFY(!bundle->texture(std::numeric_limits<uint>::max()));

    // Qt re-renders the current bundle and calls onFrameEndInvoked() after every window frame until its slot is reused.
    for (int frame = 0; frame < 3; ++frame) {
        bundle->onFrameEndInvoked();
    }
    QCOMPARE(bundle->texture(0), planes[0]);
    QCOMPARE(bundle->texture(1), planes[1]);
    QVERIFY2(!sampleReleased, "Source sample must stay alive while Qt can still sample the bundle");

    bundle.reset();
    QVERIFY2(sampleReleased, "Destroying the bundle must release the source sample");
#endif
}

#endif
