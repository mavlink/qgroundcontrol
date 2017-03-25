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
    {"DNG",         "dng"},
    {"Jpeg",        "jpg"},
    {"DNG + Jpeg",  "dng+jpg"}
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
    {"Sunset",          3},
    {"Lock",            99}
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
    {"3200"},
    {"Auto"}
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
    { "Auto", ""}
};

#define NUM_SHUTTER_VALUES (sizeof(shutterSpeeds) / sizeof(shutter_speeds_t))

//-----------------------------------------------------------------------------
// Exposure Compensation
exposure_compsensation_t evOptions[] = {
    { "-2.0", "-2.0"},
    { "-1.5", "-1.5"},
    { "-1.0", "-1.0"},
    { "-0.5", "-0.5"},
    {   "0",  "0.0"},
    { "+0.5", "0.5"},
    { "+1.0", "1.0"},
    { "+1.5", "1.5"},
    { "+2.0", "2.0"},
};

#define NUM_EV_VALUES (sizeof(evOptions) / sizeof(exposure_compsensation_t))

//-----------------------------------------------------------------------------
CameraControl::CameraControl(QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _cameraSupported(CAMERA_SUPPORT_UNDEFINED)
    , _httpErrorCount(0)
    , _currentVideoResIndex(0)
    , _currentWB(0)
    , _currentIso(NUM_ISO_VALUES - 1)
    , _currentShutter(NUM_SHUTTER_VALUES - 1)
    , _currentPhotoFmt(1)
    , _networkManager(NULL)
{
    _resetCameraValues();
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
        disconnect(&_statusTimer, &QTimer::timeout, this, &CameraControl::_getCameraStatus);
    }
    if(vehicle) {
        _vehicle = vehicle;
        _resetCameraValues();
        _cameraSupported = CAMERA_SUPPORT_UNDEFINED;
        _httpErrorCount = 0;
        emit cameraModeChanged();
        emit videoStatusChanged();
        connect(&_statusTimer, &QTimer::timeout, this, &CameraControl::_getCameraStatus);
        _statusTimer.setSingleShot(true);
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
    _statusTimer.start(100);
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
        _sendAmbRequest(QNetworkRequest(QString("%1TAKE_PHOTO").arg(kAmbCommand)));
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::startVideo()
{
    qCDebug(YuneecCameraLog) << "startVideo()";
    if(_vehicle && videoStatus() == VIDEO_CAPTURE_STATUS_STOPPED && _ambarellaStatus.cam_mode == CAMERA_MODE_VIDEO) {
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
CameraControl::setCurrentWB(quint32 index)
{
    if(index < NUM_WB_VALUES) {
        qCDebug(YuneecCameraLog) << "setCurrentWb:" << whiteBalanceOptions[index].description;
        _sendAmbRequest(QNetworkRequest(QString("%1SET_WHITEBLANCE_MODE&mode=%2").arg(kAmbCommand).arg(whiteBalanceOptions[index].mode)));
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentIso(quint32 index)
{
    if(index < NUM_ISO_VALUES) {
        qCDebug(YuneecCameraLog) << "setCurrentIso:" << isoValues[index].description;
        //-- If ISO is "locked", set Auto Mode
        if(index == (NUM_ISO_VALUES - 1)) {
            setAeMode(AE_MODE_AUTO);
        } else {
            _sendAmbRequest(QNetworkRequest(QString("%1SET_SH_TM_ISO&time=%2&value=ISO_%3").arg(kAmbCommand).arg(_ambarellaStatus.shutter_time).arg(isoValues[index].description)));
        }
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentShutter(quint32 index)
{
    if(index < NUM_SHUTTER_VALUES) {
        qCDebug(YuneecCameraLog) << "setCurrentShutter:" << shutterSpeeds[index].description;
        //-- If Shutter Speed is "locked", set Auto Mode
        if(index == (NUM_SHUTTER_VALUES - 1)) {
            setAeMode(AE_MODE_AUTO);
        } else {
            _sendAmbRequest(QNetworkRequest(QString("%1SET_SH_TM_ISO&time=%2&value=%3").arg(kAmbCommand).arg(shutterSpeeds[index].value).arg(_ambarellaStatus.iso_value)));
        }
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentIQ(quint32 index)
{
    if(index < NUM_IQ_MODES) {
        qCDebug(YuneecCameraLog) << "setCurrentIQ:" << iqModeOptions[index].description;
        _sendAmbRequest(QNetworkRequest(QString("%1SET_IQ_TYPE&mode=%2").arg(kAmbCommand).arg(iqModeOptions[index].mode)));
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentPhotoFmt(quint32 index)
{
    if(index < NUM_PHOTO_FORMAT_VALUES) {
        qCDebug(YuneecCameraLog) << "setCurrentPhotoFmt:" << photoFormatOptions[index].description;
        _sendAmbRequest(QNetworkRequest(QString("%1SET_PHOTO_FORMAT&format=%2").arg(kAmbCommand).arg(photoFormatOptions[index].mode)));
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentMetering(quint32 index)
{
    if(index < NUM_METERING_VALUES) {
        qCDebug(YuneecCameraLog) << "setCurrentMetering:" << meteringModeOptions[index].description;
        _sendAmbRequest(QNetworkRequest(QString("%1SET_METERING_MODE&mode=%2").arg(kAmbCommand).arg(meteringModeOptions[index].mode)));
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setCurrentEV(quint32 index)
{
    if(index < NUM_EV_VALUES) {
        qCDebug(YuneecCameraLog) << "setCurrentEV:" << evOptions[index].description;
        _sendAmbRequest(QNetworkRequest(QString("%1SET_EXPOSURE_VALUE&mode=%2").arg(kAmbCommand).arg(evOptions[index].value)));
    }
}

//-----------------------------------------------------------------------------
void
CameraControl::setAeMode(AEMode mode)
{
    qCDebug(YuneecCameraLog) << "setAeMode:" << mode;
    _sendAmbRequest(QNetworkRequest(QString("%1SET_AE_ENABLE&mode=%2").arg(kAmbCommand).arg(mode)));
}

//-----------------------------------------------------------------------------
void
CameraControl::formatCard()
{
    _sendAmbRequest(QNetworkRequest(QString("%1FORMAT_CARD").arg(kAmbCommand)));
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleVideoResStatus()
{
    for(uint32_t i = 0; i < NUM_VIDEO_RES; i++) {
        if(_ambarellaStatus.video_mode == videoResOptions[i].video_mode) {
            if(_currentVideoResIndex != i) {
                _currentVideoResIndex = i;
                emit currentVideoResChanged();
                return;
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
        //qCDebug(YuneecLogVerbose) << "GET_STATUS" << data;
        _cameraSupported = CAMERA_SUPPORT_YES;
        QJsonParseError jsonParseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError) {
            qWarning() <<  "Unable to parse camera status" << jsonParseError.errorString();
        } else {
            QJsonObject set = doc.object();
            _ambarellaStatus.rval            = set.value(QString("rval")).toInt();
            _ambarellaStatus.msg_id          = set.value(QString("msg_id")).toInt();
            //-- Camera Mode
            int cam_mode = set.value(QString("cam_mode")).toString().toInt();
            if(_ambarellaStatus.cam_mode != cam_mode) {
                _ambarellaStatus.cam_mode = cam_mode;
                emit cameraModeChanged();
            }
            //-- Status
            QString status = set.value(QString("status")).toString();
            if(status != _ambarellaStatus.status) {
                _ambarellaStatus.status = status;
                emit videoStatusChanged();
            }
            //-- Storage
            quint32 sdfree = set.value(QString("sdfree")).toString().toUInt();
            if(_ambarellaStatus.sdfree != sdfree) {
                _ambarellaStatus.sdfree = sdfree;
                emit sdFreeChanged();
            }
            quint32 sdTotal = set.value(QString("sdtotal")).toString().toUInt();
            if(_ambarellaStatus.sdtotal != sdTotal) {
                _ambarellaStatus.sdtotal = sdTotal;
                emit sdTotalChanged();
            }
            //-- Recording Time
            uint32_t rec_time = set.value(QString("record_time")).toString().toUInt();
            if(_ambarellaStatus.record_time != rec_time) {
                _ambarellaStatus.record_time = rec_time;
                emit recordTimeChanged();
            }
            //-- White Balance
            uint32_t wbLock = set.value(QString("awb_lock")).toString().toUInt();
            uint32_t wb = set.value(QString("white_balance")).toString().toUInt();
            if(_ambarellaStatus.white_balance != wb || _ambarellaStatus.awb_lock != wbLock) {
                _ambarellaStatus.white_balance = wb;
                _ambarellaStatus.awb_lock = wbLock;
                if(wbLock) {
                    _currentWB = NUM_WB_VALUES - 1;
                } else {
                    for(uint32_t i = 0; i < NUM_WB_VALUES; i++) {
                        if(whiteBalanceOptions[i].mode == wb) {
                            _currentWB = i;
                            break;
                        }
                    }
                }
                emit currentWbChanged();
            }
            //-- Auto Exposure Mode
            int ae = set.value(QString("ae_enabled")).toString().toInt();
            if(_ambarellaStatus.ae_enabled != ae) {
                _ambarellaStatus.ae_enabled = ae;
                emit aeModeChanged();
                //-- If AE enabled, lock ISO and Shutter
                if(ae == 0) {
                    _currentIso = NUM_ISO_VALUES - 1;
                    _currentShutter = NUM_SHUTTER_VALUES - 1;
                    emit currentIsoChanged();
                    emit currentShutterChanged();
                }
            }
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
            //-- Shutter and ISO (Manual Mode)
            _ambarellaStatus.shutter_time    = set.value(QString("shutter_time")).toString();
            _ambarellaStatus.iso_value       = set.value(QString("iso_value")).toString();
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
                _statusTimer.start(2000);
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
}

//-----------------------------------------------------------------------------
void
CameraControl::_handleTakePhotoStatus(int http_code, QByteArray data)
{
    if(http_code == 200) {
        qCDebug(YuneecLogVerbose) << "TAKE_PHOTO" << data;
        if(data.contains("status\":\"OK")) {
            _cameraSound.setLoopCount(1);
            _cameraSound.play();
        } else {
            _errorSound.setLoopCount(1);
            _errorSound.play();
        }
    }
}

//-----------------------------------------------------------------------------
CameraControl::VideoStatus
CameraControl::videoStatus()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        if(_ambarellaStatus.status == "recording")
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
        qCDebug(YuneecCameraLog) << "Get cameraMode:" << _ambarellaStatus.cam_mode;
        return (CameraMode)_ambarellaStatus.cam_mode;
    }
    return CAMERA_MODE_UNDEFINED;
}

//-----------------------------------------------------------------------------
CameraControl::AEMode
CameraControl::aeMode()
{
    if(_cameraSupported == CAMERA_SUPPORT_YES) {
        return (AEMode)_ambarellaStatus.ae_enabled;
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
    /*
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
    */
}

//-----------------------------------------------------------------------------
void
CameraControl::_resetCameraValues()
{
    _ambarellaStatus.rval = 0;
    _ambarellaStatus.msg_id = 0;
    _ambarellaStatus.cam_mode = 0;
    _ambarellaStatus.status.clear();
    _ambarellaStatus.sdfree = 0;
    _ambarellaStatus.sdtotal = 0;
    _ambarellaStatus.record_time = 0;
    _ambarellaStatus.white_balance = 0;
    _ambarellaStatus.ae_enabled = 0;
    _ambarellaStatus.iq_type = 0;
    _ambarellaStatus.exposure_value.clear();
    _ambarellaStatus.video_mode.clear();
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
