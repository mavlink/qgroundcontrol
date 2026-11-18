#include "HwBuffers/GstHwBuffersPipelineTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QSize>
#include <QtCore/QStringList>
#include <QtMultimedia/QVideoSink>

#include "GStreamerTestHelpers.h"
#include "HwBuffers/HwBuffers.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcTestSupport.h"

void GstHwBuffersPipelineTest::_testGlMemoryDispatch_data()
{
    QTest::addColumn<int>("mode");

    // The sink offers GLMemory only where gst-gl shares Qt's OpenGL context, and the direct-DMABuf bin offers none.
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    QTest::newRow("glupload bin") << int(GstQgc::GpuBinMode::GlUpload);
#else
    QTest::newRow("passthrough bin") << int(GstQgc::GpuBinMode::Passthrough);
#endif
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) && defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QTest::newRow("DMABuf-or-glupload bin") << int(GstQgc::GpuBinMode::DmaBufOrGlUpload);
#endif
}

void GstHwBuffersPipelineTest::_testGlMemoryDispatch()
{
    QFETCH(int, mode);
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    ignoreLogMessage("default", QtWarningMsg,
                     QRegularExpression(QStringLiteral("This plugin does not support createPlatformOpenGLContext")));
#endif
#ifdef Q_OS_MACOS
    // GStreamer-GL emits an NSApplication warning on macOS when running outside the main thread.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("An NSApplication needs to be running")));
#endif
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = GstQgc::RenderBackend::OpenGL, .egl = true},
         .gpuBin = {.mode = static_cast<GstQgc::GpuBinMode>(mode)}});

    GSTTEST_SKIP_UNLESS_FACTORIES("glupload");

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
    const GStreamer::GstElementPtr qvideosink = GstQgcTest::qvideosink(sinkBin.get());
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(qvideosink.get(), "sink"));
    QVERIFY2(sinkPad, "qgcqvideosink has no sink pad");

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
    QVERIFY(bufferProbe.add(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, probeCb, &probe));

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
    const QString negotiatedStr = GstTest::currentCapsString(sinkPad.get());
    QVERIFY2(probe.bufferCount.load() > 0, "No buffers reached qgcqvideosink under GLMemory caps");
    QVERIFY2(probe.glMemoryCount.load() > 0,
             qPrintable(QStringLiteral("GLMemory negotiated but buffers carried non-GL allocator. "
                                       "Buffers: %1, negotiated caps: %2")
                            .arg(probe.bufferCount.load())
                            .arg(negotiatedStr)));
    QVERIFY2(negotiatedStr.contains(QStringLiteral("memory:GLMemory")),
             qPrintable(QStringLiteral("qgcqvideosink negotiated caps lack memory:GLMemory: %1").arg(negotiatedStr)));
}

void GstHwBuffersPipelineTest::_testGlUploadBinTakesSoftwareFrames_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("pixelAspectRatio");

    const int glUpload = int(GstQgc::GpuBinMode::GlUpload);
    // In a format the sink takes, and in one only the converter reads.
    QTest::newRow("BGRA") << glUpload << QStringLiteral("BGRA") << QStringLiteral("1/1");
    QTest::newRow("Y444") << glUpload << QStringLiteral("Y444") << QStringLiteral("1/1");
    // The converter scales them to square pixels ahead of the upload: the bin has nothing that scales GLMemory.
    QTest::newRow("non-square pixels") << glUpload << QStringLiteral("BGRA") << QStringLiteral("12/11");
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    // Its switchbin sends system memory through the same glupload chain.
    const int dmaBufOrGlUpload = int(GstQgc::GpuBinMode::DmaBufOrGlUpload);
    QTest::newRow("Y444, DMABuf-or-glupload bin")
        << dmaBufOrGlUpload << QStringLiteral("Y444") << QStringLiteral("1/1");
    QTest::newRow("non-square pixels, DMABuf-or-glupload bin")
        << dmaBufOrGlUpload << QStringLiteral("BGRA") << QStringLiteral("12/11");
#endif
}

void GstHwBuffersPipelineTest::_testGlUploadBinTakesSoftwareFrames()
{
#if !defined(QGC_GST_BIN_HAS_GLUPLOAD)
    QSKIP("glupload bin not compiled in this build");
#else
    QFETCH(int, mode);
    QFETCH(QString, format);
    QFETCH(QString, pixelAspectRatio);
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = GstQgc::RenderBackend::OpenGL, .egl = true},
         .gpuBin = {.mode = static_cast<GstQgc::GpuBinMode>(mode)}});

    QVideoSink videoSink;
    const QByteArray capsLine =
        QStringLiteral("video/x-raw,format=%1,width=320,height=240,pixel-aspect-ratio=%2,framerate=30/1")
            .arg(format, pixelAspectRatio)
            .toLatin1();
    const GstQgcTest::PipelineRunResult r = GstQgcTest::runPipelineThroughQVideoSink(
        videoSink, capsLine.constData(), /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    // The bin's glupload needs an EGL/GL context, which a headless runner may not have.
    if (!r.eos && (r.errorMessage.contains(QStringLiteral("egl"), Qt::CaseInsensitive) ||
                   r.errorMessage.contains(QStringLiteral("gl context"), Qt::CaseInsensitive))) {
        QSKIP(qPrintable(QStringLiteral("glupload needs a GL context (headless env): %1").arg(r.errorMessage)));
    }
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QVERIFY(r.frameCount > 0);
    QVERIFY(r.lastFormat.pixelFormat() != QVideoFrameFormat::Format_Invalid);
    if (format == QStringLiteral("BGRA")) {
        QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);
    }
    // Square pixels at the source's display aspect ratio.
    const QStringList ratio = pixelAspectRatio.split(QLatin1Char('/'));
    const double displayAspect = (320.0 * ratio.at(0).toInt()) / (240.0 * ratio.at(1).toInt());
    const QSize frameSize = r.lastFormat.frameSize();
    QCOMPARE(frameSize.height(), 240);
    QVERIFY2(qAbs((double(frameSize.width()) / frameSize.height()) - displayAspect) < 0.01,
             qPrintable(QStringLiteral("frame %1x%2 doesn't have the source's display aspect ratio")
                            .arg(frameSize.width())
                            .arg(frameSize.height())));
#endif
}

#endif
