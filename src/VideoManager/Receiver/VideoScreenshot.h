#pragma once

class QImage;
class QString;
class QVideoFrame;

/// Screenshots of the video on display, from a network stream or the USB camera.
namespace VideoScreenshot {

/// @p frame as displayed: rotation, mirroring and the format's viewport (crop) applied. Null for an invalid frame.
QImage displayedImage(const QVideoFrame& frame);
/// Saves @p frame as displayed to @p file, in the image format its suffix names. False when there is nothing to save
/// or saving fails.
bool save(const QVideoFrame& frame, const QString& file);

}  // namespace VideoScreenshot
