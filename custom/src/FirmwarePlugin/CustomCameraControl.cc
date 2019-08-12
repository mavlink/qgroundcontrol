/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "CustomCameraControl.h"
#include "QGCCameraIO.h"

QGC_LOGGING_CATEGORY(CustomCameraLog, "CustomCameraLog")
QGC_LOGGING_CATEGORY(CustomCameraVerboseLog, "CustomCameraVerboseLog")

static const char* kCAM_IRPALETTE            = "CAM_IRPALETTE";
static const char* kCAM_NEXTVISION_IRPALETTE = "IR_SENS_POL";
static const char* kCAM_ENC                  = "CAM_ENC";

//-----------------------------------------------------------------------------
CustomCameraControl::CustomCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : QGCCameraControl(info, vehicle, compID, parent)
{
    _cameraSound.setSource(QUrl::fromUserInput("qrc:/custom/wav/camera.wav"));
    _cameraSound.setLoopCount(1);
    _cameraSound.setVolume(0.9);
    _videoSound.setSource(QUrl::fromUserInput("qrc:/custom/wav/beep.wav"));
    _videoSound.setVolume(0.9);
    _errorSound.setSource(QUrl::fromUserInput("qrc:/custom/wav/boop.wav"));
    _errorSound.setVolume(0.9);
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::takePhoto()
{
    bool res = false;
    if(cameraMode() == CAM_MODE_VIDEO && _photoMode == PHOTO_CAPTURE_TIMELAPSE) {
        //-- In video mode, we disable time lapse
        QGCCameraControl::setPhotoMode(PHOTO_CAPTURE_SINGLE);
    }
    res = QGCCameraControl::takePhoto();
    if(res) {
        if(photoMode() == PHOTO_CAPTURE_TIMELAPSE) {
            _firstPhotoLapse = true;
        }
        _cameraSound.setLoopCount(1);
        _cameraSound.play();
    } else {
        _errorSound.setLoopCount(1);
        _errorSound.play();
    }
    return res;
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::stopTakePhoto()
{
    bool res = QGCCameraControl::stopTakePhoto();
    if(res) {
        _videoSound.setLoopCount(2);
        _videoSound.play();
    } else {
        _errorSound.setLoopCount(2);
        _errorSound.play();
    }
    return res;
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::startVideo()
{
    bool res = QGCCameraControl::startVideo();
    if(!res) {
        _errorSound.setLoopCount(1);
        _errorSound.play();
    }
    return res;
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::stopVideo()
{
    bool res = QGCCameraControl::stopVideo();
    if(!res) {
        _errorSound.setLoopCount(1);
        _errorSound.play();
    }
    return res;
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::setVideoMode()
{
    if(cameraMode() != CAM_MODE_VIDEO) {
        qCDebug(CustomCameraLog) << "setVideoMode()";
        Fact* pFact = getFact(kCAM_MODE);
        if(pFact) {
            pFact->setRawValue(CAM_MODE_VIDEO);
            _setCameraMode(CAM_MODE_VIDEO);
        }
    }
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::setPhotoMode()
{
    if(cameraMode() != CAM_MODE_PHOTO) {
        qCDebug(CustomCameraLog) << "setPhotoMode()";
        Fact* pFact = getFact(kCAM_MODE);
        if(pFact) {
            pFact->setRawValue(CAM_MODE_PHOTO);
            _setCameraMode(CAM_MODE_PHOTO);
        }
    }
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::_setVideoStatus(VideoStatus status)
{
    VideoStatus oldStatus = videoStatus();
    QGCCameraControl::_setVideoStatus(status);
    if(oldStatus != status) {
        if(status == VIDEO_CAPTURE_STATUS_RUNNING) {
            _videoSound.setLoopCount(1);
            _videoSound.play();
        } else {
            if(oldStatus == VIDEO_CAPTURE_STATUS_UNDEFINED) {
                //-- System just booted and it's ready
                _videoSound.setLoopCount(1);
            } else {
                //-- Stop recording
                _videoSound.setLoopCount(2);
            }
            _videoSound.play();
        }
    }
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::handleCaptureStatus(const mavlink_camera_capture_status_t& cap)
{
    QGCCameraControl::handleCaptureStatus(cap);
    //-- Update recording time
    if(photoStatus() == PHOTO_CAPTURE_INTERVAL_IDLE || photoStatus() == PHOTO_CAPTURE_INTERVAL_IN_PROGRESS) {
        //-- Skip camera sound on first response (already did it when the user clicked it)
        if(_firstPhotoLapse) {
            _firstPhotoLapse = false;
        } else {
            _cameraSound.setLoopCount(1);
            _cameraSound.play();
        }
    }
}

//-----------------------------------------------------------------------------
Fact*
CustomCameraControl::irPalette()
{
    if(_paramComplete) {
        if(_activeSettings.contains(kCAM_IRPALETTE)) {
            return getFact(kCAM_IRPALETTE);
        }
        else if(_activeSettings.contains(kCAM_NEXTVISION_IRPALETTE)) {
            return getFact(kCAM_NEXTVISION_IRPALETTE);
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
Fact*
CustomCameraControl::videoEncoding()
{
    return _paramComplete ? getFact(kCAM_ENC) : nullptr;
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::setThermalMode(ThermalViewMode mode)
{
    if(_paramComplete) {
        if(vendor() == "NextVision" && _activeSettings.contains("CAM_SENSOR")) {
            if(mode == THERMAL_FULL) {
                getFact("CAM_SENSOR")->setRawValue(1);
            }
            else if(mode == THERMAL_OFF) {
                getFact("CAM_SENSOR")->setRawValue(0);
            }
        }
    }
    QGCCameraControl::setThermalMode(mode);
}
