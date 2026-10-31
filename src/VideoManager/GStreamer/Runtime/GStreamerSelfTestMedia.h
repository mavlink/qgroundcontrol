#pragma once

#include <chrono>
#include <gst/gst.h>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GstScoped.h"
#include "VideoReceiver.h"

/// The self-test's media pipelines: decoding the embedded samples and recording and reading back a file.
namespace GStreamer::SelfTest {

/// An encoded sample in the app's resources (Runtime/Media/generate.sh makes them).
struct Sample
{
    const char* codec;
    const char* resource;
    const char* mediaType;
    const char* parser;
};

inline constexpr Sample kH264Sample{"H.264", ":/gstreamer/selftest/h264_160x120.h264", "video/x-h264", "h264parse"};
// 320x240: NVIDIA's HEVC decoders (NVDEC, and d3d11/d3d12 on an NVIDIA GPU) reject frames under 144 pixels high.
inline constexpr Sample kH265Sample{"H.265", ":/gstreamer/selftest/h265_320x240.h265", "video/x-h265", "h265parse"};

struct RecordingFormat
{
    const char* name;
    VideoReceiver::FileFormat format;
    const char* extension;
    const char* demuxer;
};

inline constexpr RecordingFormat kRecordingFormats[] = {
    {"MP4", VideoReceiver::FileFormat::Mp4, "mp4", "qtdemux"},
    {"MKV", VideoReceiver::FileFormat::Mkv, "mkv", "matroskademux"},
};

/// Whether decodebin3 has a decoder for @p sample's codec to plug: one ranked at least MARGINAL.
bool hasDecoderFor(const Sample& sample);

/// Restores every video decoder's rank on destruction.
class DecoderRankRestorer
{
public:
    DecoderRankRestorer();
    ~DecoderRankRestorer();

    DecoderRankRestorer(const DecoderRankRestorer&) = delete;
    DecoderRankRestorer& operator=(const DecoderRankRestorer&) = delete;

private:
    std::vector<std::pair<GstFeaturePtr, guint>> _ranks;
};

struct DecodeResult
{
    QString failure;
    QStringList decoders;
    bool hardware = false;
};

/// appsrc ! <parser> ! decodebin3 ! appsink, the sink taking raw video in system memory, which every decoder can give.
DecodeResult decodeSample(const Sample& sample, const QByteArray& data, std::chrono::milliseconds timeout);

/// appsrc ! RecordingBranch's file sink (h264parse ! muxer ! filesink), as a recording is muxed.
QString recordSample(const QString& file, VideoReceiver::FileFormat format, const QByteArray& h264,
                     std::chrono::milliseconds timeout);

/// filesrc ! <demuxer> ! fakesink: the recorded file opens and holds video.
QString readBack(const QString& file, const char* demuxer, std::chrono::milliseconds timeout);

}  // namespace GStreamer::SelfTest
