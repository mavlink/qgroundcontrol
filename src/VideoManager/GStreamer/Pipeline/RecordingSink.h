#pragma once

#include <gst/gst.h>
#include <optional>

#include <QtCore/QString>

#include "GstScoped.h"
#include "VideoReceiver.h"

/// The bin that writes one recording file, [parser !] muxer ! filesink, on its own: RecordingBranch starts, switches
/// and finishes recordings with it, and finds its "parser" and "muxer" by those names.
namespace GStreamer::RecordingSink {

/// The sink bin a recording links to the valve: an optional parser for @p inputCaps, the muxer for @p format and a
/// filesink writing @p file. Returns an owned, non-floating ref, or null on failure.
GstElementPtr create(const QString& file, VideoReceiver::FileFormat format, const GstCaps* inputCaps);

/// The container a recording of a stream with @p inputCaps goes into when @p format is asked for: @p format where
/// its muxer takes the stream's codec, else Matroska, which holds every codec the receiver plays. nullopt when
/// neither muxer takes it. Without caps that tell the codec, @p format.
std::optional<VideoReceiver::FileFormat> containerFor(VideoReceiver::FileFormat format, const GstCaps* inputCaps);

}  // namespace GStreamer::RecordingSink
