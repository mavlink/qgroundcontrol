#pragma once

#include <QtMultimedia/QVideoFrame>

/// Frames sharing another frame's picture.
namespace SharedVideoFrame {

/// A frame showing @p frame's picture with a copy of its metadata (times, rotation, mirroring, subtitle). It shares
/// @p frame's buffer but not its metadata, which copies of a QVideoFrame share: changing it leaves @p frame and its
/// copies alone. Its picture is read through @p frame, in memory: it has no GPU textures, whatever @p frame has.
/// Invalid for an invalid frame.
QVideoFrame withOwnMetadata(const QVideoFrame& frame);

}  // namespace SharedVideoFrame
