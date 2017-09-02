/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "YuneecCameraControl.h"
#include "QGCCameraIO.h"
#include "TyphoonHPlugin.h"
#include "TyphoonHM4Interface.h"
#include "VideoManager.h"
#include "SettingsManager.h"
#include "VideoManager.h"
#include "Settings/SettingsManager.h"
#include "px4_custom_mode.h"

QGC_LOGGING_CATEGORY(YuneecCameraLog, "YuneecCameraLog")
QGC_LOGGING_CATEGORY(YuneecCameraLogVerbose, "YuneecCameraLogVerbose")

static const char *kCAM_ASPECTRATIO = "CAM_ASPECTRATIO";
//static const char *kCAM_AUDIOREC    = "CAM_AUDIOREC";
//static const char *kCAM_COLORMODE   = "CAM_COLORMODE";
static const char *kCAM_EV          = "CAM_EV";
static const char *kCAM_EXPMODE     = "CAM_EXPMODE";
//static const char *kCAM_FLICKER     = "CAM_FLICKER";
static const char *kCAM_ISO         = "CAM_ISO";
static const char *kCAM_METERING    = "CAM_METERING";
static const char *kCAM_MODE        = "CAM_MODE";
//static const char *kCAM_PHOTOFMT    = "CAM_PHOTOFMT";
//static const char *kCAM_PHOTOQUAL   = "CAM_PHOTOQUAL";
static const char *kCAM_SHUTTERSPD  = "CAM_SHUTTERSPD";
static const char *kCAM_SPOTAREA    = "CAM_SPOTAREA";
//static const char *kCAM_VIDFMT      = "CAM_VIDFMT";
static const char *kCAM_VIDRES      = "CAM_VIDRES";
static const char *kCAM_WBMODE      = "CAM_WBMODE";

//-----------------------------------------------------------------------------
YuneecCameraControl::YuneecCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : QGCCameraControl(info, vehicle, compID, parent)
    , _vehicle(vehicle)
    , _gimbalCalOn(false)
    , _gimbalProgress(0)
    , _gimbalRoll(0.0f)
    , _gimbalPitch(0.0f)
    , _gimbalYaw(0.0f)
    , _gimbalData(false)
    , _recordTime(0)
    , _paramComplete(false)
    , _isE90(false)
    , _inMissionMode(false)
{

    _cameraSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/wav/camera.wav"));
    _cameraSound.setLoopCount(1);
    _cameraSound.setVolume(0.9);
    _videoSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/wav/beep.wav"));
    _videoSound.setVolume(0.9);
    _errorSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/wav/boop.wav"));
    _errorSound.setVolume(0.9);

    _recTimer.setSingleShot(false);
    _recTimer.setInterval(333);
    _gimbalTimer.setSingleShot(true);

    connect(&_recTimer,     &QTimer::timeout, this, &YuneecCameraControl::_recTimerHandler);
    connect(&_gimbalTimer,  &QTimer::timeout, this, &YuneecCameraControl::_gimbalCalTimeout);
    connect(_vehicle,   &Vehicle::mavlinkMessageReceived,   this, &YuneecCameraControl::_mavlinkMessageReceived);
    connect(this,       &QGCCameraControl::parametersReady, this, &YuneecCameraControl::_parametersReady);

    TyphoonHPlugin* pPlug = dynamic_cast<TyphoonHPlugin*>(qgcApp()->toolbox()->corePlugin());
    if(pPlug && pPlug->handler()) {
        connect(pPlug->handler(), &TyphoonHM4Interface::switchStateChanged, this, &YuneecCameraControl::_switchStateChanged);
    }

    //-- Get Gimbal Version
    _vehicle->sendMavCommand(
        MAV_COMP_ID_GIMBAL,                         // Target component
        MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES,     // Command id
        true,                                       // ShowError
        1);                                         // Request gimbal version
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_parametersReady()
{
    if(!_paramComplete) {
        qCDebug(YuneecCameraLog) << "All parameters loaded.";
        _paramComplete = true;
        emit factsLoaded();
        _isE90 = modelName().contains("E90");
    }
}

//-----------------------------------------------------------------------------
QString
YuneecCameraControl::firmwareVersion()
{
    if(_version.isEmpty()) {
        char cntry = (_info.firmware_version >> 24) & 0xFF;
        int  build = (_info.firmware_version >> 16) & 0xFF;
        int  minor = (_info.firmware_version >>  8) & 0xFF;
        int  major = _info.firmware_version & 0xFF;
        _version.sprintf("%d.%d.%d_%c", major, minor, build, cntry);
    }
    return _version;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::exposureMode()
{
    return _paramComplete ? getFact(kCAM_EXPMODE) : NULL;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::ev()
{
    return _paramComplete ? getFact(kCAM_EV) : NULL;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::iso()
{
    return _paramComplete ? getFact(kCAM_ISO) : NULL;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::shutterSpeed()
{
    return _paramComplete ? getFact(kCAM_SHUTTERSPD) : NULL;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::wb()
{
    return _paramComplete ? getFact(kCAM_WBMODE) : NULL;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::meteringMode()
{
    return _paramComplete ? getFact(kCAM_METERING) : NULL;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::videoRes()
{
    return _paramComplete ? getFact(kCAM_VIDRES) : NULL;
}

//-----------------------------------------------------------------------------
Fact*
YuneecCameraControl::aspectRatio()
{
    return _paramComplete ? getFact(kCAM_ASPECTRATIO) : NULL;
}

//-----------------------------------------------------------------------------
QString
YuneecCameraControl::recordTimeStr()
{
    return QTime(0, 0).addMSecs(recordTime()).toString("hh:mm:ss");
}

//-----------------------------------------------------------------------------
bool
YuneecCameraControl::takePhoto()
{
    bool res = QGCCameraControl::takePhoto();
    if(res) {
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
YuneecCameraControl::startVideo()
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
YuneecCameraControl::stopVideo()
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
YuneecCameraControl::setVideoMode()
{
    if(cameraMode() != CAM_MODE_VIDEO) {
        qCDebug(YuneecCameraLog) << "setVideoMode()";
        Fact* pFact = getFact(kCAM_MODE);
        if(pFact) {
            pFact->setRawValue(CAM_MODE_VIDEO);
            _setCameraMode(CAM_MODE_VIDEO);
        }
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::setPhotoMode()
{
    if(cameraMode() != CAM_MODE_PHOTO) {
        qCDebug(YuneecCameraLog) << "setPhotoMode()";
        Fact* pFact = getFact(kCAM_MODE);
        if(pFact) {
            pFact->setRawValue(CAM_MODE_PHOTO);
            _setCameraMode(CAM_MODE_PHOTO);
        }
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::calibrateGimbal()
{
    if(_vehicle) {
        //-- We can currently calibrate the accelerometer.
        _vehicle->sendMavCommand(
            MAV_COMP_ID_GIMBAL,
            MAV_CMD_PREFLIGHT_CALIBRATION,
            true,
            0,0,0,0,1,0,0);
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_setVideoStatus(VideoStatus status)
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
            //-- Start recording local stream as well
            //if(qgcApp()->toolbox()->videoManager()->videoReceiver()) {
            //    qgcApp()->toolbox()->videoManager()->videoReceiver()->startRecording();
            //}
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
            //-- Stop recording local stream
            //if(qgcApp()->toolbox()->videoManager()->videoReceiver()) {
            //    qgcApp()->toolbox()->videoManager()->videoReceiver()->stopRecording();
            //}
        }
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
            _handleHardwareVersion(message);
            break;
        case MAVLINK_MSG_ID_MOUNT_ORIENTATION:
            _handleGimbalOrientation(message);
            break;
        case MAVLINK_MSG_ID_COMMAND_ACK:
            _handleCommandAck(message);
            break;
        case MAVLINK_MSG_ID_HEARTBEAT:
            _handleHeartBeat(message);
        break;
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_handleHardwareVersion(const mavlink_message_t& message)
{
    mavlink_autopilot_version_t gimbal_version;
    mavlink_msg_autopilot_version_decode(&message, &gimbal_version);
    if (message.compid == MAV_COMP_ID_GIMBAL) {
        int major = (gimbal_version.flight_sw_version >> (8 * 3)) & 0xFF;
        int minor = (gimbal_version.flight_sw_version >> (8 * 2)) & 0xFF;
        int patch = (gimbal_version.flight_sw_version >> (8 * 1)) & 0xFF;
        _gimbalVersion.sprintf("%d.%d.%d", major, minor, patch);
        qCDebug(YuneecCameraLog) << _gimbalVersion;
        emit gimbalVersionChanged();
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_handleGimbalOrientation(const mavlink_message_t& message)
{
    mavlink_mount_orientation_t o;
    mavlink_msg_mount_orientation_decode(&message, &o);
    if(fabs(_gimbalRoll - o.roll) > 0.5) {
        _gimbalRoll = o.roll;
        emit gimbalRollChanged();
    }
    if(fabs(_gimbalPitch - o.pitch) > 0.5) {
        _gimbalPitch = o.pitch;
        emit gimbalPitchChanged();
    }
    if(fabs(_gimbalYaw - o.yaw) > 0.5) {
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
YuneecCameraControl::_handleHeartBeat(const mavlink_message_t& message)
{
    if(message.compid == _vehicle->defaultComponentId()) {
        mavlink_heartbeat_t hb;
        mavlink_msg_heartbeat_decode(&message, &hb);
        px4_custom_mode *cm = (px4_custom_mode *)(void *)&hb.custom_mode;
        //-- Transition out of mission mode
        if (cm->sub_mode != PX4_CUSTOM_SUB_MODE_AUTO_MISSION && _inMissionMode)
        {
            qCDebug(YuneecCameraLog) << "Transition out of mission mode.";
            _inMissionMode = false;
            _requestAllParameters();
        }
        //-- Transition into mission mode
        else if (cm->sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_MISSION && !_inMissionMode)
        {
            qCDebug(YuneecCameraLog) << "Transition into mission mode.";
            _inMissionMode = true;
        }
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_handleCommandAck(const mavlink_message_t& message)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&message, &ack);
    if(ack.command == MAV_CMD_PREFLIGHT_CALIBRATION) {
        if(message.compid == MAV_COMP_ID_GIMBAL) {
            _handleGimbalResult(ack.result, ack.progress);
        }
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_handleGimbalResult(uint16_t result, uint8_t progress)
{
    if(_gimbalCalOn) {
        if(progress == 255) {
            _gimbalTimer.stop();
            _gimbalProgress = 100;
            _gimbalCalOn = false;
            emit gimbalCalOnChanged();
        }
    } else {
        if(progress && progress < 255) {
            _gimbalCalOn = true;
            emit gimbalCalOnChanged();
        }
    }
    if(progress < 255) {
        _gimbalProgress = progress;
        if(progress == 99) {
            _gimbalTimer.stop();
            _gimbalTimer.start(5000);
        }
    }
    emit gimbalProgressChanged();
    qCDebug(YuneecCameraLog) << "Gimbal Calibration" << QDateTime::currentDateTime().toString() << result << progress;
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_gimbalCalTimeout()
{
    if(_gimbalProgress == 99) {
        qCDebug(YuneecCameraLog) << "Gimbal Calibration End Timeout";
        _gimbalProgress = 100;
        _gimbalCalOn = false;
        emit gimbalProgressChanged();
        emit gimbalCalOnChanged();
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_switchStateChanged(int swId, int oldState, int newState)
{
    Q_UNUSED(oldState);
    //-- On Button Down
    if(newState == 1) {
        switch(swId) {
            case Yuneec::BUTTON_CAMERA_SHUTTER:
                //-- Do we have storage (in kb) and is camera idle?
                if(storageTotal() == 0 || storageFree() < 250 || photoStatus() != PHOTO_CAPTURE_IDLE) {
                    //-- Undefined camera state
                    _errorSound.setLoopCount(1);
                    _errorSound.play();
                } else {
                    if(cameraMode() == CAM_MODE_VIDEO) {
                        //-- Can camera capture images in video mode?
                        if(photosInVideoMode()) {
                            takePhoto();
                        } else {
                            //-- Are we recording video?
                            if(videoStatus() != VIDEO_CAPTURE_STATUS_STOPPED) {
                                _errorSound.setLoopCount(1);
                                _errorSound.play();
                            } else {
                                //-- Must switch to photo mode first
                                setPhotoMode();
                                QTimer::singleShot(2500, this, &YuneecCameraControl::_delayedTakePhoto);
                            }
                        }
                    } else if(cameraMode() == CAM_MODE_PHOTO) {
                        takePhoto();
                    } else {
                        //-- Undefined camera state
                        _errorSound.setLoopCount(1);
                        _errorSound.play();
                    }
                }
                break;
            case Yuneec::BUTTON_VIDEO_SHUTTER:
                //-- Do we have storage (in kb) and is camera idle?
                if(storageTotal() == 0 || storageFree() < 250 || photoStatus() != PHOTO_CAPTURE_IDLE) {
                    //-- Undefined camera state
                    _errorSound.setLoopCount(1);
                    _errorSound.play();
                } else {
                    //-- If already in video mode, simply toggle on/off
                    if(cameraMode() == CAM_MODE_VIDEO) {
                        toggleVideo();
                    } else {
                        //-- Must switch to video mode first
                        setVideoMode();
                        QTimer::singleShot(2500, this, &YuneecCameraControl::_delayedStartVideo);
                    }
                }
                break;
            default:
                break;
        }
    }
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_delayedTakePhoto()
{
    takePhoto();
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_delayedStartVideo()
{
    startVideo();
}

//-----------------------------------------------------------------------------
// Getting the rec time from the camera is way too expensive because of the
// LCM interface within the camera firmware. Instead, we keep track of the
// timer here.
void
YuneecCameraControl::_recTimerHandler()
{
    _recordTime = _recTime.elapsed();
    emit recordTimeChanged();
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::factChanged(Fact* pFact)
{
    if(pFact->name() == kCAM_SPOTAREA) {
        emit spotAreaChanged();
    }
    QGCCameraControl::factChanged(pFact);
    //-- When one of these parameters changes
    if(pFact->name() == kCAM_EV ||
        pFact->name() == kCAM_EXPMODE ||
        pFact->name() == kCAM_ISO ||
        pFact->name() == kCAM_METERING ||
        pFact->name() == kCAM_MODE ||
        pFact->name() == kCAM_SHUTTERSPD ||
        pFact->name() == kCAM_WBMODE) {
        //-- Disable shutter button
        _setPhotoStatus(PHOTO_CAPTURE_STATUS_UNDEFINED);
        //-- Request capture status to reset shutter
        _captureInfoRetries = 0;
        _captureStatusTimer.start(1000);
    }
}

//-----------------------------------------------------------------------------
QSize
YuneecCameraControl::videoSize()
{
    return _videoSize;
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::setVideoSize(QSize s)
{
    _videoSize = s;
    emit videoSizeChanged();
}

//-----------------------------------------------------------------------------
QPoint
YuneecCameraControl::spotArea()
{
    Fact* pFact = getFact(kCAM_SPOTAREA);
    if(pFact) {
        float vw = (float)_videoSize.width();
        float vh = (float)_videoSize.height();
        int x = (int)((float)((pFact->rawValue().toUInt() >> 8) & 0xFF) * vw / 100.0f);
        int y = (int)((float)(pFact->rawValue().toUInt() & 0xFF) * vh / 100.0f);
        return QPoint(x, y);
    }
    return QPoint(0, 0);
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::setSpotArea(QPoint p)
{
    Fact* pFact = getFact(kCAM_SPOTAREA);
    if(pFact) {
        float vw = (float)_videoSize.width();
        float vh = (float)_videoSize.height();
        float fx = p.x() < 0 ? 0.0f : (float)p.x();
        float fy = p.y() < 0 ? 0.0f : (float)p.y();
        uint8_t x = (uint8_t)(fx / vw * 100.0f);
        uint8_t y = (uint8_t)(fy / vh * 100.0f);
        x = x > 100 ? 100 : x;
        y = y > 100 ? 100 : y;
        uint16_t coords = (x << 8) | y;
        qCDebug(YuneecCameraLog) << "Set Spot X:" << x << "Y:" << y;
        pFact->setRawValue(coords);
    }
}

//-----------------------------------------------------------------------------
bool
YuneecCameraControl::incomingParameter(Fact* pFact, QVariant& newValue)
{
    //-- Shutter Speed and ISO may come as actual measured values as opposed to
    //   one of the predefined values in the "set". We need to "snap" it to the
    //   nearest value in those cases.
    //-- Ignore shutter speed and ISO if in Auto Exposure mode
    if(pFact->name() == kCAM_SHUTTERSPD) {
        QVariant v = _validateShutterSpeed(pFact, newValue);
        if(newValue != v) {
            qCDebug(YuneecCameraLog) << "Shutter speed adjusted:" << newValue.toDouble() << "==>" << v.toDouble();
            newValue = v;
            if(!_updatesToSend.contains(pFact->name())) {
                _updatesToSend << pFact->name();
            }
            QTimer::singleShot(100, this, &YuneecCameraControl::_sendUpdates);
        }
    } else if(pFact->name() == kCAM_ISO) {
        QVariant v = _validateISO(pFact, newValue);
        if(newValue != v) {
            qCDebug(YuneecCameraLog) << "ISO adjusted:" << newValue.toUInt() << "==>" << v.toUInt();
            newValue = v;
            if(!_updatesToSend.contains(pFact->name())) {
                _updatesToSend << pFact->name();
            }
            QTimer::singleShot(100, this, &YuneecCameraControl::_sendUpdates);
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
YuneecCameraControl::validateParameter(Fact* pFact, QVariant& newValue)
{
    if(pFact->name() == kCAM_SHUTTERSPD) {
        return _validateShutterSpeed(pFact, newValue) == newValue;
    } else if(pFact->name() == kCAM_ISO) {
        return _validateISO(pFact, newValue) == newValue;
    }
    return true;
}

//-----------------------------------------------------------------------------
QVariant
YuneecCameraControl::_validateShutterSpeed(Fact* pFact, QVariant& newValue)
{
    QMap<double, double> values;
    foreach(QVariant v, pFact->enumValues()) {
        double diff = fabs(newValue.toDouble() - v.toDouble());
        values[diff] = v.toDouble();
    }
    return values.first();
}

//-----------------------------------------------------------------------------
QVariant
YuneecCameraControl::_validateISO(Fact* pFact, QVariant& newValue)
{
    QMap<uint32_t, uint32_t> values;
    foreach(QVariant v, pFact->enumValues()) {
        uint32_t diff = abs(newValue.toInt() - v.toInt());
        values[diff] = v.toUInt();
    }
    return values.first();
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::_sendUpdates()
{
    //-- Get current exposure mode
    Fact* pFact = getFact(kCAM_EXPMODE);
    //-- Only reactively update values in Manual Exposure mode
    if(pFact && pFact->rawValue() == 1) {
        foreach(QString param, _updatesToSend) {
            _paramIO[param]->sendParameter();
        }
    }
    _updatesToSend.clear();
}

//-----------------------------------------------------------------------------
void
YuneecCameraControl::handleCaptureStatus(const mavlink_camera_capture_status_t& cap)
{
    QGCCameraControl::handleCaptureStatus(cap);
    //-- Update recording time
    if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _recordTime = cap.recording_time_ms;
        _recTime = _recTime.addMSecs(_recTime.elapsed() - cap.recording_time_ms);
        emit recordTimeChanged();
    }
}
