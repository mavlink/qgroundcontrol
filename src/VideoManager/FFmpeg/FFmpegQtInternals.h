#pragma once

#include <QtMultimedia/QVideoFrame>

#include "FFmpegPointers.h"

/// What the FFmpeg receiver takes from Qt's FFmpeg media plugin, whose implementation Qt ships as the static library
/// Qt6::FFmpegMediaPluginImplPrivate: Qt-internal, with no compatibility promise between Qt releases, and validated
/// with Qt 6.12.0 only (FFmpegQtInternals.cc refuses to compile with another). No other file of the receiver uses it.
///
/// Threading: call from any thread; Qt's own player calls both from its decoding and rendering threads.
namespace FFmpegQtInternals {

/// Has @p context choose its decoded frames' format as Qt's player does: where it has a hardware device, a format of
/// that device's, with the decoder settings Qt's texture converter needs to show it without a copy; otherwise one that
/// a QVideoFrame takes without a conversion, if the decoder offers one.
void chooseFormatsAsQt(AVCodecContext& context);

/// @p frame, decoded, as a QVideoFrame, which takes it over. A hardware frame stays in GPU memory: Qt's texture
/// converter shows it where it can, and maps it into memory otherwise. @p squarePixels: presented at its pixel aspect
/// ratio, scaled to square pixels; otherwise at its size in pixels. Invalid for a null frame.
QVideoFrame videoFrame(FFmpegPointers::Frame frame, bool squarePixels);

}  // namespace FFmpegQtInternals
