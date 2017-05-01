/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */


#include "TyphoonHPlugin.h"
#include "TyphoonHM4Interface.h"
#include "CameraControl.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "QGCMapEngine.h"

QGC_LOGGING_CATEGORY(YuneecCameraLog, "YuneecCameraLog")
QGC_LOGGING_CATEGORY(YuneecCameraLogVerbose, "YuneecCameraLogVerbose")

//-----------------------------------------------------------------------------
// Video Resolution Options
video_res_t videoResOptions[] = {
    {"4096 x 2160 25fps (4K DCI)",    4096, 2160, 25,  4096.0f / 2160.0f},
    {"4096 x 2160 24fps (4K DCI)",    4096, 2160, 24,  4096.0f / 2160.0f},
    {"3840 x 2160 30fps (4K UHD)",    3840, 2160, 30,  3840.0f / 2160.0f},
    {"3840 x 2160 25fps (4K UHD)",    3840, 2160, 25,  3840.0f / 2160.0f},
    {"3840 x 2160 24fps (4K UHD)",    3840, 2160, 24,  3840.0f / 2160.0f},
    {"2560 x 1440 30fps (WQHD)",      2560, 1440, 30,  2560.0f / 1440.0f},
    {"2560 x 1440 25fps (WQHD)",      2560, 1440, 25,  2560.0f / 1440.0f},
    {"2560 x 1440 24fps (WQHD)",      2560, 1440, 24,  2560.0f / 1440.0f},
    {"1920 x 1080 120fps (1080P)",    1920, 1080, 120, 1920.0f / 1080.0f},
    {"1920 x 1080 60fps (1080P)",     1920, 1080, 60,  1920.0f / 1080.0f},
    {"1920 x 1080 50fps (1080P)",     1920, 1080, 50,  1920.0f / 1080.0f},
    {"1920 x 1080 48fps (1080P)",     1920, 1080, 48,  1920.0f / 1080.0f},
    {"1920 x 1080 30fps (1080P)",     1920, 1080, 30,  1920.0f / 1080.0f},
    {"1920 x 1080 25fps (1080P)",     1920, 1080, 25,  1920.0f / 1080.0f},
    {"1920 x 1080 24fps (1080P)",     1920, 1080, 24,  1920.0f / 1080.0f}
};

#define NUM_VIDEO_RES (sizeof(videoResOptions) / sizeof(video_res_t))

//-----------------------------------------------------------------------------
// Color Mode (CMD=SET_IQ_TYPE&mode=x)
iq_mode_t iqModeOptions[] = {
    {"Natural",  0},
    {"Enhanced", 1},
    {"Raw",      2},
    {"Night",    3}
};

#define NUM_IQ_MODES (sizeof(iqModeOptions) / sizeof(iq_mode_t))

//-----------------------------------------------------------------------------
// Metering Mode (CMD=SET_METERING_MODE&mode=x)
metering_mode_t meteringModeOptions[] = {
    {"Spot",    0},
    {"Center",  1},
    {"Average", 2}
};

#define NUM_METERING_VALUES (sizeof(meteringModeOptions) / sizeof(metering_mode_t))

//-----------------------------------------------------------------------------
// Photo Format Mode (CMD=SET_PHOTO_FORMAT&format=dng)
photo_format_t photoFormatOptions[] = {
    {"DNG",         0},
    {"Jpeg",        1},
    {"DNG + Jpeg",  2}
};

#define NUM_PHOTO_FORMAT_VALUES (sizeof(photoFormatOptions) / sizeof(photo_format_t))

//-----------------------------------------------------------------------------
// White Balance (CMD=SET_WHITEBLANCE_MODE&mode=x)
white_balance_t whiteBalanceOptions[] = {
    {"Auto",            0},
    {"Sunny",           4},
    {"Cloudy",          5},
    {"Fluorescent",     7},
    {"Incandescent",    1},
    {"Sunset",          3}
};

#define NUM_WB_VALUES (sizeof(whiteBalanceOptions) / sizeof(white_balance_t))

//-----------------------------------------------------------------------------
// ISO Values
iso_values_t isoValues[] = {
    {"100"},
    {"150"},
    {"200"},
    {"300"},
    {"400"},
    {"600"},
    {"800"},
    {"1600"},
    {"3200"}
};

#define NUM_ISO_VALUES (sizeof(isoValues) / sizeof(iso_values_t))

//-----------------------------------------------------------------------------
// Shutter Speeds
shutter_speeds_t shutterSpeeds[] = {
    { "4s", "4L"},
    { "3s", "3L"},
    { "2s", "2L"},
    { "1s", "1L"},
    { "1/30",   "30"},
    { "1/60",   "60"},
    { "1/125",  "125"},
    { "1/250",  "250"},
    { "1/500",  "500"},
    { "1/1000", "1000"},
    { "1/2000", "2000"},
    { "1/4000", "4000"},
    { "1/8000", "8000"}
};

#define NUM_SHUTTER_VALUES (sizeof(shutterSpeeds) / sizeof(shutter_speeds_t))

//-----------------------------------------------------------------------------
// Exposure Compensation
exposure_compsensation_t evOptions[] = {
    { "-2.0", -2.0f},
    { "-1.5", -1.5f},
    { "-1.0", -1.0f},
    { "-0.5", -0.5f},
    {   "0",  0.0f},
    { "+0.5", 0.5f},
    { "+1.0", 1.0f},
    { "+1.5", 1.5f},
    { "+2.0", 2.0f},
};

#define NUM_EV_VALUES (sizeof(evOptions) / sizeof(exposure_compsensation_t))

//-----------------------------------------------------------------------------
CameraControl::CameraControl(QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _cameraSupported(CAMERA_SUPPORT_UNDEFINED)
    , _httpErrorCount(0)
    , _true_cam_mode(CAMERA_MODE_UNDEFINED)
    , _currentVideoResIndex(0)
    , _currentWB(0)
    , _currentIso(0)
    , _currentShutter(0)
    , _currentPhotoFmt(1)
{
    _resetCameraValues();
    _cameraSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/wav/camera.wav"));
    _cameraSound.setLoopCount(1);
    _cameraSound.setVolume(0.9);
    _videoSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/wav/beep.wav"));
    _videoSound.setVolume(0.9);
    _errorSound.setSource(QUrl::fromUserInput("qrc:/typhoonh/wav/boop.wav"));
    _errorSound.setVolume(0.9);
}

//-----------------------------------------------------------------------------
CameraControl::~CameraControl()
{
}

//-----------------------------------------------------------------------------
void
CameraControl::setVehicle(Vehicle* vehicle)
{
    _resetCameraValues();
    _cameraSupported = CAMERA_SUPPORT_UNDEFINED;
    emit cameraModeChanged();
    emit videoStatusChanged();
    if(_vehicle) {
        _vehicle = NULL;
        disconnect(&_statusTimer, &QTimer::timeout, this, &CameraControl::_getCameraStatus);
        _cameraSupported = CAMERA_SUPPORT_UNDEFINED;
    }
    if(vehicle) {
        _vehicle = vehicle;
        _httpErrorCount = 0;
        connect(&_statusTimer, &QTimer::timeout, this, &CameraControl::_getCameraStatus);
        _statusTimer.setSingleShot(true);
        //-- Ambarella Interface
        _initStreaming();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_initStreaming()
{
    //-- Set RTSP resolution to 480P
    //-- TODO: No API
}

//-----------------------------------------------------------------------------
void
CameraControl::_getCameraStatus()
{
    if(_vehicle) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_REQUEST_CAMERA_INFORMATION,         // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            1);                                         // Do Request
    }
    //-- Make sure we're always asking for it
    if(_cameraSupported != CAMERA_SUPPORT_NO) {
        _statusTimer.start(10000);
    }
}

//-----------------------------------------------------------------------------
QString
CameraControl::sdFreeStr()
{
    return QGCMapEngine::bigSizeToString((quint64)_ambarellaStatus.sdfree * 1024);
}

//-----------------------------------------------------------------------------
void
CameraControl::takePhoto()
{
    qCDebug(YuneecCameraLog) << "takePhoto()";
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_IMAGE_START_CAPTURE,                // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            0,                                          // Duration between two consecutive pictures (in seconds--ignored if single image)
            1,                                          // Number of images to capture total - 0 for unlimited capture
            -1,                                         // Horizontal resolution in pixels (set to -1 for highest resolution possible)
            -1);                                        // Vertical resolution in pixels (set to -1 for highest resolution possible)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::startVideo()
{
    qCDebug(YuneecCameraLog) << "startVideo()";
    if(_vehicle && videoStatus() == VIDEO_CAPTURE_STATUS_STOPPED && _ambarellaStatus.cam_mode == CAMERA_MODE_VIDEO) {
        int w = -1;
        int h = -1;
        int f = -1;
        if(_setVideoResIndex < NUM_VIDEO_RES) {
            w = videoResOptions[_currentVideoResIndex].width;
            h = videoResOptions[_currentVideoResIndex].height;
            f = videoResOptions[_currentVideoResIndex].fps;
        }
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_VIDEO_START_CAPTURE,                // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            f,                                          // FPS: (-1 for max)
            w,                                          // Horizontal resolution in pixels (set to -1 for highest resolution possible)
            h);                                         // Vertical resolution in pixels (set to -1 for highest resolution possible)
    } else {
        _errorSound.setLoopCount(1);
        _errorSound.play();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::stopVideo()
{
    qCDebug(YuneecCameraLog) << "stopVideo()";
    if(_vehicle && videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_VIDEO_STOP_CAPTURE,                 // Command id
            true,                                       // ShowError
            0);                                         // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setVideoMode()
{
    qCDebug(YuneecCameraLog) << "setVideoMode()";
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES && _ambarellaStatus.cam_mode != CAMERA_MODE_VIDEO) {
        //-- Force UI to update. We keep the real camera mode elsewhere so we
        //   track when the camera actually changed modes, which is quite some
        //   time later.
        _true_cam_mode = _ambarellaStatus.cam_mode;
        _ambarellaStatus.cam_mode = CAMERA_MODE_VIDEO;
        emit cameraModeChanged();
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_2,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            1,                                          // Camera mode (0: photo, 1: video)
            NAN,                                        // Audio recording enabled (0: off 1: on)
            NAN,                                        // Metering mode ID (Average, Center, Spot, etc.)
            NAN,                                        // Image format ID (Jpeg/Raw/Jpeg+Raw)
            NAN,                                        // Image quality ID (Compression)
            NAN);                                       // Color mode ID (Neutral, Vivid, etc.)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setPhotoMode()
{
    qCDebug(YuneecCameraLog) << "setPhotoMode()";
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES && _ambarellaStatus.cam_mode != CAMERA_MODE_VIDEO) {
        //-- Force UI to update. We keep the real camera mode elsewhere so we
        //   track when the camera actually changed modes, which is quite some
        //   time later.
        _true_cam_mode = _ambarellaStatus.cam_mode;
        _ambarellaStatus.cam_mode = CAMERA_MODE_PHOTO;
        emit cameraModeChanged();
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_2,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            0,                                          // Camera mode (0: photo, 1: video)
            NAN,                                        // Audio recording enabled (0: off 1: on)
            NAN,                                        // Metering mode ID (Average, Center, Spot, etc.)
            NAN,                                        // Image format ID (Jpeg/Raw/Jpeg+Raw)
            NAN,                                        // Image quality ID (Compression)
            NAN);                                       // Color mode ID (Neutral, Vivid, etc.)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentVideoRes(quint32 index)
{
    if(index < NUM_VIDEO_RES) {
        qCDebug(YuneecCameraLog) << "setCurrentVideoRes:" << videoResOptions[index].description;
        _setVideoResIndex = index;
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentWB(quint32 index)
{
    if(index < NUM_WB_VALUES && _vehicle && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentWb:" << whiteBalanceOptions[index].description;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_1,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            NAN,                                        // Aperture (1/value) (Fixed for CGO3+)
            NAN,                                        // Shutter speed in seconds
            NAN,                                        // ISO sensitivity
            NAN,                                        // AE mode (Auto Exposure) (0: full auto 1: full manual 2: aperture priority 3: shutter priority)
            NAN,                                        // EV value (when in auto exposure)
            whiteBalanceOptions[index].mode);           // White balance (color temperature in K) (0: Auto WB)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentIso(quint32 index)
{
    if(_vehicle && index < NUM_ISO_VALUES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentIso:" << isoValues[index].description;



    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentShutter(quint32 index)
{
    if(_vehicle && index < NUM_SHUTTER_VALUES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentShutter:" << shutterSpeeds[index].description;



    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentIQ(quint32 index)
{
    if(_vehicle && index < NUM_IQ_MODES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentIQ:" << iqModeOptions[index].description;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_2,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            NAN,                                        // Camera mode (0: photo, 1: video)
            NAN,                                        // Audio recording enabled (0: off 1: on)
            NAN,                                        // Metering mode ID (Average, Center, Spot, etc.)
            NAN,                                        // Image format ID (Jpeg/Raw/Jpeg+Raw)
            NAN,                                        // Image quality ID (Compression)
            iqModeOptions[index].mode);                 // Color mode ID (Neutral, Vivid, etc.)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentPhotoFmt(quint32 index)
{
    if(_vehicle && index < NUM_PHOTO_FORMAT_VALUES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentPhotoFmt:" << photoFormatOptions[index].description;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_2,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            NAN,                                        // Camera mode (0: photo, 1: video)
            NAN,                                        // Audio recording enabled (0: off 1: on)
            NAN,                                        // Metering mode ID (Average, Center, Spot, etc.)
            photoFormatOptions[index].mode,             // Image format ID (Jpeg/Raw/Jpeg+Raw)
            NAN,                                        // Image quality ID (Compression)
            NAN);                                       // Color mode ID (Neutral, Vivid, etc.)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentMetering(quint32 index)
{
    if(_vehicle && index < NUM_METERING_VALUES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentMetering:" << meteringModeOptions[index].description;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_2,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            NAN,                                        // Camera mode (0: photo, 1: video)
            NAN,                                        // Audio recording enabled (0: off 1: on)
            meteringModeOptions[index].mode,            // Metering mode ID (Average, Center, Spot, etc.)
            NAN,                                        // Image format ID (Jpeg/Raw/Jpeg+Raw)
            NAN,                                        // Image quality ID (Compression)
            NAN);                                       // Color mode ID (Neutral, Vivid, etc.)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentEV(quint32 index)
{
    if(_vehicle && index < NUM_EV_VALUES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentEV:" << evOptions[index].description;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_1,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            NAN,                                        // Aperture (1/value) (Fixed for CGO3+)
            NAN,                                        // Shutter speed in seconds
            NAN,                                        // ISO sensitivity
            NAN,                                        // AE mode (Auto Exposure) (0: full auto 1: full manual 2: aperture priority 3: shutter priority)
            evOptions[index].value,                     // EV value (when in auto exposure)
            NAN);                                       // White balance (color temperature in K) (0: Auto WB)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setAeMode(AEModes mode)
{
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setAeMode:" << mode;
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_SET_CAMERA_SETTINGS_1,              // Command id
            true,                                       // ShowError
            1,                                          // Camera ID (1 for first, 2 for second, etc.)
            NAN,                                        // Aperture (1/value) (Fixed for CGO3+)
            NAN,                                        // Shutter speed in seconds
            NAN,                                        // ISO sensitivity
            mode == AE_MODE_MANUAL ? 1 : 0,             // AE mode (Auto Exposure) (0: full auto 1: full manual 2: aperture priority 3: shutter priority)
            NAN,                                        // EV value (when in auto exposure)
            NAN);                                       // White balance (color temperature in K) (0: Auto WB)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::formatCard()
{
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_STORAGE_FORMAT,                     // Command id
            true,                                       // ShowError
            1,                                          // Storage ID (1 for first, 2 for second, etc.)
            1);                                         // Do Format
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::resetSettings()
{
    //-- TODO: Need MAVLink update
    /*
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_RESET_CAMERA_SETTINGS,              // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras, 1 for first, 2 for second, etc.)
            1);                                         // Do Reset
    }
    */
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleShutterStatus()
{
    /*
    for(uint32_t i = 0; i < NUM_SHUTTER_VALUES; i++) {
        if(_ambarellaStatus.shutter_time == shutterSpeeds[i].value) {
            if(_currentShutter != i) {
                _currentShutter = i;
                emit currentShutterChanged();
                return;
            }
        }
    }
    */
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleISOStatus()
{
    /*
    for(uint32_t i = 0; i < NUM_ISO_VALUES; i++) {
        QString iso = QString("ISO_%1").arg(isoValues[i].description);
        if(_ambarellaStatus.iso_value == iso) {
            if(_currentIso != i) {
                _currentIso = i;
                emit currentIsoChanged();
                return;
            }
        }
    }
    */
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleVideoResStatus()
{
    /*
    for(uint32_t i = 0; i < NUM_VIDEO_RES; i++) {
        if(_ambarellaStatus.video_mode == videoResOptions[i].video_mode) {
            if(_currentVideoResIndex != i) {
                _currentVideoResIndex = i;
                emit currentVideoResChanged();
                _updateAspectRatio();
                return;
            }
        }
    }
    */
}

//-----------------------------------------------------------------------------
void
CameraControl::_requestCameraSettings()
{
    if(_vehicle) {
        qCDebug(YuneecCameraLog) << "_requestCameraSettings";
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                     // target component
            MAV_CMD_REQUEST_CAMERA_SETTINGS,        // command id
            false,                                  // showError
            0,                                      // Camera ID (0 for all cameras, 1 for first, 2 for second, etc.)
            1);                                     // Do Request
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
            0,                                      // Camera ID (0 for all cameras, 1 for first, 2 for second, etc.)
            1);                                     // Do Request
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
                qCDebug(YuneecCameraLog) << "No response for MAV_CMD_REQUEST_CAMERA_SETTINGS";
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
                    //_waitingShutter = false;
                } else {
                    _errorSound.setLoopCount(2);
                    _errorSound.play();
                }
                break;
            case MAV_CMD_REQUEST_CAMERA_SETTINGS:
                if(noReponseFromVehicle) {
                    qCDebug(YuneecCameraLog) << "Retry MAV_CMD_REQUEST_CAMERA_SETTINGS";
                    _requestCameraSettings();
                } else {
                    if(result != MAV_RESULT_ACCEPTED) {
                        qCDebug(YuneecCameraLog) << "Bad response from MAV_CMD_REQUEST_CAMERA_SETTINGS" << result << "Retrying...";
                        _requestCameraSettings();
                    }
                }
                break;
            case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
                if(noReponseFromVehicle) {
                    qCDebug(YuneecCameraLog) << "Retry MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS";
                    _requestCaptureStatus();
                } else {
                    if(result != MAV_RESULT_ACCEPTED) {
                        qCDebug(YuneecCameraLog) << "Bad response from MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS" << result << "Retrying...";
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
CameraControl::_handleCameraSettings(const mavlink_message_t& message)
{
    mavlink_camera_settings_t settings;
    mavlink_msg_camera_settings_decode(&message, &settings);
    qCDebug(YuneecCameraLog) << "_handleCameraSettings. Mode:" << settings.mode_id;
#if 0
    //-- Camera Mode
    int cam_mode = settings.mode_id == 0 ? CAMERA_MODE_PHOTO : CAMERA_MODE_VIDEO;
    //-- Camera mode switch takes too long so we switch the UI right
    //   after the user presses the switch. Internally however, we only
    //   truly find out the mode once we get an answer from the camera.
    if(_true_cam_mode != cam_mode) {
        _true_cam_mode = cam_mode;
        _ambarellaStatus.cam_mode = cam_mode;
        emit cameraModeChanged();
        _updateAspectRatio();
    }
    //-- White Balance
    if(_ambarellaStatus.white_balance != settings.white_balance) {
        _ambarellaStatus.white_balance = settings.white_balance;
        for(uint32_t i = 0; i < NUM_WB_VALUES; i++) {
            if(whiteBalanceOptions[i].mode == wb) {
                _currentWB = i;
                break;
            }
        }
        emit currentWBChanged();
    }
    //-- Auto Exposure Mode
    if(_ambarellaStatus.ae_enable != ae) {
        _ambarellaStatus.ae_enable = ae;
        emit aeModeChanged();
        if(!ae) {
            _handleShutterStatus();
            _handleISOStatus();
        }
    }





            //-- Shutter and ISO (Manual Mode)
            _ambarellaStatus.shutter_time    = set.value(QString("shutter_time")).toString();
            _handleShutterStatus();
            _ambarellaStatus.iso_value       = set.value(QString("iso_value")).toString();
            _handleISOStatus();
            //-- Color IQ
            uint32_t iq = set.value(QString("iq_type")).toString().toUInt();
            if(_ambarellaStatus.iq_type != iq && iq < NUM_IQ_MODES) {
                _ambarellaStatus.iq_type = iq;
                emit currentIQChanged();
            }
            //-- EV
            QString ev = set.value(QString("exposure_value")).toString();
            if(_ambarellaStatus.exposure_value != ev) {
                uint32_t idx = 100000;
                for(uint32_t i = 0; i < NUM_EV_VALUES; i++) {
                    if(ev == evOptions[i].value) {
                        idx = i;
                        break;
                    }
                }
                if(idx < NUM_EV_VALUES) {
                    _currentEV = idx;
                    emit currentEVChanged();
                }
            }
            //-- Current Video Resolution and FPS
            _ambarellaStatus.video_mode      = set.value(QString("video_mode")).toString();
            _handleVideoResStatus();
            //-- Photo Format
            QString pf = set.value(QString("photo_format")).toString();
            if(_ambarellaStatus.photo_format != pf) {
                _ambarellaStatus.photo_format = pf;
                uint32_t idx = 100000;
                for(uint32_t i = 0; i < NUM_PHOTO_FORMAT_VALUES; i++) {
                    if(pf == photoFormatOptions[i].mode) {
                        idx = i;
                        break;
                    }
                }
                if(idx < NUM_PHOTO_FORMAT_VALUES) {
                    _currentPhotoFmt = idx;
                    emit currentPhotoFmtChanged();
                }
            }
            //-- Metering
            uint32_t m = set.value(QString("metering_mode")).toString().toUInt();
            if(_ambarellaStatus.metering_mode != m && m < NUM_METERING_VALUES) {
                _ambarellaStatus.metering_mode = m;
                emit currentMeteringChanged();
            }
            _ambarellaStatus.rtsp_res        = set.value(QString("rtsp_res")).toString();
            _ambarellaStatus.photo_mode      = set.value(QString("photo_mode")).toString().toInt();
            _ambarellaStatus.photo_num       = set.value(QString("photo_num")).toString().toInt();
            _ambarellaStatus.photo_times     = set.value(QString("photo_num")).toString().toInt();
            _ambarellaStatus.ev_step         = set.value(QString("ev_step")).toString();
            _ambarellaStatus.interval_ms     = set.value(QString("interval_ms")).toString().toInt();
            _ambarellaStatus.cam_scene       = set.value(QString("cam_scene")).toString().toInt();
            _ambarellaStatus.audio_enable    = set.value(QString("audio_enable")).toString().toInt() != 0;
            _ambarellaStatus.left_time       = set.value(QString("left_time")).toString().toInt();
            _ambarellaStatus.audio_switch    = set.value(QString("audio_switch")).toString().toInt() != 0;
            _ambarellaStatus.x_ratio         = set.value(QString("x_ratio")).toString().toFloat();
            _ambarellaStatus.y_ratio         = set.value(QString("y_ratio")).toString().toFloat();
            _ambarellaStatus.layers          = set.value(QString("layers")).toString().toInt();
            _ambarellaStatus.pitch           = set.value(QString("pitch")).toString().toInt();
            _ambarellaStatus.yaw             = set.value(QString("yaw")).toString().toInt();
            _ambarellaStatus.timer_photo_sta = set.value(QString("timer_photo_sta")).toString().toInt();
            //-- If recording video, we do this more often
            if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
                _statusTimer.start(500);
            } else {
                _statusTimer.start(5000);
            }
        }
    } else {
        if(_cameraSupported == CAMERA_SUPPORT_UNDEFINED) {
            if(_httpErrorCount++ > 5) {
                _cameraSupported = CAMERA_SUPPORT_NO;
            } else {
                _statusTimer.start(500);
            }
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleCaptureStatus(const mavlink_message_t &message)
{
    //-- This is a response to MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS
    mavlink_camera_capture_status_t cap;
    mavlink_msg_camera_capture_status_decode(&message, &cap);
#if 0
    bool wasReady = (bool)_cameraStatus.data_ready;


    //-- Status
    QString status = set.value(QString("status")).toString();
    if(status != _ambarellaStatus.status) {
        bool was_running = videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING;
        _ambarellaStatus.status = status;
        emit videoStatusChanged();
        if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
            _videoSound.setLoopCount(1);
            _videoSound.play();
        } else if(was_running) {
            _videoSound.setLoopCount(2);
            _videoSound.play();
        }
    }



    _cameraStatus.data_ready = true;
    _cameraStatus.available_capacity    = cap.available_capacity;
    _cameraStatus.image_interval        = cap.image_interval;
    _cameraStatus.image_resolution_h    = cap.image_resolution_h;
    _cameraStatus.image_resolution_v    = cap.image_resolution_h;
    _cameraStatus.image_status          = cap.image_status;
    _updateVideoRes(cap.video_resolution_h, cap.video_resolution_v, cap.video_framerate);
    qCDebug(YuneecCameraLog) << "_handleCaptureStatus:" << cap.video_status << cap.recording_time_ms << cap.available_capacity;
    if(!wasReady || _cameraStatus.video_status != cap.video_status) {
        _cameraStatus.video_status = cap.video_status;
        emit videoStatusChanged();
    }
    _cameraStatus.recording_time_ms = cap.recording_time_ms;
    if(_cameraStatus.video_status == VIDEO_CAPTURE_STATUS_RUNNING) {
        //emit recordTimeChanged();
        //-- TODO: While the firmware doesn't send us these automatically, we ask for it
        QTimer::singleShot(1000, this, &CameraControl::_requestCaptureStatus);
    }
#endif
}

#if 0
//-----------------------------------------------------------------------------
void
CameraControl::_handleTakePhotoStatus(int http_code, QByteArray data)
{
    if(http_code == 200) {
        qCDebug(YuneecCameraLog) << "TAKE_PHOTO" << data;
        if(data.contains("status\":\"OK")) {
            _cameraSound.setLoopCount(1);
            _cameraSound.play();
        } else {
            _errorSound.setLoopCount(1);
            _errorSound.play();
        }
    }
}
#endif

//-----------------------------------------------------------------------------
CameraControl::VideoStatus
CameraControl::videoStatus()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        if(cameraMode() == CAMERA_MODE_VIDEO && _ambarellaStatus.status == "record")
            return VIDEO_CAPTURE_STATUS_RUNNING;
        else if(cameraMode() == CAMERA_MODE_VIDEO && _ambarellaStatus.status == "capture")
            return VIDEO_CAPTURE_STATUS_CAPTURE;
        else
            return VIDEO_CAPTURE_STATUS_STOPPED;
    }
    return VIDEO_CAPTURE_STATUS_UNDEFINED;
}

//-----------------------------------------------------------------------------
CameraControl::CameraMode
CameraControl::cameraMode()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        return (CameraMode)_ambarellaStatus.cam_mode;
    }
    return CAMERA_MODE_UNDEFINED;
}

//-----------------------------------------------------------------------------
CameraControl::AEModes
CameraControl::aeMode()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        return (AEModes)_ambarellaStatus.ae_enable;
    }
    return AE_MODE_UNDEFINED;
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
CameraControl::iqModeList()
{

    if(_iqModeList.size() == 0) {
        for(size_t i = 0; i < NUM_IQ_MODES; i++) {
            _iqModeList.append(iqModeOptions[i].description);
        }
    }
    return _iqModeList;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::wbList()
{
    if(_wbList.size() == 0) {
        for(size_t i = 0; i < NUM_WB_VALUES; i++) {
            _wbList.append(whiteBalanceOptions[i].description);
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
QStringList
CameraControl::meteringList()
{
    if(_meteringList.size() == 0) {
        for(size_t i = 0; i < NUM_METERING_VALUES; i++) {
            _meteringList.append(meteringModeOptions[i].description);
        }
    }
    return _meteringList;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::photoFormatList()
{
    if(_photoFormatList.size() == 0) {
        for(size_t i = 0; i < NUM_PHOTO_FORMAT_VALUES; i++) {
            _photoFormatList.append(photoFormatOptions[i].description);
        }
    }
    return _photoFormatList;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::evList()
{
    if(_evList.size() == 0) {
        for(uint32_t i = 0; i < NUM_EV_VALUES; i++) {
            _evList.append(evOptions[i].description);
        }
    }
    return _evList;
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
CameraControl::toggleVideo()
{
    if(videoStatus() == VIDEO_CAPTURE_STATUS_STOPPED) {
        startVideo();
    } else if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        stopVideo();
    }
}

//-----------------------------------------------------------------------------
quint32
CameraControl::recordTime()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        return _ambarellaStatus.record_time;
    }
    return 0;
}

//-----------------------------------------------------------------------------
QString
CameraControl::recordTimeStr()
{
    return QTime(0, 0).addSecs(recordTime()).toString("hh:mm:ss");
}

//-----------------------------------------------------------------------------
void
CameraControl::_updateAspectRatio()
{
    //-- Photo Mode
    if(_ambarellaStatus.cam_mode == CAMERA_MODE_PHOTO) {
        qCDebug(YuneecCameraLog) << "Set 4:3 Aspect Ratio";
        qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(1.333333);
    //-- Video Mode
    } else if(_ambarellaStatus.cam_mode == CAMERA_MODE_VIDEO && _currentVideoResIndex < NUM_VIDEO_RES) {
        qCDebug(YuneecCameraLog) << "Set Video Aspect Ratio" << videoResOptions[_currentVideoResIndex].aspectRatio;
        qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(videoResOptions[_currentVideoResIndex].aspectRatio);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_resetCameraValues()
{
    _true_cam_mode = CAMERA_MODE_UNDEFINED;

    _ambarellaStatus.cam_mode   = CAMERA_MODE_UNDEFINED;
    _ambarellaStatus.video_fps  = -1;
    _ambarellaStatus.video_h    = -1;
    _ambarellaStatus.video_w    = -1;

    _ambarellaStatus.status.clear();
    _ambarellaStatus.sdfree  = 0xFFFFFFFF;
    _ambarellaStatus.sdtotal = 0xFFFFFFFF;
    _ambarellaStatus.record_time   = 0;
    _ambarellaStatus.white_balance = 0;
    _ambarellaStatus.ae_enable = AE_MODE_UNDEFINED;
    _ambarellaStatus.iq_type = 0;
    _ambarellaStatus.exposure_value.clear();
    _ambarellaStatus.awb_lock = 0;
    _ambarellaStatus.audio_switch = 0;
    _ambarellaStatus.shutter_time.clear();
    _ambarellaStatus.iso_value.clear();
    _ambarellaStatus.photo_format.clear();
    _ambarellaStatus.rtsp_res.clear();
    _ambarellaStatus.photo_mode = 0;
    _ambarellaStatus.photo_num = 0;
    _ambarellaStatus.photo_times = 0;
    _ambarellaStatus.ev_step.clear();
    _ambarellaStatus.interval_ms = 0;
    _ambarellaStatus.cam_scene = 0;
    _ambarellaStatus.audio_enable = 0;
    _ambarellaStatus.left_time = 0;
    _ambarellaStatus.metering_mode = 0;
    _ambarellaStatus.x_ratio = 0.0f;
    _ambarellaStatus.y_ratio = 0.0f;
    _ambarellaStatus.layers = 0;
    _ambarellaStatus.pitch = 0;
    _ambarellaStatus.yaw = 0;
    _ambarellaStatus.timer_photo_sta = 0;
}
