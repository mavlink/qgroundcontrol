/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SimulatedCameraControl.h"
#include "VideoManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "FlyViewSettings.h"
#include "Vehicle.h"

#include <QtQml/QQmlEngine>

//-----------------------------------------------------------------------------
SimulatedCameraControl::SimulatedCameraControl(Vehicle* vehicle, QObject* parent)
    : MavlinkCameraControl  (parent)
    , _vehicle              (vehicle)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    connect(VideoManager::instance(), &VideoManager::recordingChanged, this, &SimulatedCameraControl::videoCaptureStatusChanged);

    auto flyViewSettings = SettingsManager::instance()->flyViewSettings();
    connect(flyViewSettings->showSimpleCameraControl(), &Fact::rawValueChanged, this, &SimulatedCameraControl::infoChanged);

    _videoRecordTimeUpdateTimer.setInterval(1000);
    connect(&_videoRecordTimeUpdateTimer, &QTimer::timeout, this, &SimulatedCameraControl::recordTimeChanged);
}

SimulatedCameraControl::~SimulatedCameraControl()
{

}

QString SimulatedCameraControl::recordTimeStr()
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString("hh:mm:ss");
}

SimulatedCameraControl::VideoCaptureStatus SimulatedCameraControl::videoCaptureStatus()
{
    return _videoCaptureStatus = VideoManager::instance()->recording() ? VIDEO_CAPTURE_STATUS_RUNNING : VIDEO_CAPTURE_STATUS_STOPPED;
}

void SimulatedCameraControl::setCameraMode(CameraMode mode)
{
    qCDebug(CameraControlLog) << "setCameraMode" << cameraModeToStr(mode);

    if (hasModes()) {
        if (mode == CAM_MODE_VIDEO) {
            _setCameraMode(CAM_MODE_VIDEO);
        } else if (mode == CAM_MODE_PHOTO) {
            _setCameraMode(CAM_MODE_PHOTO);
        } else {
            qCWarning(CameraControlLog) << "setCameraMode invalid mode" << mode;
        }
    } else {
        qCWarning(CameraControlLog) << "setCameraMode called when camera does not support modes";
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
    if(cameraMode() == CAM_MODE_PHOTO || cameraMode() == CAM_MODE_SURVEY) {
        setCameraModeVideo();
    } else if(cameraMode() == CAM_MODE_VIDEO) {
        setCameraModePhoto();
    }
}

bool SimulatedCameraControl::toggleVideoRecording()
{
    if(videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        return stopVideoRecording();
    } else {
        return startVideoRecording();
    }
}

void SimulatedCameraControl::setCameraModeVideo()
{
    qCDebug(CameraControlLog) << "setCameraModeVideo()";

    if (!hasModes()) {
        qCWarning(CameraControlLog) << "setCameraModeVideo: Camera does not support modes";
        return;
    }

    _setCameraMode(CAM_MODE_VIDEO);
}

void SimulatedCameraControl::setCameraModePhoto()
{
    qCDebug(CameraControlLog) << "setCameraModePhoto()";

    if (!hasModes()) {
        qCWarning(CameraControlLog) << "setCameraModePhoto: Camera does not support modes";
        return;
    }

    _setCameraMode(CAM_MODE_PHOTO);
}


bool SimulatedCameraControl::takePhoto()
{
    qCDebug(CameraControlLog) << "takePhoto()";

    if (!capturesPhotos()) {
        qCWarning(CameraControlLog) << "takePhoto: Camera does not handle image capture";
        return false;
    }
    if (photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        qCWarning(CameraControlLog) << "Camera not idle";
        return false;
    }
    if (cameraMode() != CAM_MODE_PHOTO && cameraMode() != CAM_MODE_SURVEY) {
        qCWarning(CameraControlLog) << "takePhoto: Camera not in correct mode:" << cameraModeToStr(cameraMode());
        return false;
    }

    if (photoCaptureMode() == PHOTO_CAPTURE_SINGLE) {
        _vehicle->triggerSimpleCamera();
        _photoCaptureStatus = PHOTO_CAPTURE_IN_PROGRESS;
        emit photoCaptureStatusChanged();
        QTimer::singleShot(500, [this]() { _photoCaptureStatus = PHOTO_CAPTURE_IDLE; emit photoCaptureStatusChanged(); });
    } else if (photoCaptureMode() == PHOTO_CAPTURE_TIMELAPSE) {
        qgcApp()->showAppMessage(tr("Time lapse capture not supported by this camera"));
    }

    return true;
}

bool SimulatedCameraControl::startVideoRecording()
{
    qCDebug(CameraControlLog) << "startVideoRecording()";

    if (!capturesVideo()) {
        qCWarning(CameraControlLog) << "startVideoRecording: Camera does not handle video capture";
        return false;
    }
    if (cameraMode() == CAM_MODE_PHOTO) {
        qCWarning(CameraControlLog) << "startVideoRecording: Camera does not take video in photo mode";
        return false;
    }
    if(videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        qCWarning(CameraControlLog) << "startVideoRecording: Camera already recording";
        return false;
    }

    _videoRecordTimeUpdateTimer.start();
    _videoRecordTimeElapsedTimer.start();
    VideoManager::instance()->startRecording();
    return false;
}

bool SimulatedCameraControl::stopVideoRecording()
{
    qCDebug(CameraControlLog) << "stopVideoRecording()";

    if(videoCaptureStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
        qCWarning(CameraControlLog) << "stopVideoRecording: Camera not recording";
        return false;
    }

    _videoRecordTimeUpdateTimer.stop();
    VideoManager::instance()->stopRecording();
    return true;
}

quint32  SimulatedCameraControl::recordTime()
{
    if (_videoRecordTimeUpdateTimer.isActive()) {
        return _videoRecordTimeElapsedTimer.elapsed();
    } else {
        return 0;
    }
}

bool SimulatedCameraControl::capturesVideo()
{
    return VideoManager::instance()->hasVideo();
}

void SimulatedCameraControl::setPhotoLapse(double)
{
    // FIXME: NYI
}

bool SimulatedCameraControl::capturesPhotos()
{
    return SettingsManager::instance()->flyViewSettings()->showSimpleCameraControl()->rawValue().toBool();
}

bool SimulatedCameraControl::hasVideoStream()
{
    return VideoManager::instance()->hasVideo();
}

void SimulatedCameraControl::setPhotoLapseCount(int)
{
    // FIXME: NYI
}

void SimulatedCameraControl::setPhotoCaptureMode(MavlinkCameraControl::PhotoCaptureMode photoCaptureMode)
{
    if (_photoCaptureMode != photoCaptureMode) {
        _photoCaptureMode = photoCaptureMode;
        emit photoCaptureModeChanged();
    }
}

bool SimulatedCameraControl::hasModes()
{
    if (capturesPhotos() && capturesVideo()) {
        return true;
    } else {
        return false;
    }
}
