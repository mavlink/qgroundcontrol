#include "UVCCameraController.h"

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
{}

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
    _releaseCamera();
    _deviceName = description;
    _createCamera();
    _connectVideoOutput();
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
    if (_deviceName.isEmpty()) {
        return;
    }
    const QCameraDevice device = UVCDevices::device(_deviceName);
    if (device.isNull()) {
        qCWarning(UVCCameraControllerLog) << "No camera named" << _deviceName;
        return;
    }

    UVCDevices::requestPermission();
    _camera = new QCamera(device, this);
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
    qCWarning(UVCCameraControllerLog) << "Camera error:" << _deviceName << errorString;
    QGC::showAppMessage(tr("USB camera %1: %2").arg(_deviceName, errorString));
}

void UVCCameraController::_onVideoInputsChanged()
{
    // Qt stops an unplugged camera without an error.
    if (!_deviceName.isEmpty() && !UVCDevices::deviceExists(_deviceName)) {
        qCWarning(UVCCameraControllerLog) << "Camera unplugged:" << _deviceName;
        QGC::showAppMessage(tr("USB camera %1 was disconnected").arg(_deviceName));
    }
    emit camerasChanged();
}

QVideoSink* UVCCameraController::_videoSink() const
{
    return _videoOutput ? _videoOutput->property("videoSink").value<QVideoSink*>() : nullptr;
}
