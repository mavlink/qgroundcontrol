#pragma once

#include <QtCore/QString>
#include <QtCore/qcontainerfwd.h>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QVideoFrameFormat>

/// USB (UVC) cameras offered as video sources. A camera's description is its video source name. UVCCameraController
/// runs the selected camera through Qt Multimedia; no VideoReceiver is involved.
namespace UVCDevices {

/// Descriptions of the attached cameras.
QStringList deviceNames();
/// The attached camera of description @p description; a null device when none is.
QCameraDevice device(const QString& description);
bool deviceExists(const QString& description);

/// "MJPEG", "YUYV", "NV12", ...; empty for an invalid format.
QString pixelFormatName(QVideoFrameFormat::PixelFormat pixelFormat);
/// "1920x1080 30 fps MJPEG": a camera format as settings store it and menus show it.
QString formatName(const QCameraFormat& format);
/// The formats @p device offers, as formatName() strings without duplicates, best first: more pixels, then a higher
/// frame rate.
QStringList formatNames(const QCameraDevice& device);
/// The format of @p device that formatName() calls @p name; a null format for none.
QCameraFormat format(const QCameraDevice& device, const QString& name);
/// Asks for camera permission unless the user already answered.
void requestPermission();

}  // namespace UVCDevices
