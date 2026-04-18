#include "UVCReceiver.h"
#include "AppMessages.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QTimer>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "VideoFrameDelivery.h"
#include "VideoSettings.h"

QGC_LOGGING_CATEGORY(UVCReceiverLog, "Video.UVCReceiver")

UVCReceiver::UVCReceiver(QObject* parent)
    : VideoReceiver(parent),
      _camera(new QCamera(this)),
      _imageCapture(new QImageCapture(this)),
      _captureSession(new QMediaCaptureSession(this)),
      _mediaDevices(new QMediaDevices(this)),
      _internalSink(new QVideoSink(this))
{
    _captureSession->setCamera(_camera);
    _captureSession->setImageCapture(_imageCapture);
    // Wire capture session to internal sink; frames are forwarded through
    // frame delivery in _rewireInternalSink() once a video sink is registered.
    _captureSession->setVideoSink(_internalSink);

    checkPermission();

    connect(_mediaDevices, &QMediaDevices::videoInputsChanged, this, [this]() {
        if (!_started)
            return;
        if (QMediaDevices::videoInputs().isEmpty()) {
            qCWarning(UVCReceiverLog) << "All UVC devices gone — closing camera";
            _closeCamera();
        } else {
            qCDebug(UVCReceiverLog) << "UVC device list changed — re-opening camera";
            _closeCamera();
            _openCamera();
        }
    });

    connect(_camera, &QCamera::errorOccurred, this, [this](QCamera::Error err, const QString& desc) {
        if (err == QCamera::NoError)
            return;
        qCWarning(UVCReceiverLog) << "QCamera error:" << err << desc;
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, desc);
    });
}

UVCReceiver::~UVCReceiver()
{
    _closeCamera();
}

void UVCReceiver::_rewireInternalSink()
{
    disconnect(_internalFrameConn);

    if (!_delivery)
        return;

    _internalFrameConn = connect(_internalSink, &QVideoSink::videoFrameChanged, this,
                                 [this](const QVideoFrame& frame) {
                                     if (frame.isValid() && _delivery)
                                         _delivery->deliverFrame(frame);
                                 });
}

void UVCReceiver::onSinkAboutToChange()
{
    disconnect(_internalFrameConn);
}

void UVCReceiver::onSinkChanged(QVideoSink* /*newSink*/)
{
    _rewireInternalSink();
    qCDebug(UVCReceiverLog) << "Video sink registered";
}

void UVCReceiver::_openCamera()
{
    const QString sourceId = getSourceId();
    if (sourceId.isEmpty()) {
        qCWarning(UVCReceiverLog) << "No UVC camera found";
        return;
    }

    const QCameraDevice device = findCameraDevice(sourceId);
    if (!device.isNull()) {
        _camera->setCameraDevice(device);
    }

    _camera->setActive(true);
    qCDebug(UVCReceiverLog) << "Camera opened:" << _camera->cameraDevice().description();
}

void UVCReceiver::_closeCamera()
{
    if (_delivery)
        _delivery->disarmWatchdog();
    _camera->setActive(false);
    qCDebug(UVCReceiverLog) << "Camera closed";
}

void UVCReceiver::start(uint32_t timeout)
{
    qCDebug(UVCReceiverLog) << "start" << timeout;

    if (_started) {
        return;
    }

    if (_permissionStatus != Qt::PermissionStatus::Granted) {
        // Denied or still Undetermined — either way we can't open the camera.
        // Previously an Undetermined status fell through to _openCamera, which
        // then failed silently in QCamera without a visible error.
        const QString msg = (_permissionStatus == Qt::PermissionStatus::Denied)
                                ? QStringLiteral("Camera permission denied")
                                : QStringLiteral("Camera permission pending — try again once granted");
        qCWarning(UVCReceiverLog) << msg;
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, msg);
        return;
    }

    _watchdogInterval = std::chrono::milliseconds(static_cast<qint64>(timeout) * 1000);
    _openCamera();

    setStarted(true);
    _setStreamingActive(true);
    emit receiverStarted();
}

void UVCReceiver::stop()
{
    qCDebug(UVCReceiverLog) << "stop";

    if (!_started) {
        return;
    }

    _closeCamera();

    setStarted(false);
    _setStreamingActive(false);
    _setDecoderActive(false);
    emit receiverStopped();
}

void UVCReceiver::startDecoding()
{
    qCDebug(UVCReceiverLog) << "startDecoding";

    if (!validateBridgeForDecoding())
        return;

    // Wire internal sink -> delivery frame forwarding.
    _rewireInternalSink();

    // Delivery-owned watchdog observes deliverFrame() directly.
    if (_watchdogInterval.count() > 0)
        _delivery->armWatchdog(_watchdogInterval);

    _setDecoderActive(true);
}

void UVCReceiver::stopDecoding()
{
    qCDebug(UVCReceiverLog) << "stopDecoding";

    // Parity with GstVideoReceiver/QtMultimediaReceiver: reject if we weren't decoding.
    if (!_decoderActive) {
        return;
    }

    if (_delivery)
        _delivery->disarmWatchdog();
    disconnect(_internalFrameConn);

    _setDecoderActive(false);
}

void UVCReceiver::_setStreamingActive(bool active)
{
    if (_streamingActive == active)
        return;
    _streamingActive = active;
    emit streamingChanged(active);
}

void UVCReceiver::_setDecoderActive(bool active)
{
    if (_decoderActive == active)
        return;
    _decoderActive = active;
    emit decodingChanged(active);
}

// ═════════════════════════════════════════════════════════════════
// Static helpers
// ═════════════════════════════════════════════════════════════════

QCameraDevice UVCReceiver::findCameraDevice(const QString& cameraId)
{
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const QCameraDevice& camera : videoInputs) {
        if (camera.description() == cameraId) {
            return camera;
        }
    }

    return QCameraDevice();
}

void UVCReceiver::checkPermission()
{
    const QCameraPermission cameraPermission;
    _permissionStatus = qApp->checkPermission(cameraPermission);
    if (_permissionStatus == Qt::PermissionStatus::Undetermined) {
        qApp->requestPermission(cameraPermission, this, [this](const QPermission& permission) {
            _permissionStatus = permission.status();
            if (_permissionStatus != Qt::PermissionStatus::Granted) {
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

bool UVCReceiver::deviceExists(const QString& device)
{
    return !findCameraDevice(device).isNull();
}

QStringList UVCReceiver::getDeviceNameList()
{
    QStringList deviceNameList;

    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const QCameraDevice& cameraDevice : videoInputs) {
        deviceNameList.append(cameraDevice.description());
    }

    return deviceNameList;
}
