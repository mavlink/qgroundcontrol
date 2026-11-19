#include "gstqgc/GstQgcTestSupport.h"

#ifdef QGC_GST_STREAMING

#include <memory>

#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "QGCQVideoSinkController.h"

namespace GstQgcTest {

PipelineRunResult runPipelineThroughQVideoSink(QVideoSink& videoSink, const char* capsLine, int numBuffers,
                                               bool gpuZerocopy, const char* sourceProperties, bool captureCenterPixel)
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

}  // namespace GstQgcTest

#endif
