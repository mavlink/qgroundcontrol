/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
        _videoCaptureStatus = recording ? VIDEO_CAPTURE_STATUS_RUNNING : VIDEO_CAPTURE_STATUS_STOPPED;
        emit videoCaptureStatusChanged();
    });
    connect(videoManager, &VideoManager::hasVideoChanged, this, &SimulatedCameraControl::infoChanged);

    (void) connect(SettingsManager::instance()->flyViewSettings()->showSimpleCameraControl(), &Fact::rawValueChanged, this, &SimulatedCameraControl::infoChanged);

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

void SimulatedCameraControl::setCameraMode(CameraMode mode)
{
    qCDebug(CameraControlLog) << cameraModeToStr(mode);

    if (!hasModes()) {
        qCWarning(CameraControlLog) << "called when camera does not support modes";
        return;
    }

    switch (mode) {
        case CAM_MODE_VIDEO:
            _setCameraMode(CAM_MODE_VIDEO);
            break;
        case CAM_MODE_PHOTO:
            _setCameraMode(CAM_MODE_PHOTO);
            break;
        default:
            qCWarning(CameraControlLog) << "invalid mode" << mode;
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
    if ((cameraMode() == CAM_MODE_PHOTO) || (cameraMode() == CAM_MODE_SURVEY)) {
        setCameraModeVideo();
    } else if(cameraMode() == CAM_MODE_VIDEO) {
        setCameraModePhoto();
    }
}

bool SimulatedCameraControl::toggleVideoRecording()
{
    return ((videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) ? stopVideoRecording() : startVideoRecording());
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

    if (photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        qCWarning(CameraControlLog) << "Camera not idle";
        return false;
    }

    if ((cameraMode() != CAM_MODE_PHOTO) && (cameraMode() != CAM_MODE_SURVEY)) {
        qCWarning(CameraControlLog) << "Camera not in correct mode:" << cameraModeToStr(cameraMode());
        return false;
    }

    switch (photoCaptureMode()) {
    case PHOTO_CAPTURE_SINGLE:
        _vehicle->triggerSimpleCamera();
        _photoCaptureStatus = PHOTO_CAPTURE_IN_PROGRESS;
        emit photoCaptureStatusChanged();
        QTimer::singleShot(500, [this]() { _photoCaptureStatus = PHOTO_CAPTURE_IDLE; emit photoCaptureStatusChanged(); });
        break;
    case PHOTO_CAPTURE_TIMELAPSE:
        qgcApp()->showAppMessage(tr("Time lapse capture not supported by this camera"));
        break;
    default:
        break;
    }

    return true;
}

bool SimulatedCameraControl::startVideoRecording()
{
    if (!capturesVideo()) {
        qCWarning(CameraControlLog) << "Camera does not handle video capture";
        return false;
    }
    if (cameraMode() == CAM_MODE_PHOTO) {
        qCWarning(CameraControlLog) << "Camera does not take video in photo mode";
        return false;
    }
    if (videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        qCWarning(CameraControlLog) << "Camera already recording";
        return false;
    }

    _videoRecordTimeUpdateTimer.start();
    _videoRecordTimeElapsedTimer.start();
    VideoManager::instance()->startRecording();
    return false;
}

bool SimulatedCameraControl::stopVideoRecording()
{
    if (videoCaptureStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
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

bool SimulatedCameraControl::hasVideoStream() const
{
    return VideoManager::instance()->hasVideo();
}

void SimulatedCameraControl::setPhotoCaptureMode(MavlinkCameraControl::PhotoCaptureMode photoCaptureMode)
{
    if (_photoCaptureMode != photoCaptureMode) {
        _photoCaptureMode = photoCaptureMode;
        emit photoCaptureModeChanged();
    }
}
