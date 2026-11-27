#pragma once

#include <gst/gst.h>
#include <optional>

#include <QtCore/QString>

#include "GstScoped.h"
#include "VideoReceiver.h"

/// The bin that writes one recording file, [parser !] muxer ! filesink, on its own: RecordingBranch starts, switches
/// and finishes recordings with it.
namespace GStreamer::RecordingSink {

/// A recording's sink bin, and the parts of it RecordingBranch acts on. Owned, non-floating refs; the bin holds the
/// parts.
struct SinkBin
{
    GstElementPtr bin;
    GstElementPtr parser;  ///< Null when the stream goes to the muxer as it is.
    GstElementPtr muxer;

    explicit operator bool() const { return bin != nullptr; }
};

/// The sink bin a recording links to the valve: an optional parser for @p inputCaps, the muxer for @p format and a
/// filesink writing @p file. A null bin on failure.
SinkBin create(const QString& file, VideoReceiver::FileFormat format, const GstCaps* inputCaps);

/// The container a recording of a stream with @p inputCaps goes into when @p format is asked for: @p format where
/// its muxer takes the stream's codec, else Matroska, which holds every codec the receiver plays. nullopt when
/// neither muxer takes it. Without caps that tell the codec, @p format.
std::optional<VideoReceiver::FileFormat> containerFor(VideoReceiver::FileFormat format, const GstCaps* inputCaps);

}  // namespace GStreamer::RecordingSink
