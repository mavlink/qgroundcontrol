#include "FFmpegPointers.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
}

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
