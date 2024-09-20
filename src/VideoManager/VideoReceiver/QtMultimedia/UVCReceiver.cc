/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UVCReceiver.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtQuick/QQuickItem>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>

QGC_LOGGING_CATEGORY(UVCReceiverLog, "qgc.video.qtmultimedia.uvcreceiver")

UVCReceiver::UVCReceiver(QObject *parent)
    : QtMultimediaReceiver(parent)
    , _camera(new QCamera(this))
    , _imageCapture(new QImageCapture(this))
{
    _captureSession->setCamera(_camera);
    _captureSession->setImageCapture(_imageCapture);
    _captureSession->setVideoSink(_videoSink);

    (void) connect(_captureSession, &QMediaCaptureSession::cameraChanged, this, [this] {
        adjustAspectRatio();
    });

    _checkPermission();

    qCDebug(UVCReceiverLog) << Q_FUNC_INFO << this;
}

UVCReceiver::~UVCReceiver()
{
    qCDebug(UVCReceiverLog) << Q_FUNC_INFO << this;
}

void UVCReceiver::adjustAspectRatio()
{
    if (!_videoOutput) {
        return;
    }

    const QCameraFormat cameraFormat = _camera->cameraFormat();
    if (cameraFormat.isNull()) {
        return;
    }

    const QSize resolution = cameraFormat.resolution();
    if (resolution.isValid()) {
        const qreal aspectRatio = resolution.width() / resolution.height();
        const qreal height = height * aspectRatio;
        _videoOutput->setHeight(height * aspectRatio);
    }
}

QCameraDevice UVCReceiver::findCameraDevice(const QString &cameraId)
{
    // QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const QCameraDevice& camera : videoInputs) {
        if (camera.description() == cameraId) {
            return camera;
        }
    }

    return QMediaDevices::defaultVideoInput();
}

void UVCReceiver::_checkPermission()
{
    QCameraPermission cameraPermission;
    if (qApp->checkPermission(cameraPermission) == Qt::PermissionStatus::Undetermined) {
        qApp->requestPermission(cameraPermission, [this](const QPermission &permission) {
            if (permission.status() != Qt::PermissionStatus::Granted) {
                qgcApp()->showAppMessage(QStringLiteral("Failed to get camera permission"));
            }
        });
    }
}

bool UVCReceiver::enabled()
{
    return (QMediaDevices::videoInputs().count() > 0);
}
