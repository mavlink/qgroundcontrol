#pragma once

#include <QtCore/QDateTime>
#include <QtMultimedia/qtvideo.h>
#include <QtPositioning/QGeoCoordinate>

class QImage;
class QString;
class QVideoFrame;

/// Screenshots of the video on display, from a network stream or the USB camera.
namespace VideoScreenshot {

/// What a JPEG screenshot records besides its picture, as EXIF data: when it was taken, and where (the vehicle's
/// position, with its altitude above mean sea level where it has one). Invalid values are left out.
struct Exif
{
    QDateTime taken;
    QGeoCoordinate position;
};

/// @p frame as displayed: the format's viewport (crop), the frame's rotation and mirroring, then those of the
/// VideoOutput showing it (@p outputRotation, @p outputMirrored), in the order Qt displays them. Null for an invalid
/// frame.
QImage displayedImage(const QVideoFrame& frame, QtVideo::Rotation outputRotation = QtVideo::Rotation::None,
                      bool outputMirrored = false);
/// Saves @p frame as displayed (see displayedImage()) to @p file, in the image format its suffix names, with @p exif
/// for a JPEG. False when there is nothing to save or saving fails.
bool save(const QVideoFrame& frame, const QString& file, const Exif& exif = {},
          QtVideo::Rotation outputRotation = QtVideo::Rotation::None, bool outputMirrored = false);

}  // namespace VideoScreenshot
