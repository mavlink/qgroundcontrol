#include "GstQgcSinkPipelineTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtTest/QSignalSpy>

#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcTestSupport.h"
#include "gstqgc/gstqgcqvideosink.h"

namespace {

struct PipelineRunResult
{
    int frameCount = 0;
    QVideoFrameFormat lastFormat;
    qint64 lastStartTime = -1;
    QColor centerPixel;
    bool centerPixelValid = false;
    bool eos = false;
    QString errorMessage;
};

/// Plays "videotestsrc ! @p capsLine ! qgcvideosinkbin" to EOS with the bin bound to @p videoSink, as VideoManager
/// binds it, and records the frames that reach the QVideoSink.
PipelineRunResult runPipelineThroughQVideoSink(QVideoSink& videoSink, const char* capsLine, int numBuffers = 5,
                                               bool gpuZerocopy = false, const char* sourceProperties = "",
                                               bool captureCenterPixel = false)
{
    PipelineRunResult r;
    const auto owner = std::make_unique<QObject>();
    // Declared after the binding's owner, so the pipeline stops first.
    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(
            QStringLiteral("videotestsrc %1 num-buffers=%2 ! %3 ! qgcvideosinkbin name=sink gpu-zerocopy=%4")
                .arg(QString::fromUtf8(sourceProperties))
                .arg(numBuffers)
                .arg(QString::fromUtf8(capsLine))
                .arg(gpuZerocopy ? "true" : "false"))) {
        r.errorMessage = pipeline.error();
        return r;
    }
    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    if (!sinkBin || !GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, owner.get())) {
        r.errorMessage = QStringLiteral("setupQVideoSinkElement() failed");
        return r;
    }
    if (!owner->findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly)) {
        r.errorMessage = QStringLiteral("controller not created by setupQVideoSinkElement()");
        return r;
    }
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, owner.get(), [&](const QVideoFrame& f) {
        ++r.frameCount;
        r.lastFormat = f.surfaceFormat();
        r.lastStartTime = f.startTime();
        if (captureCenterPixel) {
            const QImage image = f.toImage();
            if (!image.isNull()) {
                r.centerPixel = image.pixelColor(image.width() / 2, image.height() / 2);
                r.centerPixelValid = true;
            }
        }
    });

    if (!pipeline.play()) {
        r.errorMessage = QStringLiteral("set_state(PLAYING) failed");
        return r;
    }
    const GStreamer::GstMessagePtr message = pipeline.waitForEosOrError();
    r.eos = message && (GST_MESSAGE_TYPE(message.get()) == GST_MESSAGE_EOS);
    if (!message) {
        r.errorMessage = QStringLiteral("timeout waiting for EOS");
    } else if (!r.eos) {
        r.errorMessage = GstTest::errorText(message.get(), /*withDetails=*/false);
    }
    // Frames reach the QVideoSink through queued calls to this thread, and the counting connection dies with owner
    // on return, so the caller only sees frames delivered before then.
    if (r.eos) {
        (void) UnitTest::waitForCondition([&r] { return r.frameCount > 0; }, TestTimeout::mediumDuration(),
                                          u"frames delivered to the QVideoSink");
    }
    return r;
}

}  // namespace

void GstQgcSinkPipelineTest::_testFrameDelivery()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(videoSink,
                                                "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                                "videoconvert ! video/x-raw,format=BGRA",
                                                /*numBuffers=*/10);
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.frameSize(), QSize(320, 240));
}

void GstQgcSinkPipelineTest::_testYuvPassthrough()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(
        videoSink, "video/x-raw,format=I420,width=320,height=240,framerate=30/1", /*numBuffers=*/10);
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_YUV420P);
}

void GstQgcSinkPipelineTest::_testPtsAndColorimetry()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(
        videoSink, "video/x-raw,format=I420,width=64,height=48,framerate=30/1,colorimetry=(string)bt709",
        /*numBuffers=*/5, /*gpuZerocopy=*/false, "do-timestamp=true");
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QVERIFY2(r.lastStartTime >= 0, "GstBuffer PTS not forwarded to QVideoFrame::startTime");
    QCOMPARE(r.lastFormat.colorSpace(), QVideoFrameFormat::ColorSpace_BT709);
}

void GstQgcSinkPipelineTest::_testCapsCacheInvalidation()
{
    QVideoSink videoSink;

    auto r1 = runPipelineThroughQVideoSink(videoSink,
                                           "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                           "videoconvert ! video/x-raw,format=BGRA");
    QVERIFY2(r1.eos, qUtf8Printable(QStringLiteral("Session 1: %1").arg(r1.errorMessage)));
    QVERIFY(r1.frameCount > 0);
    QCOMPARE(r1.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);

    auto r2 = runPipelineThroughQVideoSink(videoSink,
                                           "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                           "videoconvert ! video/x-raw,format=RGBA");
    QVERIFY2(r2.eos, qUtf8Printable(QStringLiteral("Session 2: %1").arg(r2.errorMessage)));
    QVERIFY(r2.frameCount > 0);
    QCOMPARE(r2.lastFormat.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
}

void GstQgcSinkPipelineTest::_testGpuZeroCopyFallback_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("backend");
    QTest::addColumn<QString>("format");

    // Software-decoded frames reach every GPU variant: in a format the sink takes, and in one only the converter reads.
    for (const QString& format : {QStringLiteral("BGRA"), QStringLiteral("Y444")}) {
        const QByteArray tag = format.toLatin1();
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
        QTest::addRow("glupload-%s", tag.constData())
            << int(GstQgc::GpuBinMode::GlUpload) << int(GstQgc::RenderBackend::OpenGL) << format;
#endif
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
        QTest::addRow("direct-dmabuf-%s", tag.constData())
            << int(GstQgc::GpuBinMode::DirectDmaBuf) << int(GstQgc::RenderBackend::OpenGL) << format;
#endif
        QTest::addRow("passthrough-%s", tag.constData())
            << int(GstQgc::GpuBinMode::Passthrough) << int(GstQgc::RenderBackend::Vulkan) << format;
    }
}

void GstQgcSinkPipelineTest::_testGpuZeroCopyFallback()
{
    QFETCH(int, mode);
    QFETCH(int, backend);
    QFETCH(QString, format);
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = true},
         .gpuBin = {.mode = static_cast<GstQgc::GpuBinMode>(mode)}});

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    const quint64 dmabufBefore =
        GstHwPathTelemetry::peek(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures);
#endif

    QVideoSink videoSink;
    const QByteArray capsLine =
        QStringLiteral("video/x-raw,format=%1,width=320,height=240,framerate=30/1").arg(format).toLatin1();
    auto r = runPipelineThroughQVideoSink(videoSink, capsLine.constData(), /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    // The glupload bin needs an EGL/GL context, which a headless runner may not have.
    if (!r.eos && (r.errorMessage.contains(QStringLiteral("egl"), Qt::CaseInsensitive) ||
                   r.errorMessage.contains(QStringLiteral("gl context"), Qt::CaseInsensitive))) {
        QSKIP(qPrintable(
            QStringLiteral("gpu-zerocopy pipeline needs a GL context (headless env): %1").arg(r.errorMessage)));
    }
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QVERIFY(r.frameCount > 0);
    QVERIFY(r.lastFormat.pixelFormat() != QVideoFrameFormat::Format_Invalid);
    if (format == QStringLiteral("BGRA")) {
        QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);
    }

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QCOMPARE(GstHwPathTelemetry::peek(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures),
             dmabufBefore);
#endif
}

void GstQgcSinkPipelineTest::_testQgcVideoSinkBinScalesNonSquarePixels_data()
{
    QTest::addColumn<bool>("gpuZeroCopy");
    QTest::addColumn<bool>("disablePar");

    QTest::newRow("cpu") << false << false;
    QTest::newRow("gpu") << true << false;
    QTest::newRow("cpu-disable-par") << false << true;
}

void GstQgcSinkPipelineTest::_testQgcVideoSinkBinScalesNonSquarePixels()
{
    QFETCH(bool, gpuZeroCopy);
    QFETCH(bool, disablePar);
    // Software frames take the GPU bin's system-memory caps too.
    const GstQgc::ScopedSinkEnvironmentForTest environment({.target = {.backend = GstQgc::RenderBackend::Vulkan}});

    // A 720x576 PAL source with 12/11 pixels, as analog encoders and DVRs send.
    constexpr int kWidth = 720;
    constexpr int kHeight = 576;
    constexpr double kDisplayAspect = (kWidth * 12.0) / (kHeight * 11.0);

    QVideoSink videoSink;
    QSize lastSize;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink,
                     [&lastSize](const QVideoFrame& frame) { lastSize = frame.size(); });
    QObject owner;
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=3 ! "
                                            "video/x-raw,format=I420,width=%1,height=%2,pixel-aspect-ratio=12/11,"
                                            "framerate=30/1 ! qgcvideosinkbin name=sink gpu-zerocopy=%3 disable-par=%4")
                                 .arg(kWidth)
                                 .arg(kHeight)
                                 .arg(gpuZeroCopy ? "true" : "false")
                                 .arg(disablePar ? "true" : "false")),
             qPrintable(pipeline.error()));
    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(sinkBin);
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &owner));
    QVERIFY(pipeline.play());
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    QVERIFY_TRUE_WAIT(lastSize.isValid(), TestTimeout::mediumMs());

    if (disablePar) {
        QCOMPARE(lastSize, QSize(kWidth, kHeight));
        return;
    }
    // Square pixels at the source's display aspect ratio.
    QCOMPARE(lastSize.height(), kHeight);
    QVERIFY2(qAbs((double(lastSize.width()) / lastSize.height()) - kDisplayAspect) < 0.01,
             qPrintable(QStringLiteral("frame %1x%2 lost the 12/11 display aspect ratio")
                            .arg(lastSize.width())
                            .arg(lastSize.height())));
}

void GstQgcSinkPipelineTest::_testWindowsGpuSinkPreservesSoftwarePixels()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

    QVideoSink videoSink;
    const auto result =
        runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1",
                                     /*numBuffers*/ 5, /*gpuZerocopy*/ true, "pattern=red",
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
#else
    QSKIP("Windows D3D11 GPU sink path is unavailable in this build");
#endif
}

void GstQgcSinkPipelineTest::_testWindowsGpuSinkPreservesDirectD3D11Memory()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

    GstElementFactory* uploadFactory = gst_element_factory_find("d3d11upload");
    if (!uploadFactory) {
        QSKIP("d3d11upload is unavailable");
    }
    gst_object_unref(uploadFactory);

    QVideoSink videoSink;
    auto result =
        runPipelineThroughQVideoSink(videoSink,
                                     "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! d3d11upload ! "
                                     "video/x-raw(memory:D3D11Memory),format=NV12",
                                     /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    if (!result.eos && result.errorMessage.contains(QStringLiteral("d3d11"), Qt::CaseInsensitive)) {
        QSKIP(qPrintable(QStringLiteral("D3D11 device unavailable in this runner: %1").arg(result.errorMessage)));
    }

    QVERIFY2(result.eos, qUtf8Printable(QStringLiteral("D3D11 pipeline: %1").arg(result.errorMessage)));
    QVERIFY(result.frameCount > 0);
    QCOMPARE(result.lastFormat.pixelFormat(), QVideoFrameFormat::Format_NV12);
#else
    QSKIP("D3D11 GPU sink path is only available in supported Windows builds");
#endif
}

void GstQgcSinkPipelineTest::_testTeardownUnderLoad()
{
    QVideoSink videoSink;

    {
        QObject controllerOwner;
        GstTest::TestPipeline pipeline;
        QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc is-live=true ! "
                                                "video/x-raw,format=BGRA,width=160,height=120,framerate=60/1 ! "
                                                "qgcvideosinkbin name=sink")),
                 qPrintable(pipeline.error()));
        const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
        QVERIFY(sinkBin);
        QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &controllerOwner));
        const GStreamer::GstElementPtr qvideosink =
            GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
        QVERIFY(qvideosink);

        QVERIFY(pipeline.play());
        QVERIFY_TRUE_WAIT(GstQgcTest::framesDelivered(qvideosink.get()) > 0, TestTimeout::mediumMs());
    }

    auto r = runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=RGBA,width=160,height=120,framerate=30/1");
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Restart: %1").arg(r.errorMessage)));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
}

void GstQgcSinkPipelineTest::_testFrameCountsTelemetrySignal()
{
    // Telemetry counters are process-global and leak across tests; diff against a baseline so the
    // assertions below measure only this pipeline's contribution.
    const GstHwPathTelemetry::Snapshot baseline = GstHwPathTelemetry::snapshot();

    QVideoSink videoSink;
    QObject controllerOwner;
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc is-live=false num-buffers=90 ! "
                                            "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1 ! "
                                            "qgcvideosinkbin name=sink")),
             qPrintable(pipeline.error()));

    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(sinkBin);
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &controllerOwner));
    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
    QVERIFY(qvideosink);

    QVERIFY(pipeline.play());
    QVERIFY_TRUE_WAIT(GstQgcTest::framesDelivered(qvideosink.get()) > 0, TestTimeout::mediumMs());

    QVERIFY(GstQgcTest::sinkStat(qvideosink.get(), GstQgc::sinkFramesField(HwVideoBufferPath::None)) > 0);
    quint64 gpuFallback = 0;
    for (auto p : {HwVideoBufferPath::DmaBuf, HwVideoBufferPath::GlMemory, HwVideoBufferPath::D3D11,
                   HwVideoBufferPath::D3D12, HwVideoBufferPath::IOSurface, HwVideoBufferPath::AHardwareBuffer}) {
        gpuFallback += GstHwPathTelemetry::peek(p, GstHwPathTelemetry::Counter::MapFailures) -
                       baseline.count(p, GstHwPathTelemetry::Counter::MapFailures);
    }
    QCOMPARE(gpuFallback, quint64(0));
}

void GstQgcSinkPipelineTest::_testInactiveQgcQVideoSinkDropsAndCounts()
{
    QVideoSink videoSink;
    QObject controllerOwner;
    QSignalSpy frameSpy(&videoSink, &QVideoSink::videoFrameChanged);
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc is-live=false num-buffers=12 ! "
                                            "video/x-raw,format=BGRA,width=96,height=64,framerate=30/1 ! "
                                            "qgcvideosinkbin name=sink")),
             qPrintable(pipeline.error()));

    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(sinkBin);
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &controllerOwner));
    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
    QVERIFY(qvideosink);

    g_object_set(qvideosink.get(), "active", FALSE, nullptr);

    QVERIFY(pipeline.play());
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    // Delivery is queued to this thread before the sink sees EOS, so running the queued calls shows any frame.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(frameSpy.isEmpty());

    guint64 input = 0;
    guint64 dropped = 0;
    guint64 delivered = 0;
    g_object_get(qvideosink.get(), "frames-input", &input, "frames-dropped", &dropped, "frames-delivered", &delivered,
                 nullptr);
    QVERIFY(input >= guint64(12));
    QCOMPARE(dropped, input);
    QCOMPARE(delivered, guint64(0));
}

void GstQgcSinkPipelineTest::_testCpuMemcpyActiveRowStrideHandling()
{
    // Width 753 is not 4-byte aligned; GStreamer will pad stride to 756.
    // If the memcpy copies stride bytes instead of active-row bytes, the
    // right-edge pixels of the right-most component will contain padding zeros.
    QVideoSink videoSink;
    QObject controllerOwner;

    QVideoFrame capturedFrame;
    // Controller teardown clears the QVideoSink with an invalid frame; keep the last real one.
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &controllerOwner, [&](const QVideoFrame& f) {
        if (f.isValid()) {
            capturedFrame = f;
        }
    });

    auto r = runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=BGRA,width=753,height=432,framerate=30/1",
                                          /*numBuffers=*/5);
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QVERIFY(capturedFrame.isValid());

    QVERIFY(capturedFrame.map(QVideoFrame::ReadOnly));
    const uchar* data = capturedFrame.bits(0);
    const int stride = capturedFrame.bytesPerLine(0);
    // Sample the last active pixel column (x=752) from row 0; BGRA so 4 bytes/pixel.
    const uchar* lastPixel = data + 752 * 4;
    // videotestsrc pattern=0 (smpte) fills with non-zero color in top rows.
    const bool nonZero = (lastPixel[0] | lastPixel[1] | lastPixel[2] | lastPixel[3]) != 0;
    capturedFrame.unmap();

    (void) stride;
    QVERIFY2(nonZero, "Last active column is all zeros — likely stride vs active-row memcpy bug");
}

void GstQgcSinkPipelineTest::_testColorimetryFrameRatePropagation()
{
    QVideoSink videoSink;
    const auto r = runPipelineThroughQVideoSink(videoSink, "video/x-raw,format=I420,width=64,height=48,framerate=30/1",
                                                /*numBuffers=*/3);
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.streamFrameRate(), 30.0);
}

#endif

UT_REGISTER_TEST(GstQgcSinkPipelineTest, TestLabel::Integration)
