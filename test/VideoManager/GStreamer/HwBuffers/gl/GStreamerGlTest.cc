#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>

#include <QtCore/QRegularExpression>

#include "GStreamerTestHelpers.h"
#include "HwBuffers/common/HwBuffers.h"

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
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    // The direct-DMABuf bin doesn't offer GLMemory.
    const HwBuffers::ScopedGpuBinModeForTest glBin(HwBuffers::GpuBinMode::GlUpload);
#endif

    if (!GstTest::firstMissingFactory({"glupload"}).isEmpty()) {
        QSKIP("glupload factory unavailable — gst-gl not registered in this build");
    }

    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("videotestsrc num-buffers=10 ! "
                                        "video/x-raw,format=RGBA,width=320,height=240,framerate=30/1 ! "
                                        "glupload ! "
                                        "video/x-raw(memory:GLMemory) ! "
                                        "qgcvideosinkbin name=sink gpu-zerocopy=true"))) {
        QSKIP(qPrintable(QStringLiteral("GLMemory pipeline parse skipped: %1").arg(pipeline.error())));
    }

    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");
    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    const GStreamer::GstPadPtr appsinkPad = GStreamer::adoptPad(gst_element_get_static_pad(qvideosink.get(), "sink"));
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
    GStreamer::PadProbe bufferProbe;
    QVERIFY(bufferProbe.add(appsinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, probeCb, &probe));

    if (!pipeline.play()) {
        QSKIP("GLMemory pipeline failed to PLAY (no GL context available?)");
    }

    const GStreamer::GstMessagePtr msg = pipeline.waitForEosOrError();
    if (msg && (GST_MESSAGE_TYPE(msg.get()) == GST_MESSAGE_ERROR)) {
        // Common in headless envs without an EGL/GLX-capable display. Treat as
        // skip so the test is informative when GL is available, harmless when not.
        QSKIP(qPrintable(
            QStringLiteral("GLMemory pipeline error (likely no GL context): %1").arg(GstTest::errorText(msg.get()))));
    }
    const QString negotiatedStr = GstTest::currentCapsString(appsinkPad.get());
    QVERIFY2(probe.bufferCount.load() > 0, "No buffers reached qgcqvideosink under GLMemory caps");
    QVERIFY2(probe.glMemoryCount.load() > 0,
             qPrintable(QStringLiteral("GLMemory negotiated but buffers carried non-GL allocator. "
                                       "Buffers: %1, negotiated caps: %2")
                            .arg(probe.bufferCount.load())
                            .arg(negotiatedStr)));
    QVERIFY2(negotiatedStr.contains(QStringLiteral("memory:GLMemory")),
             qPrintable(QStringLiteral("Appsink negotiated caps lack memory:GLMemory: %1").arg(negotiatedStr)));
}

#endif
