#include "GstQgcSinkPipelineTest.h"

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <QtCore/QCoreApplication>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtTest/QSignalSpy>

#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "HwBuffers/HwBuffers.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcTestSupport.h"

void GstQgcSinkPipelineTest::_testFrameDelivery()
{
    QVideoSink videoSink;
    const auto r =
        GstQgcTest::runPipelineThroughQVideoSink(videoSink,
                                                 "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
                                                 "videoconvert ! video/x-raw,format=BGRA",
                                                 /*numBuffers=*/10);
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.frameSize(), QSize(320, 240));
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);
}

void GstQgcSinkPipelineTest::_testPtsAndColorimetry()
{
    QVideoSink videoSink;
    const auto r = GstQgcTest::runPipelineThroughQVideoSink(
        videoSink, "video/x-raw,format=I420,width=64,height=48,framerate=30/1,colorimetry=(string)bt709",
        /*numBuffers=*/5, /*gpuZerocopy=*/false, "do-timestamp=true");
    QVERIFY2(r.eos, qUtf8Printable(r.errorMessage));
    QVERIFY(r.frameCount > 0);
    // The bin leaves YUV to Qt's shaders instead of converting it to RGB.
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_YUV420P);
    QVERIFY2(r.lastStartTime >= 0, "GstBuffer PTS not forwarded to QVideoFrame::startTime");
    QCOMPARE(r.lastFormat.colorSpace(), QVideoFrameFormat::ColorSpace_BT709);
    QCOMPARE(r.lastFormat.streamFrameRate(), 30.0);
}

void GstQgcSinkPipelineTest::_testGpuZeroCopyFallback_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("backend");
    QTest::addColumn<QString>("format");

    // Software-decoded frames reach the GPU variants that need no GPU for them (GstHwBuffersPipelineTest covers the
    // glupload bin): in a format the sink takes, and in one only the converter reads.
    for (const QString& format : {QStringLiteral("BGRA"), QStringLiteral("Y444")}) {
        const QByteArray tag = format.toLatin1();
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
        GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures);
#endif

    QVideoSink videoSink;
    const QByteArray capsLine =
        QStringLiteral("video/x-raw,format=%1,width=320,height=240,framerate=30/1").arg(format).toLatin1();
    auto r = GstQgcTest::runPipelineThroughQVideoSink(videoSink, capsLine.constData(), /*numBuffers*/ 5,
                                                      /*gpuZerocopy*/ true);
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QVERIFY(r.frameCount > 0);
    QVERIFY(r.lastFormat.pixelFormat() != QVideoFrameFormat::Format_Invalid);
    if (format == QStringLiteral("BGRA")) {
        QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_BGRA8888);
    }

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QCOMPARE(GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures),
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
        const GStreamer::GstElementPtr qvideosink = GstQgcTest::qvideosink(sinkBin.get());
        QVERIFY(qvideosink);

        QVERIFY(pipeline.play());
        QVERIFY_TRUE_WAIT(GstQgcTest::framesDelivered(qvideosink.get()) > 0, TestTimeout::mediumMs());
    }

    auto r = GstQgcTest::runPipelineThroughQVideoSink(videoSink,
                                                      "video/x-raw,format=RGBA,width=160,height=120,framerate=30/1");
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Restart: %1").arg(r.errorMessage)));
    QVERIFY(r.frameCount > 0);
    QCOMPARE(r.lastFormat.pixelFormat(), QVideoFrameFormat::Format_RGBA8888);
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
    const GStreamer::GstElementPtr qvideosink = GstQgcTest::qvideosink(sinkBin.get());
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

void GstQgcSinkPipelineTest::_testLateFramesAreShown()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc");
    // A live source and a sink that syncs to the clock, as the receiver sets it for a live stream.
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("appsrc name=src is-live=true format=time "
                                            "caps=video/x-raw,format=BGRA,width=4,height=4,framerate=30/1 ! "
                                            "qgcvideosinkbin name=sink sync=true")),
             qPrintable(pipeline.error()));
    const GStreamer::GstElementPtr source = pipeline.element("src");
    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(source && sinkBin);
    const GStreamer::GstElementPtr qvideosink = GstQgcTest::qvideosink(sinkBin.get());
    QVERIFY(qvideosink);

    // The pipeline's time starts at ten seconds, so frames stamped from zero are that late: a decoder that fell
    // behind. A GstVideoSink drops a frame that ends more than 5 ms before the time it arrives at.
    GstClock* const clock = gst_system_clock_obtain();
    gst_pipeline_use_clock(GST_PIPELINE(pipeline.get()), clock);
    gst_element_set_start_time(pipeline.get(), GST_CLOCK_TIME_NONE);
    gst_element_set_base_time(pipeline.get(), gst_clock_get_time(clock) - (10 * GST_SECOND));
    gst_object_unref(clock);
    QVERIFY(pipeline.play());

    constexpr int kFrames = 5;
    constexpr GstClockTime kFrameDuration = GST_SECOND / 30;
    for (int i = 0; i < kFrames; ++i) {
        GstBuffer* const buffer = gst_buffer_new_allocate(nullptr, 4 * 4 * 4, nullptr);
        gst_buffer_memset(buffer, 0, 0x80, 4 * 4 * 4);
        GST_BUFFER_PTS(buffer) = GstClockTime(i) * kFrameDuration;
        GST_BUFFER_DURATION(buffer) = kFrameDuration;
        QCOMPARE(gst_app_src_push_buffer(GST_APP_SRC(source.get()), buffer), GST_FLOW_OK);
    }
    QCOMPARE(gst_app_src_end_of_stream(GST_APP_SRC(source.get())), GST_FLOW_OK);
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    guint64 input = 0;
    g_object_get(qvideosink.get(), "frames-input", &input, nullptr);
    QCOMPARE(input, guint64(kFrames));
}

UT_REGISTER_TEST(GstQgcSinkPipelineTest, TestLabel::Integration)
