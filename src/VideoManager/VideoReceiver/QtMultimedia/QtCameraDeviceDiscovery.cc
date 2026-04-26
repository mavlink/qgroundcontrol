#include "QtCameraDeviceDiscovery.h"

#include <QtMultimedia/QMediaDevices>

namespace QtCameraDeviceDiscovery {

QString stableId(const QCameraDevice& camera)
{
    return QString::fromUtf8(camera.id());
}

bool matches(const QCameraDevice& camera, const QString& key)
{
    return key == camera.description() || key == stableId(camera);
}

QCameraDevice find(const QString& cameraId)
{
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const QCameraDevice& camera : videoInputs) {
        if (matches(camera, cameraId))
            return camera;
    }

    return QCameraDevice();
}

QString defaultId()
{
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    return videoInputs.isEmpty() ? QString() : stableId(videoInputs.first());
}

bool available()
{
    return !QMediaDevices::videoInputs().isEmpty();
}

bool exists(const QString& cameraId)
{
    return !find(cameraId).isNull();
}

QStringList nameList()
{
    QStringList deviceNameList;
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const QCameraDevice& cameraDevice : videoInputs)
        deviceNameList.append(cameraDevice.description());
    return deviceNameList;
}

QCameraFormat bestFormat(const QCameraDevice& camera)
{
    QCameraFormat best;
    for (const QCameraFormat& format : camera.videoFormats()) {
        if (format.isNull())
            continue;
        if (best.isNull()) {
            best = format;
            continue;
        }

        const int pixels = format.resolution().width() * format.resolution().height();
        const int bestPixels = best.resolution().width() * best.resolution().height();
        if (pixels > bestPixels ||
            (pixels == bestPixels && format.maxFrameRate() > best.maxFrameRate())) {
            best = format;
        }
    }
    return best;
}

}  // namespace QtCameraDeviceDiscovery
