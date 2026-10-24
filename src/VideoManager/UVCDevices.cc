#include "UVCDevices.h"

#include <algorithm>

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QMediaDevices>

#include "AppMessages.h"

bool UVCDevices::available()
{
    return !QMediaDevices::videoInputs().isEmpty();
}

QStringList UVCDevices::deviceNames()
{
    QStringList names;
    for (const QCameraDevice& camera : QMediaDevices::videoInputs()) {
        names.append(camera.description());
    }
    return names;
}

bool UVCDevices::deviceExists(const QString& description)
{
    if (description.isEmpty()) {
        return false;
    }
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    return std::any_of(cameras.cbegin(), cameras.cend(),
                       [&description](const QCameraDevice& camera) { return camera.description() == description; });
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
