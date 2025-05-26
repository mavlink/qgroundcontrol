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
#include "SettingsManager.h"
#include "VideoSettings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickItem>

QGC_LOGGING_CATEGORY(UVCReceiverLog, "qgc.videomanager.videoreceiver.qtmultimedia.uvcreceiver")

UVCReceiver::UVCReceiver(QObject *parent)
    : QtMultimediaReceiver(parent)
    , _camera(new QCamera(this))
    , _imageCapture(new QImageCapture(this))
    , _mediaDevices(new QMediaDevices(this))
{
    // qCDebug(UVCReceiverLog) << this;

    _captureSession->setCamera(_camera);
    _captureSession->setImageCapture(_imageCapture);
    _captureSession->setVideoSink(_videoSink);

    (void) connect(_captureSession, &QMediaCaptureSession::cameraChanged, this, [this] {
        adjustAspectRatio();
    });

    // QMediaDevices::defaultVideoInput()
    (void) connect(_mediaDevices, &QMediaDevices::videoInputsChanged, this, [this] {

    });

    checkPermission();
}

UVCReceiver::~UVCReceiver()
{
    // qCDebug(UVCReceiverLog) << this;
}

bool UVCReceiver::enabled()
{
#ifdef QGC_DISABLE_UVC
    return false;
#else
    return !QMediaDevices::videoInputs().isEmpty();
#endif
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
                qgcApp()->showAppMessage(QStringLiteral("Failed to get camera permission"));
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
