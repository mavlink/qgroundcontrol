#pragma once

#include <QtCore/QString>
#include <QtCore/qcontainerfwd.h>
#include <QtMultimedia/QCameraDevice>

/// USB (UVC) cameras offered as video sources. A camera's description is its video source name. UVCCameraController
/// runs the selected camera through Qt Multimedia; no VideoReceiver is involved.
namespace UVCDevices {

/// Descriptions of the attached cameras.
QStringList deviceNames();
/// The attached camera of description @p description; a null device when none is.
QCameraDevice device(const QString& description);
bool deviceExists(const QString& description);
/// Asks for camera permission unless the user already answered.
void requestPermission();

}  // namespace UVCDevices
