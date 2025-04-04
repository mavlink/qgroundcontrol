#include "UVCReceiver.h"
#include "QtMultimediaReceiver.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QCoreApplication>
#include <QtQuick/QQuickItem>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>

QGC_LOGGING_CATEGORY(UVCReceiverLog, "qgc.video.qtmultimedia.uvcreceiver")

UVCReceiver::UVCReceiver(QObject* parent)
    : QtMultimediaReceiver(parent)
    , m_camera(new QCamera(this))
    , m_imageCapture(new QImageCapture(this))
{
    m_captureSession->setCamera(m_camera);
    m_captureSession->setImageCapture(m_imageCapture);
    m_captureSession->setVideoSink(m_videoSink);

    connect(m_captureSession, &QMediaCaptureSession::cameraChanged, this, [this]{
        // adjustAspectRatio()
        _checkPermission();
    });

    qCDebug(UVCReceiverLog) << Q_FUNC_INFO << this;
}

UVCReceiver::~UVCReceiver()
{
    qCDebug(UVCReceiverLog) << Q_FUNC_INFO << this;
}

void UVCReceiver::adjustAspectRatio(qreal height)
{
    if (!m_videoOutput) {
        return;
    }

    const QCameraFormat cameraFormat = m_camera->cameraFormat();
    if (cameraFormat.isNull()) {
        return;
    }

    const QSize resolution = cameraFormat.resolution();
    if (resolution.isValid()) {
        const qreal aspectRatio = resolution.width() / resolution.height();
        const qreal height = height * aspectRatio;
        m_videoOutput->setHeight(height * aspectRatio);
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
            if (permission.status() == Qt::PermissionStatus::Granted) {
                // qgcApp()->showRebootAppMessage(tr("Restart application for changes to take effect."));
            }
        });
    }
}

bool UVCReceiver::enabled()
{
    return QMediaDevices::videoInputs().count() > 0;
}
