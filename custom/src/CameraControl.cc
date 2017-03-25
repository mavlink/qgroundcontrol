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

#include <QJsonParseError>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(YuneecCameraLog, "YuneecCameraLog")

static const char* kAmbCommand = "http://192.168.42.1/cgi-bin/cgi?CMD=";

//-----------------------------------------------------------------------------
// Video Resolution Options (CMD=SET_VIDEO_MODE&video_mode=4096x2160F25)
video_res_t videoResOptions[] = {
    {"4096 x 2160 25fps (4K DCI)",    "4096x2160F25",  4096.0 / 2160.0},
    {"4096 x 2160 24fps (4K DCI)",    "4096x2160F24",  4096.0 / 2160.0},
    {"3840 x 2160 30fps (4K UHD)",    "3840x2160F30",  3840.0 / 2160.0},
    {"3840 x 2160 25fps (4K UHD)",    "3840x2160F25",  3840.0 / 2160.0},
    {"3840 x 2160 24fps (4K UHD)",    "3840x2160F24",  3840.0 / 2160.0},
    {"2560 x 1440 30fps (WQHD)",      "2560x1440F30",  2560.0 / 1440.0},
    {"2560 x 1440 25fps (WQHD)",      "2560x1440F25",  2560.0 / 1440.0},
    {"2560 x 1440 24fps (WQHD)",      "2560x1440F24",  2560.0 / 1440.0},
    {"1920 x 1080 120fps (1080P)",    "1920x1080F120", 1920.0 / 1080.0},
    {"1920 x 1080 60fps (1080P)",     "1920x1080F60",  1920.0 / 1080.0},
    {"1920 x 1080 50fps (1080P)",     "1920x1080F50",  1920.0 / 1080.0},
    {"1920 x 1080 48fps (1080P)",     "1920x1080F48",  1920.0 / 1080.0},
    {"1920 x 1080 30fps (1080P)",     "1920x1080F30",  1920.0 / 1080.0},
    {"1920 x 1080 25fps (1080P)",     "1920x1080F25",  1920.0 / 1080.0},
    {"1920 x 1080 24fps (1080P)",     "1920x1080F24",  1920.0 / 1080.0}
};

#define NUM_VIDEO_RES (sizeof(videoResOptions) / sizeof(video_res_t))

//-----------------------------------------------------------------------------
// Color Mode (CMD=SET_IQ_TYPE&mode=x)
color_mode_t colorModeOptions[] = {
    {"Natural",  0},
    {"Enhanced", 1},
    {"Raw",      2},
    {"Night",    3}
};

//-----------------------------------------------------------------------------
// Metering Mode (CMD=SET_METERING_MODE&mode=x)
metering_mode_t meteringModeOptions[] = {
    {"Spot",    0},
    {"Center",  1},
    {"Average", 2}
};

//-----------------------------------------------------------------------------
// Metering Mode (CMD=SET_PHOTO_FORMAT&format=dng)
photo_format_t photoFormatOptions[] = {
    {"DNG",         "dng"},
    {"Jpeg",        "jpg"},
    {"DNG + Jpeg",  "dng+jpg"}
};

//-----------------------------------------------------------------------------
// White Balance (CMD=SET_WHITEBLANCE_MODE&mode=x)
white_balance_t whiteBalanceOptions[] = {
    {"Auto",            0},
    {"Sunny",           4},
    {"Cloudy",          5},
    {"Fluorescent",     7},
    {"Incandescent",    1},
    {"Sunset",          3},
    {"Lock",            99}
};

#define NUM_WB_VALUES (sizeof(whiteBalanceOptions) / sizeof(white_balance_t))

//-----------------------------------------------------------------------------
// ISO Values
iso_values_t isoValues[] = {
    {"100",  "ISO_100"},
    {"150",  "ISO_150"},
    {"200",  "ISO_200"},
    {"300",  "ISO_300"},
    {"400",  "ISO_400"},
    {"600",  "ISO_600"},
    {"800",  "ISO_800"},
    {"1600", "ISO_1600"},
    {"3200", "ISO_3200"},
    {"Auto", ""}
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
    { "1/8000", "8000"},
    { "Auto", 1.0f}
};

#define NUM_SHUTTER_VALUES (sizeof(shutterSpeeds) / sizeof(shutter_speeds_t))

//-----------------------------------------------------------------------------
CameraControl::CameraControl(QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _waitingShutter(false)
    , _cameraSupported(CAMERA_SUPPORT_UNDEFINED)
    , _httpErrorCount(0)
    , _currentVideoResIndex(0)
    , _currentWb(0)
    , _currentIso(NUM_ISO_VALUES - 1)
    , _currentShutter(NUM_SHUTTER_VALUES - 1)
    , _networkManager(NULL)
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
CameraControl::~CameraControl()
{
    if(_networkManager) {
        delete _networkManager;
    }
}

//-----------------------------------------------------------------------------
QNetworkAccessManager*
CameraControl::networkManager()
{
    if(!_networkManager) {
        _networkManager = new QNetworkAccessManager(this);
    }
    return _networkManager;
}

//-----------------------------------------------------------------------------
void
CameraControl::setVehicle(Vehicle* vehicle)
{
    if(_vehicle) {
        _vehicle = NULL;
    }
    if(vehicle) {
        _vehicle = vehicle;
        _cameraSupported = CAMERA_SUPPORT_UNDEFINED;
        _httpErrorCount = 0;
        emit cameraModeChanged();
        emit videoStatusChanged();
        //-- Ambarella Interface
        _initStreaming();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_sendAmbRequest(QNetworkRequest request)
{
    QNetworkReply* reply = networkManager()->get(request);
    connect(reply, &QNetworkReply::finished,  this, &CameraControl::_httpFinished);
    QTimer::singleShot(100, this, &CameraControl::_getCameraStatus);
}

//-----------------------------------------------------------------------------
void
CameraControl::_initStreaming()
{
    //-- Set RTSP resolution to 480P
    _sendAmbRequest(QNetworkRequest(QString("%1SET_RTSP_VID&Reslution=480P").arg(kAmbCommand)));
}

//-----------------------------------------------------------------------------
void
CameraControl::_getCameraStatus()
{
    _sendAmbRequest(QNetworkRequest(QString("%1GET_STATUS").arg(kAmbCommand)));
}

//-----------------------------------------------------------------------------
void
CameraControl::takePhoto()
{
    qCDebug(YuneecCameraLog) << "takePhoto()";
    if(_vehicle && _cameraSupported == CAMERA_SUPPORT_YES) {
        if(_waitingShutter) {
            _errorSound.setLoopCount(1);
            _errorSound.play();
        } else {
            _waitingShutter = true;
            _sendAmbRequest(QNetworkRequest(QString("%1TAKE_PHOTO").arg(kAmbCommand)));
        }
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::startVideo()
{
    qCDebug(YuneecCameraLog) << "startVideo()";
    if(_vehicle && videoStatus() == VIDEO_CAPTURE_STATUS_STOPPED && _amb_cam_status.cam_mode == CAMERA_MODE_VIDEO) {
        _sendAmbRequest(QNetworkRequest(QString("%1START_RECORD").arg(kAmbCommand)));
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
        _sendAmbRequest(QNetworkRequest(QString("%1STOP_RECORD").arg(kAmbCommand)));
        _videoSound.setLoopCount(2);
        _videoSound.play();
        QTimer::singleShot(250, this, &CameraControl::_requestCaptureStatus);
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setVideoMode()
{
    qCDebug(YuneecCameraLog) << "setVideoMode()";
    _setCamMode("video");
}

//-----------------------------------------------------------------------------
void
CameraControl::setPhotoMode()
{
    qCDebug(YuneecCameraLog) << "setPhotoMode()";
    _setCamMode("photo");
}

//-----------------------------------------------------------------------------
void
CameraControl::_setCamMode(const char* mode)
{
    _sendAmbRequest(QNetworkRequest(QString("%1SET_CAM_MODE&mode=%2").arg(kAmbCommand).arg(mode)));
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentVideoRes(quint32 index)
{
    if(index < NUM_VIDEO_RES) {
        qCDebug(YuneecCameraLog) << "setCurrentVideoRes:" << videoResOptions[index].description;
        _sendAmbRequest(QNetworkRequest(QString("%1SET_VIDEO_MODE&video_mode=%2").arg(kAmbCommand).arg(videoResOptions[index].video_mode)));
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleVideoResStatus()
{
    for(int i = 0; i < NUM_VIDEO_RES; i++) {
        if(_amb_cam_status.video_mode == videoResOptions[i].video_mode) {
            if(_currentVideoResIndex != i) {
                _currentVideoResIndex = i;
                emit currentVideoResChanged();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_httpFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(reply) {
        const int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray data = reply->readAll();
        const QNetworkRequest request = reply->request();
        QString url = request.url().toString();
        url.replace(kAmbCommand, "");
        qCDebug(YuneecLogVerbose) << QString("%1 HTTP Result:").arg(url) << http_code;
        if(url.contains("GET_STATUS")) {
            _handleCameraStatus(http_code, data);
        } else if(url.contains("TAKE_PHOTO")) {
            _handleTakePhotoStatus(http_code, data);
        }
        reply->deleteLater();
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleCameraStatus(int http_code, QByteArray data)
{
    if(http_code == 200) {
        qCDebug(YuneecLogVerbose) << "GET_STATUS" << data;
        _cameraSupported = CAMERA_SUPPORT_YES;
        QJsonParseError jsonParseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError) {
            qWarning() <<  "Unable to parse camera status" << jsonParseError.errorString();
        } else {
            QJsonObject set = doc.object();
            _amb_cam_status.rval            = set.value(QString("rval")).toInt();
            _amb_cam_status.msg_id          = set.value(QString("msg_id")).toInt();
            //-- Camera Mode
            int cam_mode = set.value(QString("cam_mode")).toString().toInt();
            if(_amb_cam_status.cam_mode != cam_mode) {
                _amb_cam_status.cam_mode = cam_mode;
                emit cameraModeChanged();
            }
            //-- Status
            QString status = set.value(QString("status")).toString();
            if(status != _amb_cam_status.status) {
                _amb_cam_status.status = status;
                emit videoStatusChanged();
            }
            _amb_cam_status.sdfree          = set.value(QString("sdfree")).toString().toUInt();
            _amb_cam_status.sdtotal         = set.value(QString("sdtotal")).toString().toUInt();
            //-- Recording Time
            int rec_time = set.value(QString("record_time")).toString().toUInt();
            if(_amb_cam_status.record_time != rec_time) {
                _amb_cam_status.record_time = rec_time;
                emit recordTimeChanged();
            }
            _amb_cam_status.white_balance   = set.value(QString("white_balance")).toString().toInt();
            _amb_cam_status.ae_enabled      = set.value(QString("ae_enabled")).toString().toInt() != 0;
            _amb_cam_status.iq_type         = set.value(QString("iq_type")).toString().toInt();
            _amb_cam_status.exposure_value  = set.value(QString("exposure_value")).toString().toFloat();
            //-- Current Video Resolution and FPS
            _amb_cam_status.video_mode      = set.value(QString("video_mode")).toString();
            _handleVideoResStatus();
            _amb_cam_status.awb_lock        = set.value(QString("awb_lock")).toString().toInt();
            _amb_cam_status.audio_switch    = set.value(QString("audio_switch")).toString().toInt() != 0;
            _amb_cam_status.shutter_time    = set.value(QString("shutter_time")).toString().toInt();
            _amb_cam_status.iso_value       = set.value(QString("iso_value")).toString();
            _amb_cam_status.photo_format    = set.value(QString("photo_format")).toString();
            _amb_cam_status.rtsp_res        = set.value(QString("rtsp_res")).toString();
            _amb_cam_status.photo_mode      = set.value(QString("photo_mode")).toString().toInt();
            _amb_cam_status.photo_num       = set.value(QString("photo_num")).toString().toInt();
            _amb_cam_status.photo_times     = set.value(QString("photo_num")).toString().toInt();
            _amb_cam_status.ev_step         = set.value(QString("ev_step")).toString().toFloat();
            _amb_cam_status.interval_ms     = set.value(QString("interval_ms")).toString().toInt();
            _amb_cam_status.cam_scene       = set.value(QString("cam_scene")).toString().toInt();
            _amb_cam_status.audio_enable    = set.value(QString("audio_enable")).toString().toInt() != 0;
            _amb_cam_status.left_time       = set.value(QString("left_time")).toString().toInt();
            _amb_cam_status.metering_mode   = set.value(QString("metering_mode")).toString().toInt();
            _amb_cam_status.x_ratio         = set.value(QString("x_ratio")).toString().toFloat();
            _amb_cam_status.y_ratio         = set.value(QString("y_ratio")).toString().toFloat();
            _amb_cam_status.layers          = set.value(QString("layers")).toString().toInt();
            _amb_cam_status.pitch           = set.value(QString("pitch")).toString().toInt();
            _amb_cam_status.yaw             = set.value(QString("yaw")).toString().toInt();
            _amb_cam_status.timer_photo_sta = set.value(QString("timer_photo_sta")).toString().toInt();
            //-- If recording video, we do this more often
            if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
                QTimer::singleShot(500, this, &CameraControl::_getCameraStatus);
            } else {
                QTimer::singleShot(2000, this, &CameraControl::_getCameraStatus);
            }
        }
    } else {
        if(_cameraSupported == CAMERA_SUPPORT_UNDEFINED) {
            if(_httpErrorCount++ > 5) {
                _cameraSupported = CAMERA_SUPPORT_NO;
            } else {
                QTimer::singleShot(500, this, &CameraControl::_getCameraStatus);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleTakePhotoStatus(int http_code, QByteArray data)
{
    if(http_code == 200) {
        qCDebug(YuneecLogVerbose) << "TAKE_PHOTO" << data;
        if(_waitingShutter) {
            if(data.contains("status\":\"OK")) {
                _waitingShutter = false;
            } else {
                qWarning() << "TAKE_PHOTO Not OK";
            }
        }
    }
}

//-----------------------------------------------------------------------------
CameraControl::VideoStatus
CameraControl::videoStatus()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        if(_amb_cam_status.status == "recording")
            return VIDEO_CAPTURE_STATUS_RUNNING;
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
        qCDebug(YuneecCameraLog) << "Get cameraMode:" << _amb_cam_status.cam_mode;
        return (CameraMode)_amb_cam_status.cam_mode;
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
        return _amb_cam_status.record_time;
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
void
CameraControl::_setSettings_1(float p1, float p2, float p3, float p4, float p5, float p6, float p7)
{
    if (_vehicle && _cameraStatus.data_ready) {
        qCDebug(YuneecCameraLog) << "_setSettings_1()" << p1 << p2 << p3 << p4 << p5 << p6 << p7;
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
        qCDebug(YuneecCameraLog) << "_setSettings_2()" << p1 << p2 << p3 << p4 << p5 << p6;
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
    _currentVideoResIndex = _findVideoRes(w, h, f);
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
CameraControl::setCurrentWb(quint32 index)
{
    if(index < NUM_WB_VALUES) {
        qCDebug(YuneecCameraLog) << "setCurrentWb:" << wbValues[index].description;
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
CameraControl::_updateAspectRatio()
{
    qCDebug(YuneecCameraLog) << "_updateAspectRatio() Mode:" << _cameraSettings.mode_id;
    if(_cameraSettings.data_ready) {
        //-- Photo Mode
        if(_cameraSettings.mode_id == CAMERA_MODE_PHOTO) {
            qCDebug(YuneecCameraLog) << "Set 4:3 Aspect Ratio";
            qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(1.333333);
        //-- Video Mode
        } else if(_cameraSettings.mode_id == CAMERA_MODE_VIDEO && _currentVideoResIndex < (int)NUM_VIDEO_RES) {
            qCDebug(YuneecCameraLog) << "Set Video Aspect Ratio" << videoResOptions[_currentVideoResIndex].aspectRatio;
            qgcApp()->toolbox()->settingsManager()->videoSettings()->aspectRatio()->setRawValue(videoResOptions[_currentVideoResIndex].aspectRatio);
        }
    }
}
