#pragma once

#include <memory>

struct AVBSFContext;
struct AVBufferRef;
struct AVCodecContext;
struct AVCodecParameters;
struct AVCodecParserContext;
struct AVFormatContext;
struct AVFrame;
struct AVIOContext;
struct AVPacket;

/// Owners of the FFmpeg objects the FFmpeg receiver allocates, each freed with its library's function.
namespace FFmpegPointers {

struct FrameDeleter
{
    void operator()(AVFrame* frame) const;
};

struct PacketDeleter
{
    void operator()(AVPacket* packet) const;
};

struct CodecContextDeleter
{
    void operator()(AVCodecContext* context) const;
};

struct CodecParametersDeleter
{
    void operator()(AVCodecParameters* parameters) const;
};

struct ParserDeleter
{
    void operator()(AVCodecParserContext* parser) const;
};

struct BufferDeleter
{
    void operator()(AVBufferRef* buffer) const;
};

/// For an input opened with avformat_open_input(): closes it. A custom AVIOContext it reads stays its owner's.
struct InputDeleter
{
    void operator()(AVFormatContext* context) const;
};

/// For an output: frees it and its streams. Its file must have been closed already.
struct OutputDeleter
{
    void operator()(AVFormatContext* context) const;
};

struct BitstreamFilterDeleter
{
    void operator()(AVBSFContext* context) const;
};

/// For an AVIOContext of avio_alloc_context(): frees it and its buffer.
struct IoContextDeleter
{
    void operator()(AVIOContext* context) const;
};

using Frame = std::unique_ptr<AVFrame, FrameDeleter>;
using Packet = std::unique_ptr<AVPacket, PacketDeleter>;
using CodecContext = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
using CodecParameters = std::unique_ptr<AVCodecParameters, CodecParametersDeleter>;
using Parser = std::unique_ptr<AVCodecParserContext, ParserDeleter>;
using Buffer = std::unique_ptr<AVBufferRef, BufferDeleter>;
using Input = std::unique_ptr<AVFormatContext, InputDeleter>;
using Output = std::unique_ptr<AVFormatContext, OutputDeleter>;
using BitstreamFilter = std::unique_ptr<AVBSFContext, BitstreamFilterDeleter>;
using IoContext = std::unique_ptr<AVIOContext, IoContextDeleter>;

}  // namespace FFmpegPointers
