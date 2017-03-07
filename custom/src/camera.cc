/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "typhoonh.h"
#include "m4.h"
#include "camera.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

#include <QDebug>

//-----------------------------------------------------------------------------
// Video Resolution Options
video_res_t videoResOptions[] = {
    {"4096 x 2160 25fps (4K DCI)",    4096, 2160, 25,  4096.0 / 2160.0},
    {"4096 x 2160 24fps (4K DCI)",    4096, 2160, 24,  4096.0 / 2160.0},
    {"3840 x 2160 30fps (4K UHD)",    3840, 2160, 30,  3840.0 / 2160.0},
    {"3840 x 2160 25fps (4K UHD)",    3840, 2160, 25,  3840.0 / 2160.0},
    {"3840 x 2160 24fps (4K UHD)",    3840, 2160, 24,  3840.0 / 2160.0},
    {"2560 x 1440 30fps (WQHD)",      2560, 1440, 30,  2560.0 / 1440.0},
    {"2560 x 1440 25fps (WQHD)",      2560, 1440, 25,  2560.0 / 1440.0},
    {"2560 x 1440 24fps (WQHD)",      2560, 1440, 24,  2560.0 / 1440.0},
    {"1920 x 1080 120fps (1080P)",    1920, 1080, 120, 1920.0 / 1080.0},
    {"1920 x 1080 60fps (1080P)",     1920, 1080, 60,  1920.0 / 1080.0},
    {"1920 x 1080 50fps (1080P)",     1920, 1080, 50,  1920.0 / 1080.0},
    {"1920 x 1080 48fps (1080P)",     1920, 1080, 48,  1920.0 / 1080.0},
    {"1920 x 1080 30fps (1080P)",     1920, 1080, 30,  1920.0 / 1080.0},
    {"1920 x 1080 25fps (1080P)",     1920, 1080, 25,  1920.0 / 1080.0},
    {"1920 x 1080 24fps (1080P)",     1920, 1080, 24,  1920.0 / 1080.0}
};

#define NUM_VIDEO_RES (sizeof(videoResOptions) / sizeof(video_res_t))

//-----------------------------------------------------------------------------
// Color Mode
color_mode_t colorModeOptions[] = {
    {"Natural",  0},
    {"Enhanced", 1},
    {"Raw",      2},
    {"Night",    3}
};

//-----------------------------------------------------------------------------
// ISO Values
iso_values_t isoValues[] = {
    {"100"}, {"150"}, {"200"}, {"300"}, {"400"}, {"600"}, {"800"}, {"1600"}, {"3200"}, {"Auto"}
};

#define NUM_ISO_VALUES (sizeof(isoValues) / sizeof(iso_values_t))

//-----------------------------------------------------------------------------
// White Balance Values
wb_values_t wbValues[] = {
    {"Auto"}, {"Incandescent"}, {"Sunset"}, {"Sunny"}, {"Cloudy"}, {"Fluorescent"}, {"Lock"}
};

#define NUM_WB_VALUES (sizeof(wbValues) / sizeof(wb_values_t))

//-----------------------------------------------------------------------------
// Shutter Speeds
shutter_speeds_t shutterSpeeds[] = {
    { "4s", 4.0},
    { "3s", 3.0},
    { "2s", 2.0},
    { "1s", 1.0},
    { "1/30",   1.0f /   30.0f},
    { "1/60",   1.0f /   60.0f},
    { "1/125",  1.0f /  125.0f},
    { "1/250",  1.0f /  250.0f},
    { "1/500",  1.0f /  500.0f},
    { "1/1000", 1.0f / 1000.0f},
    { "1/2000", 1.0f / 2000.0f},
    { "1/4000", 1.0f / 4000.0f},
    { "1/8000", 1.0f / 8000.0f},
    { "Auto", 1.0f}
};

#define NUM_SHUTTER_VALUES (sizeof(shutterSpeeds) / sizeof(shutter_speeds_t))

//-----------------------------------------------------------------------------
CameraControl::CameraControl(QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _waitingShutter(false)
    , _cameraSupported(CAMERA_SUPPORT_UNDEFINED)
    , _currentVideoRes(0)
    , _currentWb(0)
    , _currentIso(NUM_ISO_VALUES - 1)
    , _currentShutter(NUM_SHUTTER_VALUES - 1)
{
    memset(&_cameraStatus,   0, sizeof(camera_capture_status_t));
    memset(&_cameraSettings, 0, sizeof(camera_settings_t));
    _cameraStatus.video_status = VIDEO_CAPTURE_STATUS_UNDEFINED;
    _cameraSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/camera.wav"));
    _cameraSound.setLoopCount(1);
    _cameraSound.setVolume(0.9);
    _videoSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/beep.wav"));
    _videoSound.setVolume(0.9);
    _errorSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/boop.wav"));
    _errorSound.setVolume(0.9);
}

//-----------------------------------------------------------------------------
void
CameraControl::setVehicle(Vehicle* vehicle)
{
    if(_vehicle) {
        disconnect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &CameraControl::_mavlinkMessageReceived);
        _vehicle = NULL;
    }
    if(vehicle) {
        _vehicle = vehicle;
        memset(&_cameraStatus,   0, sizeof(camera_capture_status_t));
        memset(&_cameraSettings, 0, sizeof(camera_settings_t));
        _cameraStatus.video_status = VIDEO_CAPTURE_STATUS_UNDEFINED;
        _cameraSupported = CAMERA_SUPPORT_UNDEFINED;
        emit cameraModeChanged();
        emit videoStatusChanged();
        connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &CameraControl::_mavlinkMessageReceived);
        connect(_vehicle, &Vehicle::mavCommandResult,       this, &CameraControl::_mavCommandResult);
        //-- Send one single MAV_CMD_REQUEST_CAMERA_SETTINGS command to the camera and see what we get back
        QTimer::singleShot(2000, this, &CameraControl::_requestCameraSettings);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentVideoRes(quint32 index)
{
    if(index < NUM_VIDEO_RES) {
        qDebug() << "setCurrentVideoRes:" << videoResOptions[index].description;
        _currentVideoRes = index;
        emit currentVideoResChanged();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentWb(quint32 index)
{
    if(index < NUM_WB_VALUES) {
        qDebug() << "setCurrentWb:" << wbValues[index].description;
        _currentWb = index;
        emit currentWbChanged();
        float lock = 1.0f;
        if(index == (NUM_WB_VALUES - 1)) {
            lock = 0.0f;
        }
        _setSettings_2(DEFAULT_VALUE, lock, (float)index);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentIso(quint32 index)
{
    if(index < NUM_ISO_VALUES) {
        _currentIso = index;
        emit currentIsoChanged();
        float lock = 1.0f;
        if(index == (NUM_ISO_VALUES - 1)) {
            lock = 0.0f;
            //-- If ISO is locked, so must be shutter speed
            _cameraSettings.shutter_speed_locked = 0;
            if(_currentShutter != (NUM_SHUTTER_VALUES - 1)) {
                _currentShutter = (NUM_SHUTTER_VALUES - 1);
                emit currentShutterChanged();
            }
        }
        _setSettings_1(DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, (float)index, lock);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentShutter(quint32 index)
{
    if(index < NUM_SHUTTER_VALUES) {
        _currentShutter = index;
        emit currentShutterChanged();
        float lock = 1.0f;
        if(index == (NUM_SHUTTER_VALUES - 1)) {
            lock = 0.0f;
            //-- If shutter speed is locked, so must be ISO
            _cameraSettings.iso_sensitivity_locked = 0;
            if(_currentIso != (NUM_ISO_VALUES - 1)) {
                _currentIso = (NUM_ISO_VALUES - 1);
                emit currentIsoChanged();
            }
        }
        _setSettings_1(DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, shutterSpeeds[index].speed, lock);
    }
}

//-----------------------------------------------------------------------------
CameraControl::VideoStatus
CameraControl::videoStatus()
{
    if(_cameraStatus.data_ready) {
        return (VideoStatus)_cameraStatus.video_status;
    }
    return VIDEO_CAPTURE_STATUS_UNDEFINED;
}

//-----------------------------------------------------------------------------
CameraControl::CameraMode
CameraControl::cameraMode()
{
    if(_cameraSettings.data_ready) {
        qDebug() << "Get cameraMode:" << _cameraSettings.mode_id;
        return (CameraMode)_cameraSettings.mode_id;
    }
    return CAMERA_MODE_UNDEFINED;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::videoResList()
{

    if(_videoResList.size() == 0) {
        for(size_t i = 0; i < NUM_VIDEO_RES; i++) {
            _videoResList.append(videoResOptions[i].description);
        }
    }
    return _videoResList;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::colorModeList()
{

    if(_colorModeList.size() == 0) {
        for(size_t i = 0; i < sizeof(colorModeOptions) / sizeof(color_mode_t); i++) {
            _colorModeList.append(colorModeOptions[i].description);
        }
    }
    return _colorModeList;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::wbList()
{
    if(_wbList.size() == 0) {
        for(size_t i = 0; i < NUM_WB_VALUES; i++) {
            _wbList.append(wbValues[i].description);
        }
    }
    return _wbList;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::isoList()
{
    if(_isoList.size() == 0) {
        for(size_t i = 0; i < NUM_ISO_VALUES; i++) {
            _isoList.append(isoValues[i].description);
        }
    }
    return _isoList;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::shutterList()
{
    if(_shutterList.size() == 0) {
        for(size_t i = 0; i < NUM_SHUTTER_VALUES; i++) {
            _shutterList.append(shutterSpeeds[i].description);
        }
    }
    return _shutterList;
}

//-----------------------------------------------------------------------------
void
CameraControl::setCameraMode(CameraMode mode)
{
    if(mode == CAMERA_MODE_VIDEO) {
        setVideoMode();
    } else if(mode == CAMERA_MODE_PHOTO) {
        setPhotoMode();
    }
}

//-----------------------------------------------------------------------------
quint32
CameraControl::recordTime()
{
    if(_cameraStatus.data_ready) {
        return _cameraStatus.recording_time_ms;
    }
    return 0;
}

//-----------------------------------------------------------------------------
QString
CameraControl::recordTimeStr()
{
    QString timeStr("00:00:00");
    if(_cameraStatus.data_ready) {
        timeStr = QTime(0, 0).addMSecs(_cameraStatus.recording_time_ms).toString("hh:mm:ss");
    }
    return timeStr;
}

//-----------------------------------------------------------------------------
void
CameraControl::startVideo()
{
    qDebug() << "startVideo()";
    if(_vehicle && _cameraStatus.data_ready && _cameraSettings.mode_id == CAMERA_MODE_VIDEO) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_VIDEO_START_CAPTURE,                // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            videoResOptions[_currentVideoRes].fps,      // Frames per second (max)
            0,                                          // Resolution in M Pixels
            videoResOptions[_currentVideoRes].with,     // Horizontal Resolution
            videoResOptions[_currentVideoRes].height,   // Vertical Resolution
            1.0);                                       // Frequency CAMERA_CAPTURE_STATUS messages should be sent while recording (0 for no messages, otherwise time in Hz)
        _recordTime.start();
        _videoSound.setLoopCount(1);
        _videoSound.play();
        QTimer::singleShot(250, this, &CameraControl::_requestCaptureStatus);
    } else {
        _errorSound.setLoopCount(1);
        _errorSound.play();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::stopVideo()
{
    qDebug() << "stopVideo()";
    if(_vehicle && _cameraStatus.data_ready) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_VIDEO_STOP_CAPTURE ,                // Command id
            true,                                       // ShowError
            0);                                         // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
        _videoSound.setLoopCount(2);
        _videoSound.play();
        QTimer::singleShot(250, this, &CameraControl::_requestCaptureStatus);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::takePhoto()
{
    qDebug() << "takePhoto()";
    //-- Send MAVLink command telling vehicle to take photo
    if(_vehicle && _cameraStatus.data_ready) {
        if(_waitingShutter) {
            _errorSound.setLoopCount(1);
            _errorSound.play();
        } else {
            _waitingShutter = true;
            _vehicle->sendMavCommand(
                MAV_COMP_ID_CAMERA,                     // Target component
                MAV_CMD_IMAGE_START_CAPTURE,            // Command id
                true,                                   // ShowError
                0,                                      // Duration between two consecutive pictures (in seconds)
                1,                                      // Number of images to capture total - 0 for unlimited capture
                -1);                                    // Resolution in megapixels (max)
        }
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::toggleMode()
{
    if(cameraMode() == CAMERA_MODE_PHOTO) {
        setVideoMode();
    } else if(cameraMode() == CAMERA_MODE_VIDEO) {
        setPhotoMode();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setVideoMode()
{
    qDebug() << "setVideoMode()";
    _setSettings_2(DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, CAMERA_MODE_VIDEO);
    _cameraSettings.mode_id = CAMERA_MODE_VIDEO;
    emit cameraModeChanged();
}

//-----------------------------------------------------------------------------
void
CameraControl::setPhotoMode()
{
    qDebug() << "setPhotoMode()";
    _setSettings_2(DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, CAMERA_MODE_PHOTO);
    _cameraSettings.mode_id = CAMERA_MODE_PHOTO;
    emit cameraModeChanged();
}

//-----------------------------------------------------------------------------
void
CameraControl::_setSettings_1(float p1, float p2, float p3, float p4, float p5, float p6, float p7)
{
    if (_vehicle && _cameraStatus.data_ready) {
        qDebug() << "_setSettings_1()" << p1 << p2 << p3 << p4 << p5 << p6 << p7;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                                     // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_1,                          // Command id
            true,                                                   // ShowError
            p1 >= 0 ? p1 : 0,                                       // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            p2 >= 0 ? p2 : _cameraSettings.aperture,                // Aperture
            p3 >= 0 ? p3 : _cameraSettings.aperture_locked,         // Aperture Locked
            p4 >= 0 ? p4 : _cameraSettings.shutter_speed,           // Shutter Speed
            p5 >= 0 ? p5 : _cameraSettings.shutter_speed_locked,    // Shutter Speed Locked
            p6 >= 0 ? p6 : _cameraSettings.iso_sensitivity,         // ISO
            p7 >= 0 ? p7 : _cameraSettings.iso_sensitivity_locked); // ISO Locked
        //-- There is no reply other than "accepted" or not. There is no
        //   guarantee that these settings took. Let's request an updated
        //   set of camera settings.
        QTimer::singleShot(500, this, &CameraControl::_requestCameraSettings);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_setSettings_2(float p1, float p2, float p3, float p4, float p5, float p6)
{
    if (_vehicle && _cameraStatus.data_ready) {
        qDebug() << "_setSettings_2()" << p1 << p2 << p3 << p4 << p5 << p6;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                                     // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_2,                          // Command id
            true,                                                   // ShowError
            p1 >= 0 ? p1 : 0,                                       // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            p2 >= 0 ? p2 : _cameraSettings.white_balance_locked,    // White Balance Locked
            p3 >= 0 ? p3 : _cameraSettings.white_balance,           // White Balance
            p4 >= 0 ? p4 : _cameraSettings.mode_id,                 // Set Video/Photo Mode
            p5 >= 0 ? p5 : _cameraSettings.color_mode_id,           // Color Mode (IQ)
            p6 >= 0 ? p6 : _cameraSettings.image_format_id,         // Image Format
            0);                                                     // Reserved
        //-- There is no reply other than "accepted" or not. There is no
        //   guarantee that these settings took. Let's request an updated
        //   set of camera settings. If changing camera modes, it can be
        //   quite a while before we can send another command, so we wait
        //   an extra amount of time if that's the case.
        QTimer::singleShot(p5 < 0 ? 500 : 2000, this, &CameraControl::_requestCameraSettings);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::toggleVideo()
{
    if(videoStatus() == VIDEO_CAPTURE_STATUS_STOPPED) {
        startVideo();
    } else if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        stopVideo();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_mavCommandResult(int /*vehicleId*/, int /*component*/, int command, int result, bool noReponseFromVehicle)
{
    //-- Do we already know if the firmware supports cameras or not?
    if(_cameraSupported == CAMERA_SUPPORT_UNDEFINED) {
        //-- Is this the response we are waiting?
        if(command == MAV_CMD_REQUEST_CAMERA_SETTINGS) {
            if(noReponseFromVehicle) {
                qDebug() << "No response for MAV_CMD_REQUEST_CAMERA_SETTINGS";
                //-- We got no answer so we assume no camera support
                _cameraSupported = CAMERA_SUPPORT_NO;
            } else {
                if(result == MAV_RESULT_ACCEPTED) {
                    //-- We have an answer. Start the show.
                    _cameraSupported = CAMERA_SUPPORT_YES;
                    _requestCaptureStatus();
                } else {
                    //-- We got an answer but not a good one
                    _cameraSupported = CAMERA_SUPPORT_NO;
                }
            }
        }
    } else if(_cameraSupported == CAMERA_SUPPORT_YES) {
        switch(command) {
            case MAV_CMD_IMAGE_START_CAPTURE:
                if(result == MAV_RESULT_ACCEPTED) {
                    _cameraSound.play();
                    _waitingShutter = false;
                } else {
                    _errorSound.setLoopCount(2);
                    _errorSound.play();
                }
                break;
            case MAV_CMD_REQUEST_CAMERA_SETTINGS:
                if(noReponseFromVehicle) {
                    qDebug() << "Retry MAV_CMD_REQUEST_CAMERA_SETTINGS";
                    _requestCameraSettings();
                } else {
                    if(result != MAV_RESULT_ACCEPTED) {
                        qDebug() << "Bad response from MAV_CMD_REQUEST_CAMERA_SETTINGS" << result << "Retrying...";
                        _requestCameraSettings();
                    }
                }
                break;
            case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
                if(noReponseFromVehicle) {
                    qDebug() << "Retry MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS";
                    _requestCaptureStatus();
                } else {
                    if(result != MAV_RESULT_ACCEPTED) {
                        qDebug() << "Bad response from MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS" << result << "Retrying...";
                        _requestCaptureStatus();
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
CameraControl::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS:
            _handleCaptureStatus(message);
            break;
        case MAVLINK_MSG_ID_CAMERA_SETTINGS:
            _handleCameraSettings(message);
            break;
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleCaptureStatus(const mavlink_message_t &message)
{
    //-- This is a response to MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS
    mavlink_camera_capture_status_t cap;
    mavlink_msg_camera_capture_status_decode(&message, &cap);
    bool wasReady = (bool)_cameraStatus.data_ready;
    _cameraStatus.data_ready = true;
    _cameraStatus.available_capacity    = cap.available_capacity;
    _cameraStatus.image_interval        = cap.image_interval;
    _cameraStatus.image_resolution_h    = cap.image_resolution_h;
    _cameraStatus.image_resolution_v    = cap.image_resolution_h;
    _cameraStatus.image_status          = cap.image_status;
    _updateVideoRes(cap.video_resolution_h, cap.video_resolution_v, cap.video_framerate);
    qDebug() << "_handleCaptureStatus:" << cap.video_status << cap.recording_time_ms << cap.available_capacity;
    if(!wasReady || _cameraStatus.video_status != cap.video_status) {
        _cameraStatus.video_status = cap.video_status;
        emit videoStatusChanged();
    }
    _cameraStatus.recording_time_ms = cap.recording_time_ms;
    if(_cameraStatus.video_status == VIDEO_CAPTURE_STATUS_RUNNING) {
        emit recordTimeChanged();
        //-- TODO: While the firmware doesn't send us these automatically, we ask for it
        QTimer::singleShot(1000, this, &CameraControl::_requestCaptureStatus);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleCameraSettings(const mavlink_message_t &message)
{
    mavlink_camera_settings_t settings;
    mavlink_msg_camera_settings_decode(&message, &settings);
    qDebug() << "_handleCameraSettings. Mode:" << settings.mode_id;
    _cameraSettings.data_ready = true;
    _cameraSettings.aperture                = settings.aperture;
    _cameraSettings.aperture_locked         = settings.aperture_locked;
    _cameraSettings.color_mode_id           = settings.color_mode_id;
    _cameraSettings.image_format_id         = settings.image_format_id;
    _updateIso(settings.iso_sensitivity, settings.iso_sensitivity_locked);
    _cameraSettings.shutter_speed           = settings.shutter_speed;
    _cameraSettings.shutter_speed_locked    = settings.shutter_speed_locked;
    _cameraSettings.white_balance           = settings.white_balance;
    _cameraSettings.white_balance_locked    = settings.white_balance_locked;
    _cameraSettings.mode_id = settings.mode_id;
    emit cameraModeChanged();
    _updateAspectRatio();
}

//-----------------------------------------------------------------------------
void
CameraControl::_updateIso(int iso, int locked)
{
    if(_cameraSettings.iso_sensitivity != iso || _cameraSettings.iso_sensitivity_locked != locked) {
        _cameraSettings.iso_sensitivity = iso;
        _cameraSettings.iso_sensitivity_locked  = locked;
        if(locked == 0) {
            _currentIso = NUM_ISO_VALUES - 1;
        } else {
            _currentIso = iso;
        }
        emit currentIsoChanged();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_updateShutter(int speed, int locked)
{
    if(_cameraSettings.shutter_speed != speed || _cameraSettings.shutter_speed_locked != locked) {
        _cameraSettings.shutter_speed = speed;
        _cameraSettings.shutter_speed_locked  = locked;
        if(locked == 0) {
            _currentShutter = NUM_SHUTTER_VALUES - 1;
        } else {
            _currentShutter = speed;
        }
        emit currentShutterChanged();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_updateVideoRes(int w, int h, float f)
{
    if(_cameraStatus.video_framerate     != f ||
        _cameraStatus.video_resolution_h != w ||
        _cameraStatus.video_resolution_v != h)
    {
        _cameraStatus.video_framerate       = f;
        _cameraStatus.video_resolution_h    = w;
        _cameraStatus.video_resolution_v    = h;
    }
    _currentVideoRes = _findVideoRes(w, h, f);
    emit currentVideoResChanged();
}

//-----------------------------------------------------------------------------
int
CameraControl::_findVideoRes(int w, int h, float f)
{
    for(size_t i = 0; i < NUM_VIDEO_RES; i++) {
        if(videoResOptions[i].fps     == f &&
            videoResOptions[i].with   == w &&
            videoResOptions[i].height == h)
        {
            return i;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
void
CameraControl::_updateAspectRatio()
{
    qDebug() << "_updateAspectRatio() Mode:" << _cameraSettings.mode_id;
    if(_cameraSettings.data_ready) {
        //-- Photo Mode
        if(_cameraSettings.mode_id == CAMERA_MODE_PHOTO) {
            qDebug() << "Set 4:3 Aspect Ratio";
            qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(1.333333);
        //-- Video Mode
        } else if(_cameraSettings.mode_id == CAMERA_MODE_VIDEO && _currentVideoRes < (int)NUM_VIDEO_RES) {
            qDebug() << "Set Video Aspect Ratio" << videoResOptions[_currentVideoRes].aspectRatio;
            qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(videoResOptions[_currentVideoRes].aspectRatio);
        }
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_requestCameraSettings()
{
    if(_vehicle) {
        qDebug() << "_requestCameraSettings";
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                     // target component
            MAV_CMD_REQUEST_CAMERA_SETTINGS,        // command id
            false,                                  // showError
            1,
            0);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_requestCaptureStatus()
{
    if(_vehicle) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                     // target component
            MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS,  // command id
            true,                                   // showError
            1,
            0);
    }
}

