#include "FFmpegPointers.h"

#include <QtCore/QtGlobal>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#ifdef Q_OS_ANDROID
#include <libavcodec/mediacodec.h>
#endif
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
}

// The headers of another FFmpeg release compile, and then read the libraries' structs at the wrong places.
static_assert((LIBAVCODEC_VERSION_MAJOR == QGC_FFMPEG_LIBAVCODEC_MAJOR) &&
                  (LIBAVFORMAT_VERSION_MAJOR == QGC_FFMPEG_LIBAVFORMAT_MAJOR) &&
                  (LIBAVUTIL_VERSION_MAJOR == QGC_FFMPEG_LIBAVUTIL_MAJOR),
              "The FFmpeg headers found are not those generated for the Qt kit's libraries "
              "(src/VideoManager/FFmpeg/CMakeLists.txt): another include directory has FFmpeg headers too");

namespace FFmpegPointers {

void FrameDeleter::operator()(AVFrame* frame) const
{
    av_frame_free(&frame);
}

void PacketDeleter::operator()(AVPacket* packet) const
{
    av_packet_free(&packet);
}

void CodecContextDeleter::operator()(AVCodecContext* context) const
{
#ifdef Q_OS_ANDROID
    // What Qt's QFFmpeg::getFormat() gives a MediaCodec decoder with av_mediacodec_default_init(), a reference to
    // the Surface among it, is its caller's to free: avcodec_free_context() leaves it.
    if (context) {
        av_mediacodec_default_free(context);
    }
#endif
    avcodec_free_context(&context);
}

void CodecParametersDeleter::operator()(AVCodecParameters* parameters) const
{
    avcodec_parameters_free(&parameters);
}

void ParserDeleter::operator()(AVCodecParserContext* parser) const
{
    av_parser_close(parser);
}

void BufferDeleter::operator()(AVBufferRef* buffer) const
{
    av_buffer_unref(&buffer);
}

void InputDeleter::operator()(AVFormatContext* context) const
{
    avformat_close_input(&context);
}

void OutputDeleter::operator()(AVFormatContext* context) const
{
    avformat_free_context(context);
}

void BitstreamFilterDeleter::operator()(AVBSFContext* context) const
{
    av_bsf_free(&context);
}

void IoContextDeleter::operator()(AVIOContext* context) const
{
    if (context) {
        av_freep(&context->buffer);
    }
    avio_context_free(&context);
}

}  // namespace FFmpegPointers
