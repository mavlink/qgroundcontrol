#pragma once

#include <QtMultimedia/qtvideo.h>

class QImage;
class QString;
class QVideoFrame;

/// Screenshots of the video on display, from a network stream or the USB camera.
namespace VideoScreenshot {

/// @p frame as displayed: the format's viewport (crop), the frame's rotation and mirroring, then those of the
/// VideoOutput showing it (@p outputRotation, @p outputMirrored), in the order Qt displays them. Null for an invalid
/// frame.
QImage displayedImage(const QVideoFrame& frame, QtVideo::Rotation outputRotation = QtVideo::Rotation::None,
                      bool outputMirrored = false);
/// Saves @p frame as displayed (see displayedImage()) to @p file, in the image format its suffix names. False when
/// there is nothing to save or saving fails.
bool save(const QVideoFrame& frame, const QString& file, QtVideo::Rotation outputRotation = QtVideo::Rotation::None,
          bool outputMirrored = false);

}  // namespace VideoScreenshot
