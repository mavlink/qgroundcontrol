#pragma once

#include <QtCore/QString>
#include <QtCore/qcontainerfwd.h>

/// USB (UVC) cameras offered as video sources. A camera's description is its video source name. The Fly View shows
/// the selected camera through a QML CaptureSession; no VideoReceiver is involved.
namespace UVCDevices {

/// True when at least one camera is attached.
bool available();
/// Descriptions of the attached cameras.
QStringList deviceNames();
bool deviceExists(const QString& description);
/// Asks for camera permission unless the user already answered.
void requestPermission();

}  // namespace UVCDevices
