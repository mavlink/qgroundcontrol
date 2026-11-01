#include "UVCCameraController.h"

#include <algorithm>

#include <QtCore/QTimer>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QVideoSink>
#include <QtQuick/QQuickItem>

#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "UVCDevices.h"
#include "UVCStreamMonitor.h"
#include "VideoScreenshot.h"

QGC_LOGGING_CATEGORY(UVCCameraControllerLog, "Video.UVCCameraController")

UVCCameraController::UVCCameraController(QObject* parent)
    : QObject(parent)
    , _monitor(new UVCStreamMonitor(this))
    , _mediaDevices(new QMediaDevices(this))
    , _retryTimer(new QTimer(this))
{
    _retryTimer->setSingleShot(true);
    _retryTimer->setInterval(kRetryIntervalMs);
    (void) connect(_retryTimer, &QTimer::timeout, this, &UVCCameraController::_recreateCamera);
    // Frames flowing again end a run of retries.
    (void) connect(_monitor, &UVCStreamMonitor::statsChanged, this, [this] {
        if (_monitor->stats().valid) {
            _retries = 0;
        }
    });
}

UVCCameraController::~UVCCameraController()
{
    _releaseCamera();
}

void UVCCameraController::setDevice(const QString& description)
{
    if (description == _deviceName) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "device" << description;
    _retryTimer->stop();
    _retries = 0;
    _deviceName = description;
    _recreateCamera();
}

void UVCCameraController::setFormat(const QString& name)
{
    if (name == _formatName) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "format" << name;
    _formatName = name;
    if (_camera) {
        _recreateCamera();
    }
}

void UVCCameraController::watchCameras()
{
    (void) connect(_mediaDevices, &QMediaDevices::videoInputsChanged, this, &UVCCameraController::_onVideoInputsChanged,
                   Qt::UniqueConnection);
}

void UVCCameraController::registerVideoOutput(QQuickItem* videoOutput)
{
    if (!videoOutput || (videoOutput == _videoOutput)) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "video output" << videoOutput;
    _videoOutput = videoOutput;
    _connectVideoOutput();
}

void UVCCameraController::unregisterVideoOutput(QQuickItem* videoOutput)
{
    if (!videoOutput || (videoOutput != _videoOutput)) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "video output removed" << videoOutput;
    _videoOutput = nullptr;
    _connectVideoOutput();
}

bool UVCCameraController::canTakeScreenshot() const
{
    return _camera && _camera->isActive() && _videoSink();
}

bool UVCCameraController::takeScreenshot(const QString& file) const
{
    QVideoSink* const videoSink = _videoSink();
    if (!videoSink) {
        qCWarning(UVCCameraControllerLog) << "No video output to take a screenshot from";
        return false;
    }
    return VideoScreenshot::save(videoSink->videoFrame(), file);
}

void UVCCameraController::_createCamera()
{
    const QCameraDevice device = UVCDevices::device(_deviceName);
    const QStringList formats = UVCDevices::formatNames(device);
    if (formats != _formats) {
        _formats = formats;
        emit formatsChanged();
    }
    if (_deviceName.isEmpty()) {
        return;
    }
    if (device.isNull()) {
        qCWarning(UVCCameraControllerLog) << "No camera named" << _deviceName;
        return;
    }

    UVCDevices::requestPermission();
    _cameraId = device.id();
    _camera = new QCamera(device, this);
    if (const QCameraFormat format = UVCDevices::format(device, _formatName); !format.isNull()) {
        qCDebug(UVCCameraControllerLog) << "camera format" << _formatName;
        _camera->setCameraFormat(format);
    }
    (void) connect(_camera, &QCamera::errorOccurred, this,
                   [this](QCamera::Error, const QString& errorString) { _onCameraError(errorString); });
    _captureSession = new QMediaCaptureSession(this);
    _captureSession->setCamera(_camera);
}

void UVCCameraController::_releaseCamera()
{
    _monitor->detach();
    delete _captureSession;
    _captureSession = nullptr;
    delete _camera;
    _camera = nullptr;
    _cameraId.clear();
}

void UVCCameraController::_recreateCamera()
{
    _releaseCamera();
    _createCamera();
    _connectVideoOutput();
}

void UVCCameraController::_connectVideoOutput()
{
    QVideoSink* const videoSink = _videoSink();
    if (_captureSession) {
        _captureSession->setVideoOutput(_videoOutput.data());
    }
    if (_camera && videoSink) {
        _monitor->attach(_camera, videoSink);
    } else {
        _monitor->detach();
    }
    if (_camera) {
        _camera->setActive(videoSink != nullptr);
    }
}

void UVCCameraController::_onCameraError(const QString& errorString)
{
    qCWarning(UVCCameraControllerLog) << "Camera error:" << _deviceName << errorString << "retries" << _retries;
    if (_retries >= kMaxRetries) {
        QGC::showAppMessage(tr("USB camera %1: %2").arg(_deviceName, errorString));
        return;
    }
    if (_retries == 0) {
        QGC::showAppMessage(tr("USB camera %1: %2. Retrying.").arg(_deviceName, errorString));
    }
    ++_retries;
    // Deferred: the camera emitting this error can't be deleted from its own signal.
    _retryTimer->start();
}

void UVCCameraController::_onVideoInputsChanged()
{
    if (!_cameraId.isEmpty() && !_cameraAttached()) {
        // Qt stops an unplugged camera without an error.
        qCWarning(UVCCameraControllerLog) << "Camera unplugged:" << _deviceName << _cameraId;
        QGC::showAppMessage(tr("USB camera %1 was disconnected").arg(_deviceName));
        // Runs another camera of the same name, if one is attached.
        _recreateCamera();
    } else if (!_deviceName.isEmpty() && !_camera && _cameraAttached()) {
        // Plugged back in.
        _recreateCamera();
    }
    emit camerasChanged();
}

bool UVCCameraController::_cameraAttached() const
{
    if (_cameraId.isEmpty()) {
        return UVCDevices::deviceExists(_deviceName);
    }
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    return std::any_of(cameras.cbegin(), cameras.cend(),
                       [this](const QCameraDevice& camera) { return camera.id() == _cameraId; });
}

QVideoSink* UVCCameraController::_videoSink() const
{
    return _videoOutput ? _videoOutput->property("videoSink").value<QVideoSink*>() : nullptr;
}
