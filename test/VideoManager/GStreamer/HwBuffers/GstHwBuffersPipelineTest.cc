#include "HwBuffers/GstHwBuffersPipelineTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtMultimedia/QVideoFrameFormat>

#include "GStreamerTestHelpers.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"

void GstHwBuffersPipelineTest::_testHwBufferFactoryDispatchSystemMemory()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("HwVideoBuffer factory not compiled in this build (no GPU path enabled)");
#else

    // videotestsrc produces system memory; no GPU path should match.
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 ! "
                                            "video/x-raw,format=BGRA,width=64,height=64,framerate=30/1 ! "
                                            "fakesink name=sink enable-last-sample=true sync=false")),
             qPrintable(pipeline.error()));
    QVERIFY(pipeline.play());
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    GstSample* sample = nullptr;
    g_object_get(pipeline.element("sink").get(), "last-sample", &sample, nullptr);
    QVERIFY2(sample, "fakesink kept no last sample");

    GstVideoInfo info;
    GstCaps* caps = gst_sample_get_caps(sample);
    QVERIFY(gst_video_info_from_caps(&info, caps));

    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_BGRA8888);
    HwVideoBufferPath path = HwVideoBufferPath::None;

    HwVideoBufferContext ctxGpu;
    ctxGpu.gpuEnabled = true;
    auto buf = makeHwVideoBuffer(gst_sample_get_buffer(sample), caps, info, fmt, ctxGpu, path);
    QVERIFY(!buf);
    QCOMPARE(path, HwVideoBufferPath::None);

    HwVideoBufferContext ctxCpu;
    ctxCpu.gpuEnabled = false;
    auto buf2 = makeHwVideoBuffer(gst_sample_get_buffer(sample), caps, info, fmt, ctxCpu, path);
    QVERIFY(!buf2);

    gst_sample_unref(sample);
#endif
}

#endif

UT_REGISTER_TEST(GstHwBuffersPipelineTest, TestLabel::Integration)
