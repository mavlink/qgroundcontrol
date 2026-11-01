#include "UVCDevices.h"

#include <algorithm>

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtMultimedia/QMediaDevices>

#include "AppMessages.h"

QStringList UVCDevices::deviceNames()
{
    QStringList names;
    for (const QCameraDevice& camera : QMediaDevices::videoInputs()) {
        names.append(camera.description());
    }
    return names;
}

QCameraDevice UVCDevices::device(const QString& description)
{
    if (description.isEmpty()) {
        return QCameraDevice();
    }
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    const auto camera = std::find_if(cameras.cbegin(), cameras.cend(), [&description](const QCameraDevice& candidate) {
        return candidate.description() == description;
    });
    return (camera != cameras.cend()) ? *camera : QCameraDevice();
}

bool UVCDevices::deviceExists(const QString& description)
{
    return !device(description).isNull();
}

void UVCDevices::requestPermission()
{
    const QCameraPermission cameraPermission;
    if (qApp->checkPermission(cameraPermission) != Qt::PermissionStatus::Undetermined) {
        return;
    }
    qApp->requestPermission(cameraPermission, qApp, [](const QPermission& permission) {
        if (permission.status() != Qt::PermissionStatus::Granted) {
            QGC::showAppMessage(QCoreApplication::translate("UVCDevices", "Failed to get camera permission"));
        }
    });
}
