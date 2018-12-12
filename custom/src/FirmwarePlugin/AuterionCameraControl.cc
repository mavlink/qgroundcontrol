/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "AuterionCameraControl.h"
#include "QGCCameraIO.h"

QGC_LOGGING_CATEGORY(AuterionCameraLog, "AuterionCameraLog")
QGC_LOGGING_CATEGORY(AuterionCameraLogVerbose, "AuterionCameraLogVerbose")

static const char *kCAM_MODE = "CAM_MODE";

//-----------------------------------------------------------------------------
AuterionCameraControl::AuterionCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : QGCCameraControl(info, vehicle, compID, parent)
{
    _cameraSound.setSource(QUrl::fromUserInput("qrc:/auterion/wav/camera.wav"));
    _cameraSound.setLoopCount(1);
    _cameraSound.setVolume(0.9);
    _videoSound.setSource(QUrl::fromUserInput("qrc:/auterion/wav/beep.wav"));
    _videoSound.setVolume(0.9);
    _errorSound.setSource(QUrl::fromUserInput("qrc:/auterion/wav/boop.wav"));
    _errorSound.setVolume(0.9);

    _recTimer.setSingleShot(false);
    _recTimer.setInterval(333);

    connect(&_recTimer, &QTimer::timeout,                   this, &AuterionCameraControl::_recTimerHandler);
    connect(_vehicle,   &Vehicle::mavlinkMessageReceived,   this, &AuterionCameraControl::_mavlinkMessageReceived);
}

//-----------------------------------------------------------------------------
QString
AuterionCameraControl::recordTimeStr()
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString("hh:mm:ss");
}

//-----------------------------------------------------------------------------
bool
AuterionCameraControl::takePhoto()
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
AuterionCameraControl::stopTakePhoto()
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
AuterionCameraControl::startVideo()
{
    // Disabled for now
    _errorSound.setLoopCount(2);
    _errorSound.play();
    return false;

    /*
    bool res = QGCCameraControl::startVideo();
    if(!res) {
        _errorSound.setLoopCount(1);
        _errorSound.play();
    }
    return res;
    */
}

//-----------------------------------------------------------------------------
bool
AuterionCameraControl::stopVideo()
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
AuterionCameraControl::setVideoMode()
{
    if(cameraMode() != CAM_MODE_VIDEO) {
        qCDebug(AuterionCameraLog) << "setVideoMode()";
        Fact* pFact = getFact(kCAM_MODE);
        if(pFact) {
            pFact->setRawValue(CAM_MODE_VIDEO);
            _setCameraMode(CAM_MODE_VIDEO);
        }
    }
}

//-----------------------------------------------------------------------------
void
AuterionCameraControl::setPhotoMode()
{
    if(cameraMode() != CAM_MODE_PHOTO) {
        qCDebug(AuterionCameraLog) << "setPhotoMode()";
        Fact* pFact = getFact(kCAM_MODE);
        if(pFact) {
            pFact->setRawValue(CAM_MODE_PHOTO);
            _setCameraMode(CAM_MODE_PHOTO);
        }
    }
}

//-----------------------------------------------------------------------------
void
AuterionCameraControl::_setVideoStatus(VideoStatus status)
{
    VideoStatus oldStatus = videoStatus();
    QGCCameraControl::_setVideoStatus(status);
    if(oldStatus != status) {
        if(status == VIDEO_CAPTURE_STATUS_RUNNING) {
            _recordTime = 0;
            _recTime.start();
            _recTimer.start();
            _videoSound.setLoopCount(1);
            _videoSound.play();
        } else {
            _recTimer.stop();
            _recordTime = 0;
            emit recordTimeChanged();
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
AuterionCameraControl::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_MOUNT_ORIENTATION:
            _handleGimbalOrientation(message);
            break;
    }
}

//-----------------------------------------------------------------------------
void
AuterionCameraControl::_handleGimbalOrientation(const mavlink_message_t& message)
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
AuterionCameraControl::_recTimerHandler()
{
    _recordTime = static_cast<uint32_t>(_recTime.elapsed());
    emit recordTimeChanged();
}

//-----------------------------------------------------------------------------
void
AuterionCameraControl::handleCaptureStatus(const mavlink_camera_capture_status_t& cap)
{
    QGCCameraControl::handleCaptureStatus(cap);
    //-- Update recording time
    if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _recordTime = cap.recording_time_ms;
        _recTime = _recTime.addMSecs(_recTime.elapsed() - static_cast<int>(cap.recording_time_ms));
        emit recordTimeChanged();
    } else if(photoStatus() == PHOTO_CAPTURE_INTERVAL_IDLE || photoStatus() == PHOTO_CAPTURE_INTERVAL_IN_PROGRESS) {
        //-- Skip camera sound on first response (already did it when the user clicked it)
        if(_firstPhotoLapse) {
            _firstPhotoLapse = false;
        } else {
            _cameraSound.setLoopCount(1);
            _cameraSound.play();
        }
    }
}
