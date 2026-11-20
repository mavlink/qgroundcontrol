#include "HwBuffers/GstHwBuffersPipelineTest.h"

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

    // A frame in system memory, as a software decoder sends it: no GPU path should match.
    GstVideoInfo info;
    const GstTest::SamplePtr sample(GstTest::makeVideoSample(GST_VIDEO_FORMAT_BGRA, 64, 64, &info));
    QVERIFY(sample);
    GstCaps* caps = gst_sample_get_caps(sample.get());

    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_BGRA8888);
    HwVideoBufferPath path = HwVideoBufferPath::None;

    HwVideoBufferContext ctxGpu;
    ctxGpu.gpuEnabled = true;
    auto buf = makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), caps, info, fmt, ctxGpu, path);
    QVERIFY(!buf);
    QCOMPARE(path, HwVideoBufferPath::None);

    HwVideoBufferContext ctxCpu;
    ctxCpu.gpuEnabled = false;
    auto buf2 = makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), caps, info, fmt, ctxCpu, path);
    QVERIFY(!buf2);
#endif
}

UT_REGISTER_TEST(GstHwBuffersPipelineTest, TestLabel::Integration)
