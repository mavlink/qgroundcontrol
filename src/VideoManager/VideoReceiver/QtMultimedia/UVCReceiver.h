/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtMultimedia/QCameraDevice>

#include "QtMultimediaReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(UVCReceiverLog)

class QCamera;
class QImageCapture;
class QQuickItem;

class UVCReceiver : public QtMultimediaReceiver
{
    Q_OBJECT

public:
    explicit UVCReceiver(QObject *parent = nullptr);
    ~UVCReceiver();

    bool enabled();
    QCameraDevice findCameraDevice(const QString &cameraId);

public slots:
    Q_INVOKABLE void adjustAspectRatio();

private:
    void _checkPermission();

    QCamera *_camera = nullptr;
    QImageCapture *_imageCapture = nullptr;
};
