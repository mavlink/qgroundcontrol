/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCCameraControl.h"
#include "QGCCameraIO.h"
#include "SettingsManager.h"
#include "VideoManager.h"
#include "QGCMapEngine.h"

#include "QGCCameraManager.h"
#include "QGCVideoStreamInfo.h"
#include "QGCCameraOption.h"

#include <QDir>
#include <QStandardPaths>
#include <QDomDocument>
#include <QDomNodeList>

QGC_LOGGING_CATEGORY(CameraControlLog, "CameraControlLog")
QGC_LOGGING_CATEGORY(CameraControlVerboseLog, "CameraControlVerboseLog")

static const char* kPhotoMode       = "PhotoMode";
static const char* kPhotoLapse      = "PhotoLapse";
static const char* kPhotoLapseCount = "PhotoLapseCount";
static const char* kThermalOpacity  = "ThermalOpacity";
static const char* kThermalMode     = "ThermalMode";

//-----------------------------------------------------------------------------
// Known Parameters
const char* QGCCameraControl::kCAM_EV          = "CAM_EV";
const char* QGCCameraControl::kCAM_EXPMODE     = "CAM_EXPMODE";
const char* QGCCameraControl::kCAM_ISO         = "CAM_ISO";
const char* QGCCameraControl::kCAM_SHUTTERSPD  = "CAM_SHUTTERSPD";
const char* QGCCameraControl::kCAM_APERTURE    = "CAM_APERTURE";
const char* QGCCameraControl::kCAM_WBMODE      = "CAM_WBMODE";
const char* QGCCameraControl::kCAM_MODE        = "CAM_MODE";



//-----------------------------------------------------------------------------
QGCCameraControl::QGCCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : FactGroup(0, parent, true /* ignore camel case */)
    , _vehicle(vehicle)
    , _compID(compID)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    memcpy(&_info, info, sizeof(mavlink_camera_information_t));

     _definitionFileHandler = new QGCCameraDefinitionFileHandler(this);
     connect(_definitionFileHandler, &QGCCameraDefinitionFileHandler::constantsUpdated, this, &QGCCameraControl::_processConstants);
     connect(_definitionFileHandler, &QGCCameraDefinitionFileHandler::factsUpdated, this, &QGCCameraControl::_processFacts);

    _downloader = new QGCCameraHttpDownloader(this);
    connect(_downloader, &QGCCameraHttpDownloader::dataReady, this, &QGCCameraControl::_dataReady);
    connect(this, &QGCCameraControl::dataReady, this, &QGCCameraControl::_dataReady);

    _vendor = QString(reinterpret_cast<const char*>(info->vendor_name));
    _modelName = QString(reinterpret_cast<const char*>(info->model_name));
    int ver = static_cast<int>(_info.cam_definition_version);
    _cacheFile = QString::asprintf("%s/%s_%s_%03d.xml",
        qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str(),
        _vendor.toStdString().c_str(),
        _modelName.toStdString().c_str(),
        ver);
    if(info->cam_definition_uri[0] != 0) {
        //-- Process camera definition file
        _handleDefinitionFile(info->cam_definition_uri);
    } else {
        _initWhenReady();
    }
    QSettings settings;
    _photoMode       = static_cast<PhotoMode>(settings.value(kPhotoMode, static_cast<int>(PHOTO_CAPTURE_SINGLE)).toInt());
    _photoLapse      = settings.value(kPhotoLapse, 1.0).toDouble();
    _photoLapseCount = settings.value(kPhotoLapseCount, 0).toInt();
    _thermalOpacity  = settings.value(kThermalOpacity, 85.0).toDouble();
    _thermalMode     = static_cast<ThermalViewMode>(settings.value(kThermalMode, static_cast<uint32_t>(THERMAL_BLEND)).toUInt());
    _recTimer.setSingleShot(false);
    _recTimer.setInterval(333);
    connect(&_recTimer, &QTimer::timeout, this, &QGCCameraControl::_recTimerHandler);
}

QGCCameraControl::~QGCCameraControl() {
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_initWhenReady()
{
    qCDebug(CameraControlLog) << "_initWhenReady()";
    if(isBasic()) {
        qCDebug(CameraControlLog) << "Basic, MAVLink only messages.";
        _requestCameraSettings();
        QTimer::singleShot(250, this, &QGCCameraControl::_checkForVideoStreams);
        //-- Basic cameras have no parameters
        _paramComplete = true;
        emit parametersReady();
    } else {
        _requestAllParameters();
        //-- Give some time to load the parameters before going after the camera settings
        QTimer::singleShot(2000, this, &QGCCameraControl::_requestCameraSettings);
    }
    connect(_vehicle, &Vehicle::mavCommandResult, this, &QGCCameraControl::_mavCommandResult);
    connect(&_captureStatusTimer, &QTimer::timeout, this, &QGCCameraControl::_requestCaptureStatus);
    _captureStatusTimer.setSingleShot(true);
    QTimer::singleShot(2500, this, &QGCCameraControl::_requestStorageInfo);
    _captureStatusTimer.start(2750);
    emit infoChanged();
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::firmwareVersion()
{
    int major = (_info.firmware_version >> 24) & 0xFF;
    int minor = (_info.firmware_version >> 16) & 0xFF;
    int build = _info.firmware_version & 0xFFFF;
    return QString::asprintf("%d.%d.%d", major, minor, build);
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::recordTimeStr()
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString("hh:mm:ss");
}

//-----------------------------------------------------------------------------
QGCCameraControl::VideoStatus
QGCCameraControl::videoStatus()
{
    return _video_status;
}

//-----------------------------------------------------------------------------
QGCCameraControl::PhotoStatus
QGCCameraControl::photoStatus()
{
    return _photo_status;
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::storageFreeStr()
{
    return QGCMapEngine::storageFreeSizeToString(static_cast<quint64>(_storageFree));
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::batteryRemainingStr()
{
    if(_batteryRemaining >= 0) {
        return QGCMapEngine::numberToString(static_cast<quint64>(_batteryRemaining)) + " %";
    }
    return "";
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setCameraMode(CameraMode mode)
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "setCameraMode(" << mode << ")";
        if(mode == CAM_MODE_VIDEO) {
            setVideoMode();
        } else if(mode == CAM_MODE_PHOTO) {
            setPhotoMode();
        } else {
            qCDebug(CameraControlLog) << "setCameraMode() Invalid mode:" << mode;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoMode(PhotoMode mode)
{
    if(!_resetting) {
        _photoMode = mode;
        QSettings settings;
        settings.setValue(kPhotoMode, static_cast<int>(mode));
        emit photoModeChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoLapse(qreal interval)
{
    _photoLapse = interval;
    QSettings settings;
    settings.setValue(kPhotoLapse, interval);
    emit photoLapseChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoLapseCount(int count)
{
    _photoLapseCount = count;
    QSettings settings;
    settings.setValue(kPhotoLapseCount, count);
    emit photoLapseCountChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_setCameraMode(CameraMode mode)
{
    if(_cameraMode != mode) {
        _cameraMode = mode;
        emit cameraModeChanged();
        //-- Update stream status
        _streamStatusTimer.start(1000);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::toggleMode()
{
    if(!_resetting) {
        if(cameraMode() == CAM_MODE_PHOTO || cameraMode() == CAM_MODE_SURVEY) {
            setVideoMode();
        } else if(cameraMode() == CAM_MODE_VIDEO) {
            setPhotoMode();
        }
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::toggleVideo()
{
    if(!_resetting) {
        if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
            return stopVideo();
        } else {
            return startVideo();
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::takePhoto()
{
    qCDebug(CameraControlLog) << "takePhoto()";
    //-- Check if camera can capture photos or if it can capture it while in Video Mode
    if(!capturesPhotos()) {
        qCWarning(CameraControlLog) << "Camera does not handle image capture";
        return false;
    }
    if(cameraMode() == CAM_MODE_VIDEO && !photosInVideoMode()) {
        qCWarning(CameraControlLog) << "Camera does not handle image capture while in video mode";
        return false;
    }
    if(photoStatus() != PHOTO_CAPTURE_IDLE) {
        qCWarning(CameraControlLog) << "Camera not idle";
        return false;
    }
    if(!_resetting) {
        if(capturesPhotos()) {
            _vehicle->sendMavCommand(
                _compID,                                                                    // Target component
                MAV_CMD_IMAGE_START_CAPTURE,                                                // Command id
                false,                                                                      // ShowError
                0,                                                                          // Reserved (Set to 0)
                static_cast<float>(_photoMode == PHOTO_CAPTURE_SINGLE ? 0 : _photoLapse),   // Duration between two consecutive pictures (in seconds--ignored if single image)
                _photoMode == PHOTO_CAPTURE_SINGLE ? 1 : _photoLapseCount);                 // Number of images to capture total - 0 for unlimited capture
            _setPhotoStatus(PHOTO_CAPTURE_IN_PROGRESS);
            _captureInfoRetries = 0;
            //-- Capture local image as well
            if(qgcApp()->toolbox()->videoManager()) {
                qgcApp()->toolbox()->videoManager()->grabImage();
            }
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::stopTakePhoto()
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "stopTakePhoto()";
        if(photoStatus() == PHOTO_CAPTURE_IDLE || (photoStatus() != PHOTO_CAPTURE_INTERVAL_IDLE && photoStatus() != PHOTO_CAPTURE_INTERVAL_IN_PROGRESS)) {
            return false;
        }
        if(capturesPhotos()) {
            _vehicle->sendMavCommand(
                _compID,                                                    // Target component
                MAV_CMD_IMAGE_STOP_CAPTURE,                                 // Command id
                false,                                                      // ShowError
                0);                                                         // Reserved (Set to 0)
            _setPhotoStatus(PHOTO_CAPTURE_IDLE);
            _captureInfoRetries = 0;
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::startVideo()
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "startVideo()";
        //-- Check if camera can capture videos or if it can capture it while in Photo Mode
        if(!capturesVideo() || (cameraMode() == CAM_MODE_PHOTO && !videoInPhotoMode())) {
            return false;
        }
        if(videoStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
            _vehicle->sendMavCommand(
                _compID,                                    // Target component
                MAV_CMD_VIDEO_START_CAPTURE,                // Command id
                false,                                      // Don't Show Error (handle locally)
                0,                                          // Reserved (Set to 0)
                0);                                         // CAMERA_CAPTURE_STATUS Frequency
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::stopVideo()
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "stopVideo()";
        if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
            _vehicle->sendMavCommand(
                _compID,                                    // Target component
                MAV_CMD_VIDEO_STOP_CAPTURE,                 // Command id
                false,                                      // Don't Show Error (handle locally)
                0);                                         // Reserved (Set to 0)
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setVideoMode()
{
    if(!_resetting && hasModes()) {
        qCDebug(CameraControlLog) << "setVideoMode()";
        //-- Does it have a mode parameter?
        Fact* pMode = mode();
        if(pMode) {
            if(cameraMode() != CAM_MODE_VIDEO) {
                pMode->setRawValue(CAM_MODE_VIDEO);
                _setCameraMode(CAM_MODE_VIDEO);
            }
        } else {
            //-- Use MAVLink Command
            if(_cameraMode != CAM_MODE_VIDEO) {
                //-- Use basic MAVLink message
                _vehicle->sendMavCommand(
                    _compID,                                // Target component
                    MAV_CMD_SET_CAMERA_MODE,                // Command id
                    true,                                   // ShowError
                    0,                                      // Reserved (Set to 0)
                    CAM_MODE_VIDEO);                        // Camera mode (0: photo, 1: video)
                _setCameraMode(CAM_MODE_VIDEO);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoMode()
{
    if(!_resetting && hasModes()) {
        qCDebug(CameraControlLog) << "setPhotoMode()";
        //-- Does it have a mode parameter?
        Fact* pMode = mode();
        if(pMode) {
            if(cameraMode() != CAM_MODE_PHOTO) {
                pMode->setRawValue(CAM_MODE_PHOTO);
                _setCameraMode(CAM_MODE_PHOTO);
            }
        } else {
            //-- Use MAVLink Command
            if(_cameraMode != CAM_MODE_PHOTO) {
                //-- Use basic MAVLink message
                _vehicle->sendMavCommand(
                    _compID,                                // Target component
                    MAV_CMD_SET_CAMERA_MODE,                // Command id
                    true,                                   // ShowError
                    0,                                      // Reserved (Set to 0)
                    CAM_MODE_PHOTO);                        // Camera mode (0: photo, 1: video)
                _setCameraMode(CAM_MODE_PHOTO);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setThermalMode(ThermalViewMode mode)
{
    QSettings settings;
    settings.setValue(kThermalMode, static_cast<uint32_t>(mode));
    _thermalMode = mode;
    emit thermalModeChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setThermalOpacity(double val)
{
    if(val < 0.0) val = 0.0;
    if(val > 100.0) val = 100.0;
    if(fabs(_thermalOpacity - val) > 0.1) {
        _thermalOpacity = val;
        QSettings settings;
        settings.setValue(kThermalOpacity, val);
        emit thermalOpacityChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setZoomLevel(qreal level)
{
    qCDebug(CameraControlLog) << "setZoomLevel()" << level;
    if(hasZoom()) {
        //-- Limit
        level = std::min(std::max(level, 0.0), 100.0);
        if(_vehicle) {
            _vehicle->sendMavCommand(
                _compID,                                // Target component
                MAV_CMD_SET_CAMERA_ZOOM,                // Command id
                false,                                  // ShowError
                ZOOM_TYPE_RANGE,                        // Zoom type
                static_cast<float>(level));             // Level
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setFocusLevel(qreal level)
{
    qCDebug(CameraControlLog) << "setFocusLevel()" << level;
    if(hasFocus()) {
        //-- Limit
        level = std::min(std::max(level, 0.0), 100.0);
        if(_vehicle) {
            _vehicle->sendMavCommand(
                _compID,                                // Target component
                MAV_CMD_SET_CAMERA_FOCUS,               // Command id
                false,                                  // ShowError
                FOCUS_TYPE_RANGE,                       // Focus type
                static_cast<float>(level));             // Level
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::resetSettings()
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "resetSettings()";
        _resetting = true;
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_RESET_CAMERA_SETTINGS,          // Command id
            true,                                   // ShowError
            1);                                     // Do Reset
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::formatCard(int id)
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "formatCard()";
        if(_vehicle) {
            _vehicle->sendMavCommand(
                _compID,                                // Target component
                MAV_CMD_STORAGE_FORMAT,                 // Command id
                true,                                   // ShowError
                id,                                     // Storage ID (1 for first, 2 for second, etc.)
                1);                                     // Do Format
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::stepZoom(int direction)
{
    qCDebug(CameraControlLog) << "stepZoom()" << direction;
    if(_vehicle && hasZoom()) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_ZOOM,                // Command id
            false,                                  // ShowError
            ZOOM_TYPE_STEP,                         // Zoom type
            direction);                             // Direction (-1 wide, 1 tele)
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::startZoom(int direction)
{
    qCDebug(CameraControlLog) << "startZoom()" << direction;
    if(_vehicle && hasZoom()) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_ZOOM,                // Command id
            false,                                  // ShowError
            ZOOM_TYPE_CONTINUOUS,                   // Zoom type
            direction);                             // Direction (-1 wide, 1 tele)
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::stopZoom()
{
    qCDebug(CameraControlLog) << "stopZoom()";
    if(_vehicle && hasZoom()) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_ZOOM,                // Command id
            false,                                  // ShowError
            ZOOM_TYPE_CONTINUOUS,                   // Zoom type
            0);                                     // Direction (-1 wide, 1 tele)
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestCaptureStatus()
{
    qCDebug(CameraControlLog) << "_requestCaptureStatus()";

    _vehicle->sendMavCommand(
        _compID,                                // target component
        MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS,  // command id
        false,                                  // showError
        1);                                     // Do Request
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::factChanged(Fact* pFact)
{
    _updateActiveList();
    _updateRanges(pFact);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    //-- Is this ours?
    if(_vehicle->id() != vehicleId || compID() != component) {
        return;
    }
    if(!noReponseFromVehicle && result == MAV_RESULT_IN_PROGRESS) {
        //-- Do Nothing
        qCDebug(CameraControlLog) << "In progress response for" << command;
    }else if(!noReponseFromVehicle && result == MAV_RESULT_ACCEPTED) {
        switch(command) {
            case MAV_CMD_RESET_CAMERA_SETTINGS:
                _resetting = false;
                if(isBasic()) {
                    _requestCameraSettings();
                } else {
                    QTimer::singleShot(500, this, &QGCCameraControl::_requestAllParameters);
                    QTimer::singleShot(2500, this, &QGCCameraControl::_requestCameraSettings);
                }
                break;
            case MAV_CMD_VIDEO_START_CAPTURE:
                _setVideoStatus(VIDEO_CAPTURE_STATUS_RUNNING);
                _captureStatusTimer.start(1000);
                break;
            case MAV_CMD_VIDEO_STOP_CAPTURE:
                _setVideoStatus(VIDEO_CAPTURE_STATUS_STOPPED);
                _captureStatusTimer.start(1000);
                break;
            case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
                _captureInfoRetries = 0;
                break;
            case MAV_CMD_REQUEST_STORAGE_INFORMATION:
                _storageInfoRetries = 0;
                break;
            case MAV_CMD_IMAGE_START_CAPTURE:
                _captureStatusTimer.start(1000);
                break;
        }
    } else {
        if(noReponseFromVehicle || result == MAV_RESULT_TEMPORARILY_REJECTED || result == MAV_RESULT_FAILED) {
            if(noReponseFromVehicle) {
                qCDebug(CameraControlLog) << "No response for" << command;
            } else if (result == MAV_RESULT_TEMPORARILY_REJECTED) {
                qCDebug(CameraControlLog) << "Command temporarily rejected for" << command;
            } else {
                qCDebug(CameraControlLog) << "Command failed for" << command;
            }
            switch(command) {
                case MAV_CMD_IMAGE_START_CAPTURE:
                case MAV_CMD_IMAGE_STOP_CAPTURE:
                    if(++_captureInfoRetries < 3) {
                        _captureStatusTimer.start(1000);
                    } else {
                        qCDebug(CameraControlLog) << "Giving up start/stop image capture";
                        _setPhotoStatus(PHOTO_CAPTURE_IDLE);
                    }
                    break;
                case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
                    if(++_captureInfoRetries < 3) {
                        _captureStatusTimer.start(500);
                    } else {
                        qCDebug(CameraControlLog) << "Giving up requesting capture status";
                    }
                    break;
                case MAV_CMD_REQUEST_STORAGE_INFORMATION:
                    if(++_storageInfoRetries < 3) {
                        QTimer::singleShot(500, this, &QGCCameraControl::_requestStorageInfo);
                    } else {
                        qCDebug(CameraControlLog) << "Giving up requesting storage status";
                    }
                    break;
            }
        } else {
            qCDebug(CameraControlLog) << "Bad response for" << command << result;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_setVideoStatus(VideoStatus status)
{
    if(_video_status != status) {
        _video_status = status;
        emit videoStatusChanged();
        if(status == VIDEO_CAPTURE_STATUS_RUNNING) {
             _recordTime = 0;
             _recTime = QTime::currentTime();
             _recTimer.start();
        } else {
             _recTimer.stop();
             _recordTime = 0;
             emit recordTimeChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_recTimerHandler()
{
    _recordTime = static_cast<uint32_t>(_recTime.msecsTo(QTime::currentTime()));
    emit recordTimeChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_setPhotoStatus(PhotoStatus status)
{
    if(_photo_status != status) {
        qCDebug(CameraControlLog) << "Set Photo Status:" << status;
        _photo_status = status;
        emit photoStatusChanged();
    }
}



//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestAllParameters()
{
    //-- Reset receive list
    for(const QString& paramName: _paramIO.keys()) {
        if(_paramIO[paramName]) {
            _paramIO[paramName]->setParamRequest();
        } else {
            qCritical() << "QGCParamIO is NULL" << paramName;
        }
    }
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();

        MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t msg;
        mavlink_msg_param_ext_request_list_pack_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    static_cast<uint8_t>(_vehicle->id()),
                    static_cast<uint8_t>(compID()),
                    0);                                                 // trimmed messages = false
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
    qCDebug(CameraControlVerboseLog) << "Request all parameters";
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::_getParamName(const char* param_id)
{
    QByteArray bytes(param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
    QString parameterName(bytes);
    return parameterName;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleParamAck(const mavlink_param_ext_ack_t& ack)
{
    QString paramName = _getParamName(ack.param_id);
    if(!_paramIO.contains(paramName)) {
        qCWarning(CameraControlLog) << "Received PARAM_EXT_ACK for unknown param:" << paramName;
        return;
    }
    if(_paramIO[paramName]) {
        _paramIO[paramName]->handleParamAck(ack);
    } else {
        qCritical() << "QGCParamIO is NULL" << paramName;
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleParamValue(const mavlink_param_ext_value_t& value)
{
    QString paramName = _getParamName(value.param_id);
    if(!_paramIO.contains(paramName)) {
        qCWarning(CameraControlLog) << "Received PARAM_EXT_VALUE for unknown param:" << paramName;
        return;
    }
    if(_paramIO[paramName]) {
        _paramIO[paramName]->handleParamValue(value);
    } else {
        qCritical() << "QGCParamIO is NULL" << paramName;
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_updateActiveList()
{
    //-- Clear out excluded parameters based on exclusion rules
    QStringList exclusionList;
    for(QGCCameraOptionExclusion* param: _valueExclusions) {
        Fact* pFact = getFact(param->param);
        if(pFact) {
            QString option = pFact->rawValueString();
            if(param->value == option) {
                exclusionList << param->exclusions;
            }
        }
    }
    QStringList active;
    for(QString key: _settings) {
        if(!exclusionList.contains(key)) {
            active.append(key);
        }
    }
    if(active != _activeSettings) {
        qCDebug(CameraControlVerboseLog) << "Excluding" << exclusionList;
        _activeSettings = active;
        emit activeSettingsChanged();
        //-- Force validity of "Facts" based on active set
        if(_paramComplete) {
            emit parametersReady();
        }
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_processConditionTest(const QString conditionTest)
{
    enum {
        TEST_NONE,
        TEST_EQUAL,
        TEST_NOT_EQUAL,
        TEST_GREATER,
        TEST_SMALLER
    };
    qCDebug(CameraControlVerboseLog) << "_processConditionTest(" << conditionTest << ")";
    int op = TEST_NONE;
    QStringList test;

    auto split = [&conditionTest](const QString& sep ) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        return conditionTest.split(sep, QString::SkipEmptyParts);
#else
        return conditionTest.split(sep, Qt::SkipEmptyParts);
#endif
    };

    if(conditionTest.contains("!=")) {
        test = split("!=");
        op = TEST_NOT_EQUAL;
    } else if(conditionTest.contains("=")) {
        test = split("=");
        op = TEST_EQUAL;
    } else if(conditionTest.contains(">")) {
        test = split(">");
        op = TEST_GREATER;
    } else if(conditionTest.contains("<")) {
        test = split("<");
        op = TEST_SMALLER;
    }
    if(test.size() == 2) {
        Fact* pFact = getFact(test[0]);
        if(pFact) {
            switch(op) {
            case TEST_EQUAL:
                return pFact->rawValueString() == test[1];
            case TEST_NOT_EQUAL:
                return pFact->rawValueString() != test[1];
            case TEST_GREATER:
                return pFact->rawValueString() > test[1];
            case TEST_SMALLER:
                return pFact->rawValueString() < test[1];
            case TEST_NONE:
                break;
            }
        } else {
            qWarning() << "Invalid condition parameter:" << test[0] << "in" << conditionTest;
            return false;
        }
    }
    qWarning() << "Invalid condition" << conditionTest;
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_processCondition(const QString condition)
{
    qCDebug(CameraControlVerboseLog) << "_processCondition(" << condition << ")";
    bool result = true;
    bool andOp  = true;
    if(!condition.isEmpty()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        QStringList scond = condition.split(" ", QString::SkipEmptyParts);
#else
        QStringList scond = condition.split(" ", Qt::SkipEmptyParts);
#endif
        while(scond.size()) {
            QString test = scond.first();
            scond.removeFirst();
            if(andOp) {
                result = result && _processConditionTest(test);
            } else {
                result = result || _processConditionTest(test);
            }
            if(!scond.size()) {
                return result;
            }
            andOp = scond.first().toUpper() == "AND";
            scond.removeFirst();
        }
    }
    return result;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_updateRanges(Fact* pFact)
{
    QMap<Fact*, QGCCameraOptionRange*> rangesSet;
    QMap<Fact*, QString> rangesReset;
    QStringList changedList;
    QStringList resetList;
    QStringList updates;
    //-- Iterate range sets looking for limited ranges
    for(QGCCameraOptionRange* pRange: _optionRanges) {
        //-- If this fact or one of its conditions is part of this range set
        if(!changedList.contains(pRange->targetParam) && (pRange->param == pFact->name() || pRange->condition.contains(pFact->name()))) {
            Fact* pRFact = getFact(pRange->param);          //-- This parameter
            Fact* pTFact = getFact(pRange->targetParam);    //-- The target parameter (the one its range is to change)
            if(pRFact && pTFact) {
                //qCDebug(CameraControlVerboseLog) << "Check new set of options for" << pTFact->name();
                QString option = pRFact->rawValueString();  //-- This parameter value
                //-- If this value (and condition) triggers a change in the target range
                //qCDebug(CameraControlVerboseLog) << "Range value:" << pRange->value << "Current value:" << option << "Condition:" << pRange->condition;
                if(pRange->value == option && _processCondition(pRange->condition)) {
                    if(pTFact->enumStrings() != pRange->optNames) {
                        //-- Set limited range set
                        rangesSet[pTFact] = pRange;
                    }
                    changedList << pRange->targetParam;
                }
            }
        }
    }
    //-- Iterate range sets again looking for resets
    for(QGCCameraOptionRange* pRange: _optionRanges) {
        if(!changedList.contains(pRange->targetParam) && (pRange->param == pFact->name() || pRange->condition.contains(pFact->name()))) {
            Fact* pTFact = getFact(pRange->targetParam);    //-- The target parameter (the one its range is to change)
            if(!resetList.contains(pRange->targetParam)) {
                if(pTFact->enumStrings() != _originalOptNames[pRange->targetParam]) {
                    //-- Restore full option set
                    rangesReset[pTFact] = pRange->targetParam;
                }
                resetList << pRange->targetParam;
            }
        }
    }
    //-- Update limited range set
    for (Fact* f: rangesSet.keys()) {
        f->setEnumInfo(rangesSet[f]->optNames, rangesSet[f]->optVariants);
        if(!updates.contains(f->name())) {
            _paramIO[f->name()]->optNames = rangesSet[f]->optNames;
            _paramIO[f->name()]->optVariants = rangesSet[f]->optVariants;
            emit f->enumsChanged();
            qCDebug(CameraControlVerboseLog) << "Limited set of options for:" << f->name() << rangesSet[f]->optNames;;
            updates << f->name();
        }
    }
    //-- Restore full range set
    for (Fact* f: rangesReset.keys()) {
        f->setEnumInfo(_originalOptNames[rangesReset[f]], _originalOptValues[rangesReset[f]]);
        if(!updates.contains(f->name())) {
            _paramIO[f->name()]->optNames = _originalOptNames[rangesReset[f]];
            _paramIO[f->name()]->optVariants = _originalOptValues[rangesReset[f]];
            emit f->enumsChanged();
            qCDebug(CameraControlVerboseLog) << "Restore full set of options for:" << f->name() << _originalOptNames[f->name()];
            updates << f->name();
        }
    }
    //-- Parameter update requests
    if(_requestUpdates.contains(pFact->name())) {
        for(const QString& param: _requestUpdates[pFact->name()]) {
            if(!_updatesToRequest.contains(param)) {
                _updatesToRequest << param;
            }
        }
    }
    if(_updatesToRequest.size()) {
        QTimer::singleShot(500, this, &QGCCameraControl::_requestParamUpdates);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestParamUpdates()
{
    for(const QString& param: _updatesToRequest) {
        _paramIO[param]->paramRequest();
    }
    _updatesToRequest.clear();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestCameraSettings()
{
    qCDebug(CameraControlLog) << "_requestCameraSettings()";
    if(_vehicle) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_REQUEST_CAMERA_SETTINGS,        // command id
            false,                                  // showError
            1);                                     // Do Request
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestStorageInfo()
{
    qCDebug(CameraControlLog) << "_requestStorageInfo()";
    if(_vehicle) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_REQUEST_STORAGE_INFORMATION,    // command id
            false,                                  // showError
            0,                                      // Storage ID (0 for all, 1 for first, 2 for second, etc.)
            1);                                     // Do Request
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleSettings(const mavlink_camera_settings_t& settings)
{
    qCDebug(CameraControlLog) << "handleSettings() Mode:" << settings.mode_id;
    _setCameraMode(static_cast<CameraMode>(settings.mode_id));
    qreal z = static_cast<qreal>(settings.zoomLevel);
    qreal f = static_cast<qreal>(settings.focusLevel);
    if(std::isfinite(z) && z != _zoomLevel) {
        _zoomLevel = z;
        emit zoomLevelChanged();
    }
    if(std::isfinite(f) && f != _focusLevel) {
        _focusLevel = f;
        emit focusLevelChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleStorageInfo(const mavlink_storage_information_t& st)
{
    qCDebug(CameraControlLog) << "handleStorageInfo:" << st.available_capacity << st.status << st.storage_count << st.storage_id << st.total_capacity << st.used_capacity;
    if(st.status == STORAGE_STATUS_READY) {
        uint32_t t = static_cast<uint32_t>(st.total_capacity);
        if(_storageTotal != t) {
            _storageTotal = t;
            emit storageTotalChanged();
        }
        uint32_t a = static_cast<uint32_t>(st.available_capacity);
        if(_storageFree != a) {
            _storageFree = a;
            emit storageFreeChanged();
        }
    }
    if(_storageStatus != static_cast<StorageStatus>(st.status)) {
        _storageStatus = static_cast<StorageStatus>(st.status);
        emit storageStatusChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleBatteryStatus(const mavlink_battery_status_t& bs)
{
    qCDebug(CameraControlLog) << "handleBatteryStatus:" << bs.battery_remaining;
    if(bs.battery_remaining >= 0 && _batteryRemaining != static_cast<int>(bs.battery_remaining)) {
        _batteryRemaining = static_cast<int>(bs.battery_remaining);
        emit batteryRemainingChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleCaptureStatus(const mavlink_camera_capture_status_t& cap)
{
    //-- This is a response to MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS
    qCDebug(CameraControlLog) << "handleCaptureStatus:" << cap.available_capacity << cap.image_interval << cap.image_status << cap.recording_time_ms << cap.video_status;
    //-- Disk Free Space
    uint32_t a = static_cast<uint32_t>(cap.available_capacity);
    if(_storageFree != a) {
        _storageFree = a;
        emit storageFreeChanged();
    }
    //-- Do we have recording time?
    if(cap.recording_time_ms) {
        // Resync our _recTime timer to the time info received from the camera component
        _recordTime = cap.recording_time_ms;
        _recTime = _recTime.addMSecs(_recTime.msecsTo(QTime::currentTime()) - static_cast<int>(cap.recording_time_ms));
        emit recordTimeChanged();
    }
    //-- Video/Image Capture Status
    uint8_t vs = cap.video_status < static_cast<uint8_t>(VIDEO_CAPTURE_STATUS_LAST) ? cap.video_status : static_cast<uint8_t>(VIDEO_CAPTURE_STATUS_UNDEFINED);
    uint8_t ps = cap.image_status < static_cast<uint8_t>(PHOTO_CAPTURE_LAST) ? cap.image_status : static_cast<uint8_t>(PHOTO_CAPTURE_STATUS_UNDEFINED);
    _setVideoStatus(static_cast<VideoStatus>(vs));
    _setPhotoStatus(static_cast<PhotoStatus>(ps));
    //-- Keep asking for it once in a while when recording
    if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _captureStatusTimer.start(5000);
    //-- Same while (single) image capture is busy
    } else if(photoStatus() != PHOTO_CAPTURE_IDLE && photoMode() == PHOTO_CAPTURE_SINGLE) {
        _captureStatusTimer.start(1000);
    }
    //-- Time Lapse
    if(photoStatus() == PHOTO_CAPTURE_INTERVAL_IDLE || photoStatus() == PHOTO_CAPTURE_INTERVAL_IN_PROGRESS) {
        //-- Capture local image as well
        if(qgcApp()->toolbox()->videoManager()) {
            QString photoPath = qgcApp()->toolbox()->settingsManager()->appSettings()->savePath()->rawValue().toString() + QStringLiteral("/Photo");
            QDir().mkpath(photoPath);
            photoPath += + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz") + ".jpg";
            qgcApp()->toolbox()->videoManager()->grabImage(photoPath);
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleVideoInfo(const mavlink_video_stream_information_t* vi)
{
    qCDebug(CameraControlLog) << "handleVideoInfo:" << vi->stream_id << vi->uri;
    _expectedCount = vi->count;
    if(!_findStream(vi->stream_id, false)) {
        qCDebug(CameraControlLog) << "Create stream handler for stream ID:" << vi->stream_id;
        QGCVideoStreamInfo* pStream = new QGCVideoStreamInfo(this, vi);
        QQmlEngine::setObjectOwnership(pStream, QQmlEngine::CppOwnership);
        _streams.append(pStream);
        //-- Thermal is handled separately and not listed
        if(!pStream->isThermal()) {
            _streamLabels.append(pStream->name());
            emit streamsChanged();
            emit streamLabelsChanged();
        } else {
            emit thermalStreamChanged();
        }
    }
    //-- Check for missing count
    if(_streams.count() < _expectedCount) {
        _streamInfoTimer.start(1000);
    } else {
        //-- Done
        qCDebug(CameraControlLog) << "All stream handlers done";
        _streamInfoTimer.stop();
        emit autoStreamChanged();
        emit _vehicle->cameraManager()->streamChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleVideoStatus(const mavlink_video_stream_status_t* vs)
{
    _streamStatusTimer.stop();
    qCDebug(CameraControlLog) << "handleVideoStatus:" << vs->stream_id;
    QGCVideoStreamInfo* pInfo = _findStream(vs->stream_id);
    if(pInfo) {
        pInfo->update(vs);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setCurrentStream(int stream)
{
    if(stream != _currentStream && stream >= 0 && stream < _streamLabels.count()) {
        if(_currentStream != stream) {
            QGCVideoStreamInfo* pInfo = currentStreamInstance();
            if(pInfo) {
                qCDebug(CameraControlLog) << "Stopping stream:" << pInfo->uri();
                //-- Stop current stream
                _vehicle->sendMavCommand(
                    _compID,                                // Target component
                    MAV_CMD_VIDEO_STOP_STREAMING,           // Command id
                    false,                                  // ShowError
                    pInfo->streamID());                     // Stream ID
            }
            _currentStream = stream;
            pInfo = currentStreamInstance();
            if(pInfo) {
                //-- Start new stream
                qCDebug(CameraControlLog) << "Starting stream:" << pInfo->uri();
                _vehicle->sendMavCommand(
                    _compID,                                // Target component
                    MAV_CMD_VIDEO_START_STREAMING,          // Command id
                    false,                                  // ShowError
                    pInfo->streamID());                     // Stream ID
                //-- Update stream status
                _requestStreamStatus(static_cast<uint8_t>(pInfo->streamID()));
            }
            emit currentStreamChanged();
            emit _vehicle->cameraManager()->streamChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::stopStream()
{
    QGCVideoStreamInfo* pInfo = currentStreamInstance();
    if(pInfo) {
        //-- Stop current stream
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_VIDEO_STOP_STREAMING,           // Command id
            false,                                  // ShowError
            pInfo->streamID());                     // Stream ID
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::resumeStream()
{
    QGCVideoStreamInfo* pInfo = currentStreamInstance();
    if(pInfo) {
        //-- Start new stream
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_VIDEO_START_STREAMING,          // Command id
            false,                                  // ShowError
            pInfo->streamID());                     // Stream ID
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::autoStream()
{
    if(hasVideoStream()) {
        return _streams.count() > 0;
    }
    return false;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::currentStreamInstance()
{
    if(_currentStream < _streamLabels.count() && _streamLabels.count()) {
        QGCVideoStreamInfo* pStream = _findStream(_streamLabels[_currentStream]);
        return pStream;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::thermalStreamInstance()
{
    //-- For now, it will return the first thermal listed (if any)
    for(int i = 0; i < _streams.count(); i++) {
        if(_streams[i]) {
            QGCVideoStreamInfo* pStream = qobject_cast<QGCVideoStreamInfo*>(_streams[i]);
            if(pStream) {
                if(pStream->isThermal()) {
                    return pStream;
                }
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestStreamInfo(uint8_t streamID)
{
    qCDebug(CameraControlLog) << "Requesting video stream info for:" << streamID;
    _vehicle->sendMavCommand(
        _compID,                                            // Target component
        MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION,           // Command id
        false,                                              // ShowError
        streamID);                                          // Stream ID
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestStreamStatus(uint8_t streamID)
{
    qCDebug(CameraControlLog) << "Requesting video stream status for:" << streamID;
    _vehicle->sendMavCommand(
        _compID,                                            // Target component
        MAV_CMD_REQUEST_VIDEO_STREAM_STATUS,                // Command id
        false,                                              // ShowError
        streamID);                                          // Stream ID
    _streamStatusTimer.start(1000);                         // Wait up to a second for it
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::_findStream(uint8_t id, bool report)
{
    for(int i = 0; i < _streams.count(); i++) {
        if(_streams[i]) {
            QGCVideoStreamInfo* pStream = qobject_cast<QGCVideoStreamInfo*>(_streams[i]);
            if(pStream) {
                if(pStream->streamID() == id) {
                    return pStream;
                }
            } else {
                qCritical() << "Null QGCVideoStreamInfo instance";
            }
        }
    }
    if(report) {
        qWarning() << "Stream id not found:" << id;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::_findStream(const QString name)
{
    for(int i = 0; i < _streams.count(); i++) {
        if(_streams[i]) {
            QGCVideoStreamInfo* pStream = qobject_cast<QGCVideoStreamInfo*>(_streams[i]);
            if(pStream) {
                if(pStream->name() == name) {
                    return pStream;
                }
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_streamTimeout()
{
    _requestCount++;
    int count = _expectedCount * 3;
    if(_requestCount > count) {
        qCWarning(CameraControlLog) << "Giving up requesting video stream info";
        _streamInfoTimer.stop();
        //-- If we have at least one stream, work with what we have.
        if(_streams.count()) {
            emit autoStreamChanged();
            emit _vehicle->cameraManager()->streamChanged();
        }
        return;
    }
    for(uint8_t i = 0; i < _expectedCount; i++) {
        //-- Stream ID starts at 1
        if(!_findStream(i+1, false)) {
            _requestStreamInfo(i+1);
            return;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_streamStatusTimeout()
{
    QGCVideoStreamInfo* pStream = currentStreamInstance();
    if(pStream) {
        _requestStreamStatus(static_cast<uint8_t>(pStream->streamID()));
    }
}



//-----------------------------------------------------------------------------
void
QGCCameraControl::_processConstants()
{
    qCDebug(CameraControlLog) << "Updating contants with input from definition file";
    _vendor = _definitionFileHandler->_vendor;
    _modelName = _definitionFileHandler->_modelName;
    _version = _definitionFileHandler->_version;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_processFacts()
{
    qCDebug(CameraControlLog) << "Updating lists and maps with parsed data";
    _valueExclusions.append(_definitionFileHandler->_valueExclusions);
    _optionRanges.append(_definitionFileHandler->_optionRanges);
    _settings.append(_definitionFileHandler->_settings);
    _requestUpdates = QMap<QString, QStringList>(_definitionFileHandler->_requestUpdates);
    _originalOptNames = QMap<QString, QStringList>(_definitionFileHandler->_originalOptNames);
    _originalOptValues = QMap<QString, QVariantList>(_definitionFileHandler->_originalOptValues);

     qCDebug(CameraControlLog) << "Updating contants with input from definition file";
     for (int i=0; i < _definitionFileHandler->_parsedFacts.size(); i++) {
         ParsedFact parsedfact = _definitionFileHandler->_parsedFacts[i];

         if (!_nameToFactMetaDataMap.contains(parsedfact.name)) {
             parsedfact.metaData->setParent(this);
             QQmlEngine::setObjectOwnership(parsedfact.metaData, QQmlEngine::CppOwnership);

             Fact* pFact = new Fact(_compID, parsedfact.name, parsedfact.type, this);
             QQmlEngine::setObjectOwnership(pFact, QQmlEngine::CppOwnership);
             pFact->setMetaData(parsedfact.metaData);
             pFact->_containerSetRawValue(parsedfact.metaData->rawDefaultValue());

             QGCCameraParamIO* pIO = new QGCCameraParamIO(this, pFact, _vehicle);
             QQmlEngine::setObjectOwnership(pIO, QQmlEngine::CppOwnership);
             _paramIO[parsedfact.name] = pIO;

             _nameToFactMetaDataMap[parsedfact.name] = parsedfact.metaData;
             _addFact(pFact, parsedfact.name);
         } else {
             qWarning() << QStringLiteral("Duplicate fact name:") << parsedfact.name;
             delete parsedfact.metaData; // Cleanup
         }
     }

     if(_nameToFactMetaDataMap.size() > 0) {
         _addFactGroup(this, "camera");
         _processRanges();
         _activeSettings = _settings;
         emit activeSettingsChanged();
     }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_processRanges()
{
    //-- After all parameter are loaded, process parameter ranges
    for(QGCCameraOptionRange* pRange: _optionRanges) {
        Fact* pRFact = getFact(pRange->targetParam);
        if(pRFact) {
            for(int i = 0; i < pRange->optNames.size(); i++) {
                QVariant optVariant;
                QString  errorString;
                if (!pRFact->metaData()->convertAndValidateRaw(pRange->optValues[i], false, optVariant, errorString)) {
                    qWarning() << "Invalid roption value, name:" << pRange->targetParam
                               << " type:"  << pRFact->metaData()->type()
                               << " value:" << pRange->optValues[i]
                               << " error:" << errorString;
                } else {
                    pRange->optVariants << optVariant;
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
void
QGCCameraControl::_handleDefinitionFile(const QString &url)
{
    //-- First check and see if we have it cached
    QFile xmlFile(_cacheFile);
    if (!xmlFile.exists()) {
        qCDebug(CameraControlLog) << "No camera definition file cached";
        _downloader->download(url);
        return;
    }
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not read cached camera definition file:" << _cacheFile;
        _downloader->download(url);
        return;
    }
    QByteArray bytes = xmlFile.readAll();
    QDomDocument doc;
    if(!doc.setContent(bytes, false)) {
        qWarning() << "Could not parse cached camera definition file:" << _cacheFile;
        _downloader->download(url);
        return;
    }
    //-- We have it
    qCDebug(CameraControlLog) << "Using cached camera definition file:" << _cacheFile;
    _cached = true;
    emit dataReady(bytes);
}


//-----------------------------------------------------------------------------
void
QGCCameraControl::_dataReady(QByteArray data)
{
    if(data.size()) {
        qCDebug(CameraControlLog) << "Parsing camera definition";
        _definitionFileHandler->parse(data);

        //-- If this is new, cache it
        if(!_cached) {
            qCDebug(CameraControlLog) << "Saving camera definition file" << _cacheFile;
            QFile file(_cacheFile);
            if (!file.open(QIODevice::WriteOnly)) {
                qWarning() << QString("Could not save cache file %1. Error: %2").arg(_cacheFile).arg(file.errorString());
            } else {
                file.write(data);
            }
        }

    } else {
        qCDebug(CameraControlLog) << "No camera definition";
    }
    _initWhenReady();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_paramDone()
{
    for(const QString& param: _paramIO.keys()) {
        if(!_paramIO[param]->paramDone()) {
            return;
        }
    }
    //-- All parameters loaded (or timed out)
    _paramComplete = true;
    emit parametersReady();
    //-- Check for video streaming
    _checkForVideoStreams();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_checkForVideoStreams()
{
    if(_info.flags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM) {
        //-- Skip it if using Taisync as it has its own video settings
        if(!qgcApp()->toolbox()->videoManager()->isTaisync()) {
            connect(&_streamInfoTimer, &QTimer::timeout, this, &QGCCameraControl::_streamTimeout);
            _streamInfoTimer.setSingleShot(false);
            connect(&_streamStatusTimer, &QTimer::timeout, this, &QGCCameraControl::_streamStatusTimeout);
            _streamStatusTimer.setSingleShot(true);
            //-- Request all streams
            _requestStreamInfo(0);
            _streamInfoTimer.start(2000);
        }
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::incomingParameter(Fact* pFact, QVariant& newValue)
{
    Q_UNUSED(pFact);
    Q_UNUSED(newValue);
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::validateParameter(Fact* pFact, QVariant& newValue)
{
    Q_UNUSED(pFact);
    Q_UNUSED(newValue);
    return true;
}

//-----------------------------------------------------------------------------
QStringList
QGCCameraControl::activeSettings()
{
    qCDebug(CameraControlLog) << "Active:" << _activeSettings;
    return _activeSettings;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::exposureMode()
{
    return (_paramComplete && _activeSettings.contains(kCAM_EXPMODE)) ? getFact(kCAM_EXPMODE) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::ev()
{
    return (_paramComplete && _activeSettings.contains(kCAM_EV)) ? getFact(kCAM_EV) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::iso()
{
    return (_paramComplete && _activeSettings.contains(kCAM_ISO)) ? getFact(kCAM_ISO) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::shutterSpeed()
{
    return (_paramComplete && _activeSettings.contains(kCAM_SHUTTERSPD)) ? getFact(kCAM_SHUTTERSPD) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::aperture()
{
    return (_paramComplete && _activeSettings.contains(kCAM_APERTURE)) ? getFact(kCAM_APERTURE) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::wb()
{
    return (_paramComplete && _activeSettings.contains(kCAM_WBMODE)) ? getFact(kCAM_WBMODE) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::mode()
{
    return _paramComplete && factExists(kCAM_MODE) ? getFact(kCAM_MODE) : nullptr;
}

