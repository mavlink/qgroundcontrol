#include "HwBuffers/GstHwBuffersPipelineTest.h"

#ifdef QGC_GST_STREAMING

#if defined(Q_OS_MACOS) && defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

#include <QtCore/QScopeGuard>
#include <QtMultimedia/QVideoFrameFormat>

#include "CpuVideoFramePool.h"
#include "GStreamerTestHelpers.h"
#include "GstHwVideoBufferFactory.h"
#include "HwBuffers/GstHwImageReadback.h"
#endif

void GstHwBuffersPipelineTest::_testIOSurfaceFrameToImage()
{
#if !defined(Q_OS_MACOS) || !defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    QSKIP("IOSurface GPU path not compiled in this build");
#else
    const QString missing = GstTest::firstMissingFactory({"appsrc", "h264parse", "vtdec", "appsink"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 factory unavailable").arg(missing)));
    }
    QRhiMetalInitParams params;
    const std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Metal, &params));
    if (!rhi) {
        QSKIP("No Metal device on this runner");
    }
    QString loadError;
    const QList<GstTest::AccessUnit> units =
        GstTest::loadAccessUnits(QString::fromLatin1(GstTest::kH264Stream), &loadError);
    QVERIFY2(!units.isEmpty(), qPrintable(loadError));

    // vtdec decodes the stream's first keyframe into an IOSurface-backed CVPixelBuffer.
    GstTest::TestPipeline pipeline;
    QVERIFY2(
        pipeline.launch(QStringLiteral("appsrc name=src format=time "
                                       "caps=video/x-h264,stream-format=byte-stream,alignment=au ! "
                                       "h264parse ! vtdec ! video/x-raw,format=NV12 ! appsink name=sink sync=false")),
        qPrintable(pipeline.error()));
    QVERIFY(pipeline.play());
    const GStreamer::GstElementPtr source = pipeline.element("src");
    GstBuffer* const keyframe = gst_buffer_new_memdup(units.constFirst().bytes.constData(),
                                                      static_cast<gsize>(units.constFirst().bytes.size()));
    GST_BUFFER_PTS(keyframe) = 0;
    QCOMPARE(gst_app_src_push_buffer(GST_APP_SRC(source.get()), keyframe), GST_FLOW_OK);
    QCOMPARE(gst_app_src_end_of_stream(GST_APP_SRC(source.get())), GST_FLOW_OK);
    GstSample* const sample = gst_app_sink_try_pull_sample(GST_APP_SINK(pipeline.element("sink").get()),
                                                           GstTest::clockTime(TestTimeout::mediumDuration()));
    if (!sample) {
        QSKIP("VideoToolbox decoded no frame on this runner");
    }
    const auto unref = qScopeGuard([sample] { gst_sample_unref(sample); });

    GstVideoInfo info;
    QVERIFY(gst_video_info_from_caps(&info, gst_sample_get_caps(sample)));
    const QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&info), GST_VIDEO_INFO_HEIGHT(&info)),
                                   QVideoFrameFormat::Format_NV12);
    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    std::unique_ptr<QHwVideoBuffer> buffer =
        makeHwVideoBuffer(gst_sample_get_buffer(sample), gst_sample_get_caps(sample), info, format, context, path);
    if (!buffer) {
        QSKIP("vtdec's frame is not IOSurface-backed on this runner");
    }
    QCOMPARE(path, HwVideoBufferPath::IOSurface);

    // The same CVPixelBuffer read through the CPU is the reference.
    const QImage actual =
        QVideoFrame(std::make_unique<GstTest::RhiBoundVideoBuffer>(std::move(buffer), rhi.get())).toImage();
    const QImage expected =
        GstTest::cpuImage(CpuVideoFramePool::copyFromBuffer(gst_sample_get_buffer(sample), info, format));
    const QString mismatch = GstTest::firstImageMismatch(actual, expected, /*tolerance=*/8);
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
#endif
}

#endif
