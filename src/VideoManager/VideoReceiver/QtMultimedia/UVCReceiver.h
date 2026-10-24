#pragma once

#include <QtCore/QObject>
#include <QtMultimedia/QCameraDevice>

/// UVC camera discovery; the cameras themselves are shown through a QML CaptureSession.
class UVCReceiver : public QObject
{
    Q_OBJECT

public:
    static bool enabled();
    static QCameraDevice findCameraDevice(const QString &cameraId);
    static void checkPermission();
    static QString getSourceId();
    static bool deviceExists(const QString &device);
    static QStringList getDeviceNameList();
};
