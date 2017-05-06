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
// Video Resolution Options (CGO3+)
video_res_t videoResCGO3P[] = {
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

#define NUM_CGO3P_VIDEO_RES (sizeof(videoResCGO3P) / sizeof(video_res_t))

//-----------------------------------------------------------------------------
// Video Resolution Options (E90)
video_res_t videoResE90[] = {
    {"4096 x 2160 60fps (4K DCI)",    4096, 2160, 60,  4096.0f / 2160.0f},
    {"4096 x 2160 48fps (4K DCI)",    4096, 2160, 48,  4096.0f / 2160.0f},
    {"4096 x 2160 30fps (4K DCI)",    4096, 2160, 30,  4096.0f / 2160.0f},
    {"4096 x 2160 24fps (4K DCI)",    4096, 2160, 24,  4096.0f / 2160.0f},

    {"3840 x 2160 60fps (4K UHD)",    3840, 2160, 60,  3840.0f / 2160.0f},
    {"3840 x 2160 48fps (4K UHD)",    3840, 2160, 48,  3840.0f / 2160.0f},
    {"3840 x 2160 30fps (4K UHD)",    3840, 2160, 30,  3840.0f / 2160.0f},
    {"3840 x 2160 24fps (4K UHD)",    3840, 2160, 24,  3840.0f / 2160.0f},

    {"2515 x 1530 60fps (WQHD)",      2515, 1530, 60,  2515.0f / 1530.0f},
    {"2515 x 1530 48fps (WQHD)",      2515, 1530, 48,  2515.0f / 1530.0f},
    {"2515 x 1530 30fps (WQHD)",      2515, 1530, 30,  2515.0f / 1530.0f},
    {"2515 x 1530 24fps (WQHD)",      2515, 1530, 24,  2515.0f / 1530.0f},

    {"1920 x 1080 120fps (1080P)",    1920, 1080, 120, 1920.0f / 1080.0f},
    {"1920 x 1080 60fps (1080P)",     1920, 1080, 60,  1920.0f / 1080.0f},
    {"1920 x 1080 48fps (1080P)",     1920, 1080, 48,  1920.0f / 1080.0f},
    {"1920 x 1080 30fps (1080P)",     1920, 1080, 30,  1920.0f / 1080.0f},
    {"1920 x 1080 24fps (1080P)",     1920, 1080, 24,  1920.0f / 1080.0f},

    {"1280 x 720 120fps (720P)",      1280,  720, 120,  1280.0f /  720.0f},
    {"1280 x 720 60fps (720P)",       1280,  720, 60,   1280.0f /  720.0f},
    {"1280 x 720 48fps (720P)",       1280,  720, 48,   1280.0f /  720.0f},
    {"1280 x 720 30fps (720P)",       1280,  720, 30,   1280.0f /  720.0f},
    {"1280 x 720 24fps (720P)",       1280,  720, 24,   1280.0f /  720.0f}
};

#define NUM_E90_VIDEO_RES (sizeof(videoResE90) / sizeof(video_res_t))

//static video_res_t  current_camera = videoResCGO3P;
static quint32        current_camera_option_count = NUM_CGO3P_VIDEO_RES;

//-----------------------------------------------------------------------------
// Color Mode (CMD=SET_IQ_TYPE&mode=x)
iq_mode_t iqModeOptions[] = {
    {"Natural"},
    {"Enhanced"},
    {"Raw"},
    {"Night"}
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
    {"DNG"},
    {"Jpeg"},
    {"DNG + Jpeg"}
};

#define NUM_PHOTO_FORMAT_VALUES (sizeof(photoFormatOptions) / sizeof(photo_format_t))

//-----------------------------------------------------------------------------
// White Balance (CMD=SET_WHITEBLANCE_MODE&mode=x)
white_balance_t whiteBalanceOptions[] = {
    {"Auto",            0},
    {"Sunny",           6500},
    {"Cloudy",          7500},
    {"Fluorescent",     4000},
    {"Incandescent",    2800},
    {"Sunset",          5000}
};

#define NUM_WB_VALUES (sizeof(whiteBalanceOptions) / sizeof(white_balance_t))

//-----------------------------------------------------------------------------
// ISO Values
iso_values_t isoValues[] = {
    {"100",   100},
    {"150",   150},
    {"200",   200},
    {"300",   300},
    {"400",   400},
    {"600",   600},
    {"800",   800},
    {"1600", 1600},
    {"3200", 3200}
};

#define NUM_ISO_VALUES (sizeof(isoValues) / sizeof(iso_values_t))

//-----------------------------------------------------------------------------
// Shutter Speeds
shutter_speeds_t shutterSpeeds[] = {
    { "4s", 4.0f},
    { "3s", 3.0f},
    { "2s", 2.0f},
    { "1s", 1.0f},
    { "1/30",   1.0f/30.0f},
    { "1/60",   1.0f/60.0f},
    { "1/125",  1.0f/125.0f},
    { "1/250",  1.0f/250.0f},
    { "1/500",  1.0f/500.0f},
    { "1/1000", 1.0f/1000.0f},
    { "1/2000", 1.0f/2000.0f},
    { "1/4000", 1.0f/4000.0f},
    { "1/8000", 1.0f/8000.0f}
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
    , _currentTask(TIMER_GET_STORAGE_INFO)
    , _cameraSupported(CAMERA_SUPPORT_UNDEFINED)
    , _true_cam_mode(CAMERA_MODE_UNDEFINED)
    , _currentVideoResIndex(0)
    , _currentWB(0)
    , _currentIso(0)
    , _tempIso(0)
    , _currentShutter(0)
    , _tempShutter(0)
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
    _currentTask = TIMER_GET_STORAGE_INFO;
    emit cameraModeChanged();
    emit videoStatusChanged();
    emit photoStatusChanged();
    if(_vehicle) {
        _vehicle = NULL;
        disconnect(&_statusTimer, &QTimer::timeout, this, &CameraControl::_timerHandler);
        disconnect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &CameraControl::_mavlinkMessageReceived);
        disconnect(_vehicle, &Vehicle::mavCommandResult,       this, &CameraControl::_mavCommandResult);
        _cameraSupported = CAMERA_SUPPORT_UNDEFINED;
    }
    if(vehicle) {
        _vehicle = vehicle;
        connect(&_statusTimer, &QTimer::timeout, this, &CameraControl::_timerHandler);
        connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &CameraControl::_mavlinkMessageReceived);
        connect(_vehicle, &Vehicle::mavCommandResult,       this, &CameraControl::_mavCommandResult);
        _statusTimer.setSingleShot(true);
        //-- Ambarella Interface
        _initStreaming();
        //-- Request Camera Settings
        _requestCameraSettings();
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
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES && photoStatus() == PHOTO_CAPTURE_STATUS_IDLE) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_IMAGE_START_CAPTURE,                // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            0,                                          // Duration between two consecutive pictures (in seconds--ignored if single image)
            1,                                          // Number of images to capture total - 0 for unlimited capture
            -1,                                         // Horizontal resolution in pixels (set to -1 for highest resolution possible)
            -1);                                        // Vertical resolution in pixels (set to -1 for highest resolution possible)
        _startTimer(TIMER_GET_CAPTURE_INFO, 250);
        _cameraSound.setLoopCount(1);
        _cameraSound.play();
    } else {
        _errorSound.setLoopCount(1);
        _errorSound.play();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::startVideo()
{
    qCDebug(YuneecCameraLog) << "startVideo()";
    if(_vehicle && videoStatus() == VIDEO_CAPTURE_STATUS_STOPPED && _ambarellaSettings.cam_mode == CAMERA_MODE_VIDEO) {
        int w = -1;
        int h = -1;
        int f = -1;
        if(_setVideoResIndex < current_camera_option_count) {
            w = videoResCGO3P[_currentVideoResIndex].width;
            h = videoResCGO3P[_currentVideoResIndex].height;
            f = videoResCGO3P[_currentVideoResIndex].fps;
        }
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_VIDEO_START_CAPTURE,                // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras), 1 for first, 2 for second, etc.
            f,                                          // FPS: (-1 for max)
            w,                                          // Horizontal resolution in pixels (set to -1 for highest resolution possible)
            h);                                         // Vertical resolution in pixels (set to -1 for highest resolution possible)
        _startTimer(TIMER_GET_CAPTURE_INFO, 250);
        _videoSound.setLoopCount(1);
        _videoSound.play();
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
        _startTimer(TIMER_GET_CAPTURE_INFO, 250);
        _videoSound.setLoopCount(2);
        _videoSound.play();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setVideoMode()
{
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES && _ambarellaSettings.cam_mode != CAMERA_MODE_VIDEO) {
        qCDebug(YuneecCameraLog) << "setVideoMode()";
        //-- Force UI to update. We keep the real camera mode elsewhere so we
        //   track when the camera actually changed modes, which is quite some
        //   time later.
        _true_cam_mode = _ambarellaSettings.cam_mode;
        _ambarellaSettings.cam_mode = CAMERA_MODE_VIDEO;
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
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES && _ambarellaSettings.cam_mode == CAMERA_MODE_VIDEO) {
        qCDebug(YuneecCameraLog) << "setPhotoMode()";
        //-- Force UI to update. We keep the real camera mode elsewhere so we
        //   track when the camera actually changed modes, which is quite some
        //   time later.
        _true_cam_mode = _ambarellaSettings.cam_mode;
        _ambarellaSettings.cam_mode = CAMERA_MODE_PHOTO;
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
    if(index < current_camera_option_count) {
        qCDebug(YuneecCameraLog) << "setCurrentVideoRes:" << videoResCGO3P[index].description;
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
            whiteBalanceOptions[index].temperature);    // White balance (color temperature in K) (0: Auto WB)
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_setIsoShutter(int iso, float shutter)
{
    Q_UNUSED(iso);
    Q_UNUSED(shutter);
    _vehicle->sendMavCommand(
        MAV_COMP_ID_CAMERA,                         // Target component
        MAV_CMD_SET_CAMERA_SETTINGS_1,              // Command id
        true,                                       // ShowError
        1,                                          // Camera ID (1 for first, 2 for second, etc.)
        NAN,                                        // Aperture (1/value) (Fixed for CGO3+)
        shutter,                                    // Shutter speed in seconds
        iso,                                        // ISO sensitivity
        NAN,                                        // AE mode (Auto Exposure) (0: full auto 1: full manual 2: aperture priority 3: shutter priority)
        NAN,                                        // EV value (when in auto exposure)
        NAN);                                       // White balance (color temperature in K) (0: Auto WB)
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentIso(quint32 index)
{
    if(_vehicle && index < NUM_ISO_VALUES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentIso:" << isoValues[index].description;
        _tempIso = index;
        _setIsoShutter(isoValues[index].value, _tempShutter);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentShutter(quint32 index)
{
    if(_vehicle && index < NUM_SHUTTER_VALUES && _cameraSupported == CAMERA_SUPPORT_YES) {
        qCDebug(YuneecCameraLog) << "setCurrentShutter:" << shutterSpeeds[index].description;
        _tempShutter = index;
        _setIsoShutter(_tempIso, shutterSpeeds[index].value);
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
            index);                                     // Color mode ID (Neutral, Vivid, etc.)
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
            index,                                      // Image format ID (Jpeg/Raw/Jpeg+Raw)
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
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                         // Target component
            MAV_CMD_RESET_CAMERA_SETTINGS,              // Command id
            true,                                       // ShowError
            0,                                          // Camera ID (0 for all cameras, 1 for first, 2 for second, etc.)
            1);                                         // Do Reset
    }
}

//-----------------------------------------------------------------------------
int CameraControl::_findShutterSpeedIndex(float shutter_speed)
{
    //-- As accuracy, use 1/10 of the smallest shutter possible.
    const float accuracy = (1.0f / 8000.0f) / 10.0f;
    for (uint32_t i = 0; i < NUM_SHUTTER_VALUES; ++i) {
        float diff = fabsf(shutterSpeeds[i].value - shutter_speed);
        if (diff < accuracy) {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleISOStatus()
{
    /*
    for(uint32_t i = 0; i < NUM_ISO_VALUES; i++) {
        QString iso = QString("ISO_%1").arg(isoValues[i].description);
        if(_ambarellaSettings.iso_value == iso) {
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
int
CameraControl::_findVideoResIndex(int w, int h, float fps)
{
    for(uint32_t i = 0; i < current_camera_option_count; i++) {
        if(w == videoResCGO3P[i].width && h == videoResCGO3P[i].height && fps == videoResCGO3P[i].fps) {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
void
CameraControl::_timerHandler()
{
    switch(_currentTask) {
        case TIMER_GET_STORAGE_INFO:
            _requestStorageStatus();
            break;
        case TIMER_GET_CAPTURE_INFO:
            _requestCaptureStatus();
            break;
        case TIMER_GET_CAMERA_SETTINGS:
            _requestCameraSettings();
            break;
    }
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
            false,                                  // showError
            0,                                      // Camera ID (0 for all cameras, 1 for first, 2 for second, etc.)
            1);                                     // Do Request
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_requestStorageStatus()
{
    if(_vehicle) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_CAMERA,                     // target component
            MAV_CMD_REQUEST_STORAGE_INFORMATION,    // command id
            false,                                  // showError
            0,                                      // Storage ID (0 for all, 1 for first, 2 for second, etc.)
            1);                                     // Do Request
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_startTimer(int task, int elapsed)
{
    _currentTask = task;
    _statusTimer.start(elapsed);
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
                    _startTimer(TIMER_GET_STORAGE_INFO, 500);
                } else {
                    //-- We got an answer but not a good one
                    _cameraSupported = CAMERA_SUPPORT_NO;
                }
            }
        }
    } else if(_cameraSupported == CAMERA_SUPPORT_YES) {
        switch(command) {
            case MAV_CMD_IMAGE_START_CAPTURE:
                if(result != MAV_RESULT_ACCEPTED) {
                    _errorSound.setLoopCount(2);
                    _errorSound.play();
                }
                break;
            case MAV_CMD_REQUEST_STORAGE_INFORMATION:
                if(noReponseFromVehicle) {
                    qCDebug(YuneecCameraLog) << "Retry MAV_CMD_REQUEST_STORAGE_INFORMATION";
                    _currentTask = TIMER_GET_STORAGE_INFO;
                    _startTimer(TIMER_GET_STORAGE_INFO, 500);
                } else {
                    if(result != MAV_RESULT_ACCEPTED) {
                        qCDebug(YuneecCameraLog) << "Bad response from MAV_CMD_REQUEST_STORAGE_INFORMATION" << result << "Retrying...";
                        _startTimer(TIMER_GET_STORAGE_INFO, 500);
                    }
                }
                break;
            case MAV_CMD_REQUEST_CAMERA_SETTINGS:
                if(noReponseFromVehicle) {
                    qCDebug(YuneecCameraLog) << "Retry MAV_CMD_REQUEST_CAMERA_SETTINGS";
                    _startTimer(TIMER_GET_CAMERA_SETTINGS, 500);
                } else {
                    if(result != MAV_RESULT_ACCEPTED) {
                        qCDebug(YuneecCameraLog) << "Bad response from MAV_CMD_REQUEST_CAMERA_SETTINGS" << result << "Retrying...";
                        _startTimer(TIMER_GET_CAMERA_SETTINGS, 500);
                    } else {
                        _startTimer(TIMER_GET_STORAGE_INFO, 500);
                    }
                }
                break;
            case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
                if(noReponseFromVehicle) {
                    qCDebug(YuneecCameraLog) << "Retry MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS";
                    _startTimer(TIMER_GET_CAPTURE_INFO, 1000);
                } else {
                    if(result != MAV_RESULT_ACCEPTED) {
                        qCDebug(YuneecCameraLog) << "Bad response from MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS" << result << "Retrying...";
                        _startTimer(TIMER_GET_CAPTURE_INFO, 1000);
                    }
                }
                break;
            case MAV_CMD_SET_CAMERA_SETTINGS_1:
            case MAV_CMD_SET_CAMERA_SETTINGS_2:
            case MAV_CMD_RESET_CAMERA_SETTINGS:
                if(!noReponseFromVehicle && result == MAV_RESULT_ACCEPTED) {
                    _startTimer(TIMER_GET_CAMERA_SETTINGS, 250);
                } else {
                    _errorSound.setLoopCount(1);
                    _errorSound.play();
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
        case MAVLINK_MSG_ID_STORAGE_INFORMATION:
            _handleStorageInfo(message);
            break;
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleCameraSettings(const mavlink_message_t& message)
{
    mavlink_camera_settings_t settings;
    mavlink_msg_camera_settings_decode(&message, &settings);
    qCDebug(YuneecCameraLog) << "_handleCameraSettings:" << settings.mode_id << settings.color_mode_id << settings.ev << settings.exposure_mode << settings.image_format_id << settings.iso_sensitivity << settings.metering_mode_id << settings.shutter_speed;
    //-- Auto Exposure Mode
    int ae = settings.exposure_mode == 0 ? AE_MODE_AUTO : AE_MODE_MANUAL;
    if(_ambarellaSettings.ae_enable != ae) {
        _ambarellaSettings.ae_enable = ae;
        emit aeModeChanged();
    }
    //-- Shutter Speed
    int idx = _findShutterSpeedIndex(settings.shutter_speed);
    if(idx >= 0) {
        _tempShutter = idx;
        _currentShutter = idx;
        emit currentShutterChanged();
    }
    //-- ISO Value
    // TODO: Waiting to see what is returned.
    // For now, all I get is NAN.
    //-- EV
    if(_ambarellaSettings.exposure_value != settings.ev) {
        uint32_t idx = 100000;
        for(uint32_t i = 0; i < NUM_EV_VALUES; i++) {
            if(settings.ev == evOptions[i].value) {
                idx = i;
                break;
            }
        }
        if(idx < NUM_EV_VALUES) {
            _currentEV = idx;
            emit currentEVChanged();
        }
    }
    //-- White Balance
    quint32 wb = (quint32)settings.white_balance;
    if(_ambarellaSettings.white_balance != wb) {
        _ambarellaSettings.white_balance = wb;
        for(uint32_t i = 0; i < NUM_WB_VALUES; i++) {
            if(whiteBalanceOptions[i].temperature == wb) {
                _currentWB = i;
                break;
            }
        }
        emit currentWBChanged();
    }
    //-- Camera Mode
    int cam_mode = CAMERA_MODE_UNDEFINED;
    if(settings.mode_id == 0)
        cam_mode = CAMERA_MODE_PHOTO;
    else if(settings.mode_id == 1)
        cam_mode = CAMERA_MODE_VIDEO;
    //-- Camera mode switch takes too long so we switch the UI right
    //   after the user presses the switch. Internally however, we only
    //   truly find out the mode once we get an answer from the camera.
    if(_true_cam_mode != cam_mode) {
        _true_cam_mode = cam_mode;
        _ambarellaSettings.cam_mode = cam_mode;
        emit cameraModeChanged();
        _updateAspectRatio();
    }
    //-- Audio Enabled
    bool record_enabled = settings.audio_recording == 1;
    if(_ambarellaSettings.audio_switch != record_enabled) {
        _ambarellaSettings.audio_switch = record_enabled;
        //-- TODO
    }
    //-- Color IQ
    if(_ambarellaSettings.iq_type != settings.color_mode_id && settings.color_mode_id < NUM_IQ_MODES) {
        _ambarellaSettings.iq_type = settings.color_mode_id;
        emit currentIQChanged();
    }
    //-- Photo Format
    if(_ambarellaSettings.photo_format != settings.color_mode_id) {
        _ambarellaSettings.photo_format = settings.color_mode_id;
        emit currentPhotoFmtChanged();
    }
    //-- Image Quality (Compression)
    if(_ambarellaSettings.photo_quality != settings.image_quality_id) {
        _ambarellaSettings.photo_quality = settings.image_quality_id;
        //-- TODO
    }
    //-- Metering
    if(_ambarellaSettings.metering_mode != settings.metering_mode_id && settings.metering_mode_id < NUM_METERING_VALUES) {
        _ambarellaSettings.metering_mode = settings.metering_mode_id;
        emit currentMeteringChanged();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleCaptureStatus(const mavlink_message_t &message)
{
    //-- This is a response to MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS
    mavlink_camera_capture_status_t cap;
    mavlink_msg_camera_capture_status_decode(&message, &cap);
    qCDebug(YuneecCameraLog) << "_handleCaptureStatus:" << cap.available_capacity << cap.image_interval << cap.image_resolution_h << cap.image_resolution_v << cap.image_status << cap.recording_time_ms << cap.video_framerate << cap.video_resolution_h << cap.video_resolution_v << cap.video_status;
    //-- Image Capture Status
    if(_ambarellaStatus.image_status != cap.image_status) {
        _ambarellaStatus.image_status = cap.image_status;
        emit photoStatusChanged();
    }
    //-- Video Capture Status
    if(_ambarellaStatus.video_status != cap.video_status) {
        _ambarellaStatus.video_status = cap.video_status;
        emit videoStatusChanged();
    }
    //-- Current Video Resolution and FPS
    int idx = _findVideoResIndex(cap.video_resolution_h, cap.video_resolution_v, cap.video_framerate);
    if((int)_currentVideoResIndex != idx && idx >= 0) {
        _currentVideoResIndex = idx;
        emit currentVideoResChanged();
        _updateAspectRatio();
    }
    //-- Recording running time
    if(_ambarellaStatus.record_time != cap.recording_time_ms) {
        _ambarellaStatus.record_time = cap.recording_time_ms;
        emit recordTimeChanged();
    }
    //-- If recording video, we do this more often
    if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _startTimer(TIMER_GET_CAPTURE_INFO, 500);
    } else if(photoStatus() == PHOTO_CAPTURE_STATUS_RUNNING) {
        _startTimer(TIMER_GET_CAPTURE_INFO, 1000);
    } else {
        _startTimer(TIMER_GET_CAPTURE_INFO, 5000);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleStorageInfo(const mavlink_message_t& message)
{
    mavlink_storage_information_t st;
    mavlink_msg_storage_information_decode(&message, &st);
    qCDebug(YuneecCameraLog) << "_handleStorageInfo:" << st.available_capacity << st.status << st.storage_count << st.storage_id << st.total_capacity << st.used_capacity;
    if(_ambarellaStatus.sdtotal != st.total_capacity) {
        _ambarellaStatus.sdtotal = st.total_capacity;
        emit sdTotalChanged();
    }
    if(_ambarellaStatus.sdfree != st.available_capacity) {
        _ambarellaStatus.sdfree = st.available_capacity;
        emit sdFreeChanged();
    }
    //-- Get Capture Status next
    _startTimer(TIMER_GET_CAPTURE_INFO, 500);
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
        return (VideoStatus)_ambarellaStatus.video_status;
    }
    return VIDEO_CAPTURE_STATUS_UNDEFINED;
}

//-----------------------------------------------------------------------------
CameraControl::PhotoStatus
CameraControl::photoStatus()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        return (PhotoStatus)_ambarellaStatus.image_status;
    }
    return PHOTO_CAPTURE_STATUS_UNDEFINED;
}

//-----------------------------------------------------------------------------
CameraControl::CameraMode
CameraControl::cameraMode()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        return (CameraMode)_ambarellaSettings.cam_mode;
    }
    return CAMERA_MODE_UNDEFINED;
}

//-----------------------------------------------------------------------------
CameraControl::AEModes
CameraControl::aeMode()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        return (AEModes)_ambarellaSettings.ae_enable;
    }
    return AE_MODE_UNDEFINED;
}

//-----------------------------------------------------------------------------
QStringList
CameraControl::videoResList()
{

    if(_videoResList.size() == 0) {
        for(size_t i = 0; i < current_camera_option_count; i++) {
            _videoResList.append(videoResCGO3P[i].description);
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
    return QTime(0, 0).addMSecs(recordTime()).toString("hh:mm:ss");
}

//-----------------------------------------------------------------------------
void
CameraControl::_updateAspectRatio()
{
    //-- Photo Mode
    if(_ambarellaSettings.cam_mode == CAMERA_MODE_PHOTO) {
        qCDebug(YuneecCameraLog) << "Set 4:3 Aspect Ratio";
        qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(1.333333);
    //-- Video Mode
    } else if(_ambarellaSettings.cam_mode == CAMERA_MODE_VIDEO && _currentVideoResIndex < current_camera_option_count) {
        qCDebug(YuneecCameraLog) << "Set Video Aspect Ratio" << videoResCGO3P[_currentVideoResIndex].aspectRatio;
        qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(videoResCGO3P[_currentVideoResIndex].aspectRatio);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_resetCameraValues()
{
    _true_cam_mode = CAMERA_MODE_UNDEFINED;

    _ambarellaSettings.ae_enable        = AE_MODE_UNDEFINED;
    _ambarellaSettings.exposure_value   = 0.0f;
    _ambarellaSettings.cam_mode         = CAMERA_MODE_UNDEFINED;
    _ambarellaSettings.audio_switch     = false;
    _ambarellaSettings.iq_type          = 0;
    _ambarellaSettings.photo_format     = 0;
    _ambarellaSettings.photo_quality    = 100;
    _ambarellaSettings.white_balance    = 0;
    _ambarellaSettings.metering_mode    = 0;

    _ambarellaStatus.image_status       = PHOTO_CAPTURE_STATUS_UNDEFINED;
    _ambarellaStatus.video_status       = VIDEO_CAPTURE_STATUS_UNDEFINED;
    _ambarellaStatus.sdfree             = 0xFFFFFFFF;
    _ambarellaStatus.sdtotal            = 0xFFFFFFFF;
    _ambarellaStatus.record_time        = 0;
}
