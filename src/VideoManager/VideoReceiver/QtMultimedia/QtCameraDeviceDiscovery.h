#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QCameraFormat>

namespace QtCameraDeviceDiscovery {

[[nodiscard]] QString stableId(const QCameraDevice& camera);
[[nodiscard]] bool matches(const QCameraDevice& camera, const QString& key);
[[nodiscard]] QCameraDevice find(const QString& cameraId);
[[nodiscard]] QString defaultId();
[[nodiscard]] bool available();
[[nodiscard]] bool exists(const QString& cameraId);
[[nodiscard]] QStringList nameList();
[[nodiscard]] QCameraFormat bestFormat(const QCameraDevice& camera);

}  // namespace QtCameraDeviceDiscovery
