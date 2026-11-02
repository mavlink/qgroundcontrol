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

QString UVCDevices::pixelFormatName(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
        case QVideoFrameFormat::Format_Invalid:
            return QString();
        case QVideoFrameFormat::Format_Jpeg:
            // A camera's JPEG frames are motion JPEG.
            return QStringLiteral("MJPEG");
        default:
            return QVideoFrameFormat::pixelFormatToString(pixelFormat);
    }
}

QString UVCDevices::formatName(const QCameraFormat& format)
{
    const QSize resolution = format.resolution();
    return QStringLiteral("%1x%2 %3 fps %4")
        .arg(resolution.width())
        .arg(resolution.height())
        .arg(QString::number(format.maxFrameRate(), 'g', 4), pixelFormatName(format.pixelFormat()));
}

QStringList UVCDevices::formatNames(const QCameraDevice& device)
{
    QList<QCameraFormat> formats = device.videoFormats();
    std::stable_sort(formats.begin(), formats.end(), [](const QCameraFormat& a, const QCameraFormat& b) {
        const qint64 pixelsA = qint64(a.resolution().width()) * a.resolution().height();
        const qint64 pixelsB = qint64(b.resolution().width()) * b.resolution().height();
        return (pixelsA != pixelsB) ? (pixelsA > pixelsB) : (a.maxFrameRate() > b.maxFrameRate());
    });
    QStringList names;
    for (const QCameraFormat& format : std::as_const(formats)) {
        const QString name = formatName(format);
        if (!names.contains(name)) {
            names.append(name);
        }
    }
    return names;
}

QCameraFormat UVCDevices::format(const QCameraDevice& device, const QString& name)
{
    if (name.isEmpty()) {
        return QCameraFormat();
    }
    const QList<QCameraFormat> formats = device.videoFormats();
    const auto format = std::find_if(formats.cbegin(), formats.cend(),
                                     [&name](const QCameraFormat& candidate) { return formatName(candidate) == name; });
    return (format != formats.cend()) ? *format : QCameraFormat();
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
