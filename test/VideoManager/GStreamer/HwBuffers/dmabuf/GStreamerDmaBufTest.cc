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

void GStreamerTest::_testDmaBufDispatch()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

#if defined(QGC_GST_BIN_USE_GLUPLOAD)
    QSKIP("Direct-DMABuf dispatch not exercised when the GPU bin routes through glupload (GLMemory path)");
#endif

    for (const char* name : {"vah264enc", "vah264dec", "vapostproc"}) {
        GstElementFactory* f = gst_element_factory_find(name);
        if (!f) {
            QSKIP(qPrintable(QStringLiteral("VA-API not available: missing %1").arg(QString::fromUtf8(name))));
        }
        gst_object_unref(f);
    }

    const auto onlyDmaBufCaps = [](GstCaps* caps) -> GstCaps* {
        GstCaps* out = gst_caps_new_empty();
        if (!caps) {
            return out;
        }
        const guint n = gst_caps_get_size(caps);
        for (guint i = 0; i < n; ++i) {
            GstCapsFeatures* features = gst_caps_get_features(caps, i);
            if (!features || !gst_caps_features_contains(features, "memory:DMABuf")) {
                continue;
            }
            gst_caps_append_structure_full(out, gst_structure_copy(gst_caps_get_structure(caps, i)),
                                           gst_caps_features_copy(features));
        }
        return out;
    };

    GstCaps* qgcGpuCaps = gst_caps_from_string(GstQgc::buildGpuCapsString().c_str());
    QVERIFY2(qgcGpuCaps, "QGC GPU caps did not parse");
    auto qgcGpuCapsGuard = qScopeGuard([&] { gst_caps_unref(qgcGpuCaps); });

    GstCaps* qgcDmaBufCaps = onlyDmaBufCaps(qgcGpuCaps);
    QVERIFY2(qgcDmaBufCaps, "QGC DMABuf caps did not parse");
    auto qgcDmaBufCapsGuard = qScopeGuard([&] { gst_caps_unref(qgcDmaBufCaps); });
    QVERIFY2(!gst_caps_is_empty(qgcDmaBufCaps), "QGC GPU caps did not include a DMABuf structure");

    {
        GError* preflightError = nullptr;
        GstElement* preflight = gst_parse_launch(
            "videotestsrc num-buffers=2 ! "
            "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! "
            "vah264enc ! h264parse ! vah264dec ! vapostproc ! "
            "capsfilter name=qgccaps ! fakesink sync=false",
            &preflightError);
        if (preflightError) {
            const QString msg = QString::fromUtf8(preflightError->message);
            g_clear_error(&preflightError);
            QSKIP(qPrintable(QStringLiteral("DMABuf preflight parse skipped: %1").arg(msg)));
        }
        QVERIFY2(preflight, "Failed to create DMABuf preflight pipeline");
        auto preflightGuard = qScopeGuard([&] {
            gst_element_set_state(preflight, GST_STATE_NULL);
            gst_object_unref(preflight);
        });

        GstElement* capsFilter = gst_bin_get_by_name(GST_BIN(preflight), "qgccaps");
        QVERIFY2(capsFilter, "Could not find DMABuf preflight capsfilter");
        g_object_set(capsFilter, "caps", qgcDmaBufCaps, nullptr);
        gst_object_unref(capsFilter);

        const GstStateChangeReturn preflightRet = gst_element_set_state(preflight, GST_STATE_PLAYING);
        if (preflightRet == GST_STATE_CHANGE_FAILURE) {
            QSKIP("DMABuf preflight failed to PLAY (no VA driver / DRI device?)");
        }

        GstBus* bus = gst_element_get_bus(preflight);
        GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                                                     static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        gst_object_unref(bus);

        bool sawPreflightError = false;
        QString errMsg;
        if (msg && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            errMsg = QStringLiteral("%1 (%2)")
                         .arg(err ? QString::fromUtf8(err->message) : QStringLiteral("?"))
                         .arg(debug ? QString::fromUtf8(debug) : QString());
            g_clear_error(&err);
            g_free(debug);
            sawPreflightError = true;
        }
        if (msg) {
            gst_message_unref(msg);
        }

        if (!msg || sawPreflightError) {
            gchar* qgcCapsStr = gst_caps_to_string(qgcDmaBufCaps);
            const QString reason = !msg ? QStringLiteral("timed out waiting for EOS")
                                        : QStringLiteral("negotiation failed: %1").arg(errMsg);
            const QString message =
                QStringLiteral("VA driver cannot produce QGC-compatible direct DMABuf caps (%1). QGC: %2")
                    .arg(reason, QString::fromUtf8(qgcCapsStr ? qgcCapsStr : ""));
            g_free(qgcCapsStr);
            QSKIP(qPrintable(message));
        }
    }

    QVideoSink videoSink;

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! "
        "vah264enc ! h264parse ! vah264dec ! vapostproc ! "
        "capsfilter name=qgccaps ! "
        "qgcvideosinkbin name=sink gpu-zerocopy=true",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QSKIP(qPrintable(QStringLiteral("DMABuf pipeline parse skipped: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create DMABuf test pipeline");
    auto pipelineGuard = qScopeGuard([&] {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    });

    GstElement* capsFilter = gst_bin_get_by_name(GST_BIN(pipeline), "qgccaps");
    QVERIFY2(capsFilter, "Could not find DMABuf capsfilter");
    g_object_set(capsFilter, "caps", qgcDmaBufCaps, nullptr);
    gst_object_unref(capsFilter);

    GstElement* sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");
    auto sinkBinGuard = qScopeGuard([&] { gst_object_unref(sinkBin); });

    // Without a real QVideoSink, makeAdapterContext(true) never sets gpuEnabled / resolves the
    // EGLDisplay, so show_frame's HwVideoBufferContext stays CPU-only and the DmaBuf counter never moves.
    auto owner = std::make_unique<QObject>();
    if (!GStreamer::setupQVideoSinkElement(sinkBin, &videoSink, owner.get())) {
        QSKIP("setupQVideoSinkElement() failed (no GPU context for DMABuf)");
    }

    GstElement* qvideosink = gst_bin_get_by_name(GST_BIN(sinkBin), "qgcqvideosink");
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    auto qvideosinkGuard = qScopeGuard([&] { gst_object_unref(qvideosink); });

    GstPad* appsinkPad = gst_element_get_static_pad(qvideosink, "sink");
    QVERIFY2(appsinkPad, "qgcqvideosink has no sink pad");
    auto appsinkPadGuard = qScopeGuard([&] { gst_object_unref(appsinkPad); });

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
            if (mem && mem->allocator && mem->allocator->mem_type &&
                g_str_has_prefix(mem->allocator->mem_type, "DMABuf")) {
                st->dmaBufCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return GST_PAD_PROBE_OK;
    };
    const gulong probeId = gst_pad_add_probe(appsinkPad, GST_PAD_PROBE_TYPE_BUFFER, probeCb, &probe, nullptr);
    QVERIFY(probeId);
    auto probeGuard = qScopeGuard([&] { gst_pad_remove_probe(appsinkPad, probeId); });

    const quint64 deliveredBefore = GstHwPathTelemetry::peekDeliveredCount(HwVideoBufferPath::DmaBuf);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        QSKIP("DMABuf pipeline failed to PLAY (no VA driver / DRI device?)");
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

    // Settle drain so queued videoFrameChanged deliveries land before telemetry is read.
    {
        QElapsedTimer drain;
        drain.start();
        while (drain.elapsed() < 100) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
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

    const quint64 deliveredAfter = GstHwPathTelemetry::peekDeliveredCount(HwVideoBufferPath::DmaBuf);
    const int bufferCount = probe.bufferCount.load();
    const int dmaBufCount = probe.dmaBufCount.load();

    if (sawError) {
        QSKIP(qPrintable(QStringLiteral("DMABuf pipeline error (likely no VA driver): %1").arg(errMsg)));
    }
    QVERIFY2(bufferCount > 0, "No buffers reached qgcqvideosink under DMABuf caps");
    if (dmaBufCount == 0 || !negotiatedStr.contains(QStringLiteral("memory:DMABuf"))) {
        QSKIP(qPrintable(QStringLiteral("VA driver chose system memory, not DMABuf. Buffers: %1, "
                                        "negotiated caps: %2")
                             .arg(bufferCount)
                             .arg(negotiatedStr)));
    }
    QVERIFY2(deliveredAfter > deliveredBefore,
             qPrintable(QStringLiteral("DMABuf negotiated but DmaBuf delivered-count did not advance "
                                       "(before=%1 after=%2). Buffers: %3, caps: %4")
                            .arg(deliveredBefore)
                            .arg(deliveredAfter)
                            .arg(bufferCount)
                            .arg(negotiatedStr)));
}

void GStreamerTest::_testDmaDrmCapsRejectNonLinearModifiers()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
    constexpr quint64 kIntelTileYModifier = 0x0100000000000009ULL;
    QVERIFY2(!GstHw::dmaDrmModifierAdvertisedForTest(kIntelTileYModifier),
             "Direct QGC DMABuf caps must not advertise tiled VA modifiers; Mesa/Gallium can segfault while "
             "binding the resulting EGLImage on Qt's render thread");
#else
    QSKIP("DMABuf DMA_DRM caps unavailable in this build");
#endif
}

void GStreamerTest::_testDmaBufRejectsNonLinearDirectImport()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    constexpr guint64 kLinearModifier = 0;
    constexpr guint64 kIntelTileYModifier = 0x0100000000000009ULL;

    QVERIFY(GstDmaBufVideoBuffer::directGlImportAllowedForTest(false, kLinearModifier));
    QVERIFY(GstDmaBufVideoBuffer::directGlImportAllowedForTest(true, kLinearModifier));
    QVERIFY2(!GstDmaBufVideoBuffer::directGlImportAllowedForTest(false, kIntelTileYModifier),
             "Non-LINEAR DMABuf cannot be imported without EGL_EXT_image_dma_buf_import_modifiers");
    QVERIFY2(!GstDmaBufVideoBuffer::directGlImportAllowedForTest(true, kIntelTileYModifier),
             "Direct QGC DMABuf must reject tiled VA modifiers even when EGL advertises modifier support; binding "
             "the resulting EGLImage can segfault in Mesa/Gallium");
#else
    QSKIP("DMABuf unavailable in this build");
#endif
}

void GStreamerTest::_testDmaBufTiledImportAvoidsTexStorage()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    constexpr guint64 kLinearModifier = 0;
    constexpr guint64 kIntelTileYModifier = 0x0100000000000009ULL;

    QVERIFY(GstDmaBufVideoBuffer::texStorageImportAllowedForTest(false, true, kLinearModifier));
    QVERIFY2(!GstDmaBufVideoBuffer::texStorageImportAllowedForTest(false, true, kIntelTileYModifier),
             "Tiled DMABuf EGLImages must avoid GL_EXT_EGL_image_storage; Mesa/Gallium can segfault while "
             "binding a tiled VA image through glEGLImageTargetTexStorageEXT");
    QVERIFY(!GstDmaBufVideoBuffer::texStorageImportAllowedForTest(true, true, kLinearModifier));
    QVERIFY(!GstDmaBufVideoBuffer::texStorageImportAllowedForTest(false, false, kLinearModifier));
#else
    QSKIP("DMABuf unavailable in this build");
#endif
}

void GStreamerTest::_testHwBufferMapTexturesGuard()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    GStreamer::redirectGLibLogging();

    const quint64 failsBefore = GstHwPathTelemetry::peekMapFailureCount(HwVideoBufferPath::DmaBuf);

    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=1 ! "
        R"(capsfilter caps="video/x-raw(memory:DMABuf),format=NV12,width=64,height=64" ! )"
        "fakesink name=sink sync=false",
        nullptr);
    if (!pipeline) {
        QSKIP("Could not construct DMABuf test pipeline (element missing)");
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(pipeline);
        QSKIP("DMABuf pipeline failed to reach PAUSED (no DMABuf source on this machine)");
    }
    gst_element_get_state(pipeline, nullptr, nullptr, 2 * GST_SECOND);

    GstElement* fakesink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    GstSample* sample = nullptr;
    if (fakesink) {
        g_object_get(fakesink, "last-sample", &sample, nullptr);
        gst_object_unref(fakesink);
    }

    if (!sample) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QSKIP("No DMABuf sample produced (caps negotiation failed on this machine)");
    }

    GstVideoInfo info;
    GstCaps* caps = gst_sample_get_caps(sample);
    gst_video_info_from_caps(&info, caps);
    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_NV12);

    GstDmaBufVideoBuffer buf(sample, info, fmt, EGL_NO_DISPLAY);
    QVideoFrameTexturesUPtr old;
    QVideoFrameTexturesUPtr result = buf.mapTextures(*reinterpret_cast<QRhi*>(1), old);
    QVERIFY(!result);

    const quint64 failsAfter = GstHwPathTelemetry::peekMapFailureCount(HwVideoBufferPath::DmaBuf);
    QVERIFY(failsAfter > failsBefore);

    gst_sample_unref(sample);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
#endif
}

void GStreamerTest::_testHwBufferFactoryCacheRejectsMemoryTypeChange()
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

    GstCaps* caps = gst_video_info_to_caps(&info);
    QVERIFY(caps);
    const auto capsGuard = qScopeGuard([&] { gst_caps_unref(caps); });

    GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr);
    QVERIFY(sample);
    const auto sampleGuard = qScopeGuard([&] { gst_sample_unref(sample); });

    HwVideoBufferContext context;
    context.gpuEnabled = true;
    context.dmaBufEglDisplay = reinterpret_cast<EGLDisplay>(quintptr(1));

    HwResolvedPathCache cache;
    cache.path = HwVideoBufferPath::DmaBuf;
    cache.validated = true;

    HwVideoBufferPath path = HwVideoBufferPath::None;
    QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_BGRA8888);

    auto hwBuffer = makeHwVideoBuffer(sample, info, format, context, path, &cache);
    QVERIFY2(!hwBuffer, "Cached DMABuf path must reject a system-memory buffer and fall back to CPU mapping");
    QCOMPARE(path, HwVideoBufferPath::None);
    QVERIFY(!cache.validated);
#endif
}

void GStreamerTest::_testDmaBufSingleFdImportEnvGate()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    TestFixtures::EnvVarFixture gate("QGC_GST_DMABUF_SINGLE_EGLIMAGE");

    qunsetenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE");
    QVERIFY(GstDmaBufVideoBuffer::singleFdImportEnabledForTest());

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "0");
    QVERIFY(!GstDmaBufVideoBuffer::singleFdImportEnabledForTest());

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "false");
    QVERIFY(!GstDmaBufVideoBuffer::singleFdImportEnabledForTest());

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "off");
    QVERIFY(!GstDmaBufVideoBuffer::singleFdImportEnabledForTest());

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "1");
    QVERIFY(GstDmaBufVideoBuffer::singleFdImportEnabledForTest());

    qputenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE", "force");
    QVERIFY(GstDmaBufVideoBuffer::singleFdImportEnabledForTest());
#endif
}

#endif
