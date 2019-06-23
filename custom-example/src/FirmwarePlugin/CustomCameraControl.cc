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

//-----------------------------------------------------------------------------
CustomCameraControl::CustomCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : QGCCameraControl(info, vehicle, compID, parent)
{
    connect(_vehicle,   &Vehicle::mavlinkMessageReceived,   this, &CustomCameraControl::_mavlinkMessageReceived);
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::takePhoto()
{
    bool res = false;
    res = QGCCameraControl::takePhoto();
    return res;
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::stopTakePhoto()
{
    bool res = QGCCameraControl::stopTakePhoto();
    return res;
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::startVideo()
{
    bool res = QGCCameraControl::startVideo();
    return res;
}

//-----------------------------------------------------------------------------
bool
CustomCameraControl::stopVideo()
{
    bool res = QGCCameraControl::stopVideo();
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
    QGCCameraControl::_setVideoStatus(status);
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_MOUNT_ORIENTATION:
            _handleGimbalOrientation(message);
            break;
    }
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::_handleGimbalOrientation(const mavlink_message_t& message)
{
    mavlink_mount_orientation_t o;
    mavlink_msg_mount_orientation_decode(&message, &o);
    if(fabsf(_gimbalRoll - o.roll) > 0.5f) {
        _gimbalRoll = o.roll;
        emit gimbalRollChanged();
    }
    if(fabsf(_gimbalPitch - o.pitch) > 0.5f) {
        _gimbalPitch = o.pitch;
        emit gimbalPitchChanged();
    }
    if(fabsf(_gimbalYaw - o.yaw) > 0.5f) {
        _gimbalYaw = o.yaw;
        emit gimbalYawChanged();
    }
    if(!_gimbalData) {
        _gimbalData = true;
        emit gimbalDataChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomCameraControl::handleCaptureStatus(const mavlink_camera_capture_status_t& cap)
{
    QGCCameraControl::handleCaptureStatus(cap);
}
