#include "SimulatedCameraControl.h"
#include "FlyViewSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(SimulatedCameraControlLog, "Camera.SimulatedCameraControl")

SimulatedCameraControl::SimulatedCameraControl(Vehicle *vehicle, QObject *parent)
    : MavlinkCameraControl(vehicle, parent)
{
    qCDebug(SimulatedCameraControlLog) << this;

    auto videoManager = VideoManager::instance();
    (void) connect(videoManager, &VideoManager::recordingChanged, this, [this](bool recording) {
        _videoCaptureStatusValue = recording ? VIDEO_CAPTURE_STATUS_RUNNING : VIDEO_CAPTURE_STATUS_STOPPED;
        emit videoCaptureStatusChanged();
    });

    (void) connect(videoManager, &VideoManager::hasVideoChanged, this, &SimulatedCameraControl::infoChanged);
    (void) connect(videoManager, &VideoManager::decodingChanged, this, &SimulatedCameraControl::infoChanged);

    (void) connect(videoManager, &VideoManager::hasVideoChanged, this, &SimulatedCameraControl::captureVideoStateChanged);
    (void) connect(videoManager, &VideoManager::decodingChanged, this, &SimulatedCameraControl::captureVideoStateChanged);

    (void) connect(videoManager, &VideoManager::recordingChanged, this, &SimulatedCameraControl::captureVideoStateChanged);
    (void) connect(this, &SimulatedCameraControl::videoCaptureStatusChanged, this, &SimulatedCameraControl::captureVideoStateChanged);
    (void) connect(this, &SimulatedCameraControl::photoCaptureStatusChanged, this, &SimulatedCameraControl::captureVideoStateChanged);
    (void) connect(this, &SimulatedCameraControl::cameraModeChanged, this, &SimulatedCameraControl::captureVideoStateChanged);
    (void) connect(this, &SimulatedCameraControl::photoCaptureStatusChanged, this, &SimulatedCameraControl::capturePhotosStateChanged);
    (void) connect(this, &SimulatedCameraControl::cameraModeChanged, this, &SimulatedCameraControl::capturePhotosStateChanged);

    (void) connect(SettingsManager::instance()->flyViewSettings()->showSimpleCameraControl(), &Fact::rawValueChanged, this, &SimulatedCameraControl::infoChanged);

    if (capturesVideo()) {
        _cameraMode = CAM_MODE_VIDEO;
    } else if (capturesPhotos()) {
        _cameraMode = CAM_MODE_PHOTO;
    } else {
        _cameraMode = CAM_MODE_UNDEFINED;
    }

    _videoRecordTimeUpdateTimer.setInterval(1000);
    (void) connect(&_videoRecordTimeUpdateTimer, &QTimer::timeout, this, &SimulatedCameraControl::recordTimeChanged);
}

SimulatedCameraControl::~SimulatedCameraControl()
{
    qCDebug(SimulatedCameraControlLog) << this;
}

QString SimulatedCameraControl::recordTimeStr() const
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString("hh:mm:ss");
}

void SimulatedCameraControl::setCameraMode(CameraMode cameraMode)
{
    qCDebug(CameraControlLog) << cameraModeToStr(cameraMode);

    if (!hasModes()) {
        qCWarning(CameraControlLog) << "Set camera mode denied - camera does not support modes";
        return;
    }

    switch (cameraMode) {
        case CAM_MODE_VIDEO:
            _setCameraMode(CAM_MODE_VIDEO);
            break;
        case CAM_MODE_PHOTO:
            _setCameraMode(CAM_MODE_PHOTO);
            break;
        default:
            qCWarning(CameraControlLog) << "Invalid mode" << cameraMode;
            break;
    }
}

void SimulatedCameraControl::_setCameraMode(CameraMode mode)
{
    if (_cameraMode != mode) {
        _cameraMode = mode;
        emit cameraModeChanged();
    }
}

void SimulatedCameraControl::toggleCameraMode()
{
    if (!hasModes()) {
        qCWarning(CameraControlLog) << "Toggle camera mode denied - camera does not support modes";
        return;
    }
    if ((_cameraMode == CAM_MODE_PHOTO) || (_cameraMode == CAM_MODE_SURVEY)) {
        setCameraModeVideo();
    } else if(_cameraMode == CAM_MODE_VIDEO) {
        setCameraModePhoto();
    }
}

bool SimulatedCameraControl::toggleVideoRecording()
{
    return ((_videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) ? stopVideoRecording() : startVideoRecording());
}

void SimulatedCameraControl::setCameraModeVideo()
{
    if (!hasModes()) {
        qCWarning(CameraControlLog) << "Camera does not support modes";
        return;
    }

    _setCameraMode(CAM_MODE_VIDEO);
}

void SimulatedCameraControl::setCameraModePhoto()
{
    if (!hasModes()) {
        qCWarning(CameraControlLog) << "Camera does not support modes";
        return;
    }

    _setCameraMode(CAM_MODE_PHOTO);
}

bool SimulatedCameraControl::takePhoto()
{
    if (!capturesPhotos()) {
        qCWarning(CameraControlLog) << "Camera does not handle image capture";
        return false;
    }

    if (_photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        qCWarning(CameraControlLog) << "Camera not idle";
        return false;
    }

    if ((_cameraMode != CAM_MODE_PHOTO) && (_cameraMode != CAM_MODE_SURVEY)) {
        qCWarning(CameraControlLog) << "Camera not in correct mode:" << cameraModeToStr(_cameraMode);
        return false;
    }

    switch (photoCaptureMode()) {
    case PHOTO_CAPTURE_SINGLE:
        _vehicle->triggerSimpleCamera();
        _photoCaptureStatusValue = PHOTO_CAPTURE_IN_PROGRESS;
        emit photoCaptureStatusChanged();
        QTimer::singleShot(500, this, [this]() { _photoCaptureStatusValue = PHOTO_CAPTURE_IDLE; emit photoCaptureStatusChanged(); });
        return true;
    case PHOTO_CAPTURE_TIMELAPSE:
        qgcApp()->showAppMessage(tr("Time lapse capture not supported by this camera"));
    default:
        break;
    }

    return false;
}

bool SimulatedCameraControl::startVideoRecording()
{
    if (!capturesVideo()) {
        qCWarning(CameraControlLog) << "Camera does not handle video capture";
        return false;
    }
    if (_cameraMode == CAM_MODE_PHOTO) {
        qCWarning(CameraControlLog) << "Camera does not take video in photo mode";
        return false;
    }
    if (_videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        qCWarning(CameraControlLog) << "Camera already recording";
        return false;
    }

    _videoRecordTimeUpdateTimer.start();
    _videoRecordTimeElapsedTimer.start();
    VideoManager::instance()->startRecording();
    return true;
}

bool SimulatedCameraControl::stopVideoRecording()
{
    if (_videoCaptureStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
        qCWarning(CameraControlLog) << "Camera not recording";
        return false;
    }

    _videoRecordTimeUpdateTimer.stop();
    VideoManager::instance()->stopRecording();
    return true;
}

quint32 SimulatedCameraControl::recordTime() const
{
    return (_videoRecordTimeUpdateTimer.isActive() ? _videoRecordTimeElapsedTimer.elapsed() : 0);
}

bool SimulatedCameraControl::capturesVideo() const
{
    return VideoManager::instance()->hasVideo();
}

bool SimulatedCameraControl::capturesPhotos() const
{
    return SettingsManager::instance()->flyViewSettings()->showSimpleCameraControl()->rawValue().toBool();
}


bool SimulatedCameraControl::hasModes() const
{
    return (capturesPhotos() && capturesVideo());
}

bool SimulatedCameraControl::hasVideoStream() const
{
    return VideoManager::instance()->decoding();
}

MavlinkCameraControl::CaptureVideoState SimulatedCameraControl::captureVideoState() const
{
    if (!capturesVideo()) {
        return CaptureVideoStateDisabled;
    }
    if (_videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING || VideoManager::instance()->recording()) {
        return CaptureVideoStateCapturing;
    }
    if (_photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        return CaptureVideoStateDisabled;
    }
    return CaptureVideoStateIdle;
}

MavlinkCameraControl::CapturePhotosState SimulatedCameraControl::capturePhotosState() const
{
    if (_photoCaptureStatus() == PHOTO_CAPTURE_IN_PROGRESS) {
        return CapturePhotosStateCapturingSinglePhoto;
    }
    if (_photoCaptureStatus() == PHOTO_CAPTURE_INTERVAL_IN_PROGRESS || _photoCaptureStatus() == PHOTO_CAPTURE_INTERVAL_IDLE) {
        return CapturePhotosStateCapturingMultiplePhotos;
    }
    return capturesPhotos() ? CapturePhotosStateIdle : CapturePhotosStateDisabled;
}

void SimulatedCameraControl::setPhotoCaptureMode(MavlinkCameraControl::PhotoCaptureMode photoCaptureMode)
{
    if (photoCaptureMode == PHOTO_CAPTURE_TIMELAPSE) {
        qCWarning(CameraControlLog) << "Time lapse capture not supported by simulated camera";
        return;
    }
    if (_photoCaptureMode != photoCaptureMode) {
        _photoCaptureMode = photoCaptureMode;
        emit photoCaptureModeChanged();
    }
}
