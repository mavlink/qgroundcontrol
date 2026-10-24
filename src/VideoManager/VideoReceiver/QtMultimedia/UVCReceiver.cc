#include "UVCReceiver.h"
#include "AppMessages.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QMediaDevices>

QGC_LOGGING_CATEGORY(UVCReceiverLog, "Video.UVCReceiver")

bool UVCReceiver::enabled()
{
    return !QMediaDevices::videoInputs().isEmpty();
}

QCameraDevice UVCReceiver::findCameraDevice(const QString &cameraId)
{
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const QCameraDevice &camera : videoInputs) {
        if (camera.description() == cameraId) {
            return camera;
        }
    }

    return QCameraDevice();
}

void UVCReceiver::checkPermission()
{
    const QCameraPermission cameraPermission;
    if (qApp->checkPermission(cameraPermission) == Qt::PermissionStatus::Undetermined) {
        qApp->requestPermission(cameraPermission, qgcApp(), [](const QPermission &permission) {
            if (permission.status() != Qt::PermissionStatus::Granted) {
                QGC::showAppMessage(tr("Failed to get camera permission"));
            }
        });
    }
}

QString UVCReceiver::getSourceId()
{
    const QString videoSource = SettingsManager::instance()->videoSettings()->videoSource()->rawValue().toString();
    const QCameraDevice cameraDevice = findCameraDevice(videoSource);
    if (cameraDevice.isNull()) {
        return QString();
    }

    const QString videoSourceID = cameraDevice.description();
    qCDebug(UVCReceiverLog) << "Found USB source:" << videoSourceID << "Name:" << videoSource;
    return videoSourceID;
}

bool UVCReceiver::deviceExists(const QString &device)
{
    return !findCameraDevice(device).isNull();
}

QStringList UVCReceiver::getDeviceNameList()
{
    QStringList deviceNameList;

    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : videoInputs) {
        deviceNameList.append(cameraDevice.description());
    }

    return deviceNameList;
}
