#include "FFmpegQtInternals.h"

#include <memory>

#include <QtCore/qtversionchecks.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
}

// Qt's FFmpeg plugin implementation is internal to Qt. Before moving to another Qt release, read again what this file
// calls in that release's qtmultimedia/src/plugins/multimedia/ffmpeg: QFFmpegVideoBuffer, QFFmpeg::getFormat() and
// QFFmpeg::VideoRenderer::renderInternal(), which this file follows (on Android with
// QFFmpeg::HWAccel::createFramesContext()), and QVideoFramePrivate::createFrame().
static_assert(QT_VERSION == QT_VERSION_CHECK(6, 12, 0),
              "The FFmpeg receiver's use of Qt's FFmpeg plugin implementation was validated with Qt 6.12.0 only; "
              "check it against this Qt version as src/VideoManager/FFmpeg/FFmpegQtInternals.cc describes");

namespace FFmpegQtInternals {

void chooseFormatsAsQt(AVCodecContext& context)
{
    context.get_format = QFFmpeg::getFormat;
}

void completeHardwareFrame([[maybe_unused]] const AVCodecContext& decoder, [[maybe_unused]] AVFrame& frame,
                           [[maybe_unused]] FFmpegPointers::Buffer& framesContext)
{
#ifdef Q_OS_ANDROID
    // As QFFmpeg::VideoRenderer::renderInternal() does on Android (QTBUG-108446).
    if (frame.hw_frames_ctx || !decoder.hw_device_ctx || (frame.format != AV_PIX_FMT_MEDIACODEC)) {
        return;
    }
    if (!framesContext) {
        FFmpegPointers::Buffer created(av_hwframe_ctx_alloc(decoder.hw_device_ctx));
        if (!created) {
            return;
        }
        auto* const frames = reinterpret_cast<AVHWFramesContext*>(created->data);
        frames->format = AV_PIX_FMT_MEDIACODEC;
        frames->sw_format = static_cast<AVPixelFormat>(frame.format);
        frames->width = frame.width;
        frames->height = frame.height;
        if (av_hwframe_ctx_init(created.get()) < 0) {
            return;
        }
        framesContext = std::move(created);
    }
    frame.hw_frames_ctx = av_buffer_ref(framesContext.get());
#endif
}

QVideoFrame videoFrame(FFmpegPointers::Frame frame, bool squarePixels)
{
    if (!frame) {
        return {};
    }
    const AVRational sampleAspectRatio = frame->sample_aspect_ratio;
    const bool known = (sampleAspectRatio.num > 0) && (sampleAspectRatio.den > 0);
    const AVRational pixelAspectRatio = (squarePixels && known) ? sampleAspectRatio : AVRational{1, 1};

    // As QFFmpeg::VideoRenderer::renderInternal() makes the frames of Qt's player.
    auto buffer = std::make_unique<QFFmpegVideoBuffer>(QFFmpeg::AVFrameUPtr(frame.release()), pixelAspectRatio);
    QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
    format.setColorSpace(buffer->colorSpace());
    format.setColorTransfer(buffer->colorTransfer());
    format.setColorRange(buffer->colorRange());
    format.setMaxLuminance(buffer->maxNits());
    return QVideoFramePrivate::createFrame(std::move(buffer), format);
}

}  // namespace FFmpegQtInternals
