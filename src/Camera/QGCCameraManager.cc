/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCameraManager.h"
#include "JoystickManager.h"
#include "SimulatedCameraControl.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "CameraMetaData.h"
#include "JsonHelper.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(CameraManagerLog, "qgc.camera.qgccameramanager")

QVariantList QGCCameraManager::_cameraList;

//-----------------------------------------------------------------------------
QGCCameraManager::CameraStruct::CameraStruct(QObject* parent, uint8_t compID_, Vehicle* vehicle_)
    : QObject   (parent)
    , compID    (compID_)
    , vehicle   (vehicle_)
{
}

//-----------------------------------------------------------------------------
QGCCameraManager::QGCCameraManager(Vehicle *vehicle)
    : _vehicle                  (vehicle)
    , _simulatedCameraControl   (new SimulatedCameraControl(vehicle, this))
{
    qCDebug(CameraManagerLog) << "QGCCameraManager Created";

    (void) qRegisterMetaType<CameraMetaData>("CameraMetaData");

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    _addCameraControlToLists(_simulatedCameraControl);

    connect(MultiVehicleManager::instance(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &QGCCameraManager::_vehicleReady);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &QGCCameraManager::_mavlinkMessageReceived);
    connect(&_camerasLostHeartbeatTimer, &QTimer::timeout, this, &QGCCameraManager::_checkForLostCameras);

    _camerasLostHeartbeatTimer.setSingleShot(false);
    _lastZoomChange.start();
    _lastCameraChange.start();
    _camerasLostHeartbeatTimer.start(500);
}

QGCCameraManager::~QGCCameraManager()
{
    for (QVariant cam : _cameraList) {
        delete cam.value<CameraMetaData*>();
    }
}

void QGCCameraManager::registerQmlTypes()
{
    qmlRegisterUncreatableType<MavlinkCameraControl>("QGroundControl.Vehicle", 1, 0, "MavlinkCameraControl", "Reference only");
    qmlRegisterUncreatableType<QGCCameraManager>    ("QGroundControl.Vehicle", 1, 0, "QGCCameraManager",     "Reference only");
    qmlRegisterUncreatableType<QGCVideoStreamInfo>  ("QGroundControl.Vehicle", 1, 0, "QGCVideoStreamInfo",   "Reference only");
}

void QGCCameraManager::setCurrentCamera(int sel)
{
    if(sel != _currentCameraIndex && sel >= 0 && sel < _cameras.count()) {
        _currentCameraIndex = sel;
        emit currentCameraChanged();
        emit streamChanged();
    }
}

void QGCCameraManager::_vehicleReady(bool ready)
{
    qCDebug(CameraManagerLog) << "_vehicleReady(" << ready << ")";
    if(ready) {
        if(MultiVehicleManager::instance()->activeVehicle() == _vehicle) {
            _vehicleReadyState = true;
            _activeJoystickChanged(JoystickManager::instance()->activeJoystick());
            connect(JoystickManager::instance(), &JoystickManager::activeJoystickChanged, this, &QGCCameraManager::_activeJoystickChanged);
        }
    }
}

void QGCCameraManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    //-- Only pay attention to camera components, as identified by their compId
    if(message.sysid == _vehicle->id() && (message.compid == MAV_COMP_ID_AUTOPILOT1 ||
        (message.compid >= MAV_COMP_ID_CAMERA && message.compid <= MAV_COMP_ID_CAMERA6))) {
        switch (message.msgid) {
            case MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS:
                _handleCaptureStatus(message);
                break;
            case MAVLINK_MSG_ID_STORAGE_INFORMATION:
                _handleStorageInfo(message);
                break;
            case MAVLINK_MSG_ID_HEARTBEAT:
                _handleHeartbeat(message);
                break;
            case MAVLINK_MSG_ID_CAMERA_INFORMATION:
                _handleCameraInfo(message);
                break;
            case MAVLINK_MSG_ID_CAMERA_SETTINGS:
                _handleCameraSettings(message);
                break;
            case MAVLINK_MSG_ID_PARAM_EXT_ACK:
                _handleParamAck(message);
                break;
            case MAVLINK_MSG_ID_PARAM_EXT_VALUE:
                _handleParamValue(message);
                break;
            case MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION:
                _handleVideoStreamInfo(message);
                break;
            case MAVLINK_MSG_ID_VIDEO_STREAM_STATUS:
                _handleVideoStreamStatus(message);
                break;
            case MAVLINK_MSG_ID_BATTERY_STATUS:
                _handleBatteryStatus(message);
                break;
            case MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS:
                _handleTrackingImageStatus(message);
                break;
        }
    }
}

void QGCCameraManager::_handleHeartbeat(const mavlink_message_t &message)
{
    QString sCompID = QString::number(message.compid);

    if (!_cameraInfoRequest.contains(sCompID)) {
        // This is the first time we are heading from this camera
        qCDebug(CameraManagerLog) << "Hearbeat from " << message.compid;
        CameraStruct* pInfo = new CameraStruct(this, message.compid, _vehicle);
        pInfo->lastHeartbeat.start();
        _cameraInfoRequest[sCompID] = pInfo;
        _requestCameraInfo(pInfo);
    } else {
        if (_cameraInfoRequest[sCompID]) {
            CameraStruct* pInfo = _cameraInfoRequest[sCompID];
            //-- Check if we have indeed received the camera info
            if (pInfo->infoReceived) {
                //-- We have it. Just update the heartbeat timeout
                pInfo->lastHeartbeat.start();
            }
        } else {
            qWarning() << Q_FUNC_INFO << "_cameraInfoRequest[" << sCompID << "] is null";
        }
    }
}

MavlinkCameraControl* QGCCameraManager::currentCameraInstance()
{
    if(_currentCameraIndex < _cameras.count() && _cameras.count()) {
        auto pCamera = qobject_cast<MavlinkCameraControl*>(_cameras[_currentCameraIndex]);
        return pCamera;
    }
    return nullptr;
}

QGCVideoStreamInfo* QGCCameraManager::currentStreamInstance()
{
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        QGCVideoStreamInfo* pInfo = pCamera->currentStreamInstance();
        return pInfo;
    }
    return nullptr;
}

QGCVideoStreamInfo* QGCCameraManager::thermalStreamInstance()
{
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        QGCVideoStreamInfo* pInfo = pCamera->thermalStreamInstance();
        return pInfo;
    }
    return nullptr;
}

MavlinkCameraControl* QGCCameraManager::_findCamera(int id)
{
    for(int i = 0; i < _cameras.count(); i++) {
        if(_cameras[i]) {
            auto pCamera = qobject_cast<MavlinkCameraControl*>(_cameras[i]);
            if(pCamera) {
                if(pCamera->compID() == id) {
                    return pCamera;
                }
            } else {
                qCritical() << "Null MavlinkCameraControl instance";
            }
        }
    }
    //qWarning() << "Camera component id not found:" << id;
    return nullptr;
}

void QGCCameraManager::_addCameraControlToLists(MavlinkCameraControl* cameraControl)
{
    QQmlEngine::setObjectOwnership(cameraControl, QQmlEngine::CppOwnership);
    _cameras.append(cameraControl);
    _cameraLabels.append(cameraControl->modelName());
    emit camerasChanged();
    emit cameraLabelsChanged();

    // If the simulated camera is already in the list, remove it since we have a real camera now
    if (_cameras.count() == 2 && _cameras[0] == _simulatedCameraControl) {
        _cameras.removeAt(0);
        _cameraLabels.removeAt(0);
        emit camerasChanged();
        emit cameraLabelsChanged();
        emit currentCameraChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleCameraInfo(const mavlink_message_t& message)
{
    qCDebug(CameraManagerLog) << "_handleCameraInfo";
    //-- Have we requested it?
    QString sCompID = QString::number(message.compid);
    if(_cameraInfoRequest.contains(sCompID) && !_cameraInfoRequest[sCompID]->infoReceived) {
        //-- Flag it as done
        _cameraInfoRequest[sCompID]->infoReceived = true;
        mavlink_camera_information_t info;
        mavlink_msg_camera_information_decode(&message, &info);
        qCDebug(CameraManagerLog) << "_handleCameraInfo:" << reinterpret_cast<const char*>(info.model_name) << reinterpret_cast<const char*>(info.vendor_name) << "Comp ID:" << message.compid;
        auto pCamera = _vehicle->firmwarePlugin()->createCameraControl(&info, _vehicle, message.compid, this);
        if(pCamera) {
            _addCameraControlToLists(pCamera);
        }
    }
}

/// Called to check for cameras which are no longer sending a heartbeat
void QGCCameraManager::_checkForLostCameras()
{
    //-- Iterate cameras
    foreach (QString sCompID, _cameraInfoRequest.keys()) {
        if (_cameraInfoRequest[sCompID]) {
            CameraStruct* pInfo = _cameraInfoRequest[sCompID];
            //-- Have we received a camera info message?
            if (pInfo->infoReceived) {
                //-- Has the camera stopped talking to us?
                if (pInfo->lastHeartbeat.elapsed() > 5000) {
                    auto pCamera = _findCamera(pInfo->compID);

                    if (pCamera) {
                        // Before removing the current camera from the list add the simulated camera back into the list if thera are no other cameras.
                        // This way we smaoothly transition from a real camera to the simulated camera.
                        if (_cameras.count() == 1) {
                            qCDebug(CameraManagerLog) << "Adding simulated camera back to list.";
                            _addCameraControlToLists(_simulatedCameraControl);
                        }

                        qWarning() << "Camera" << pCamera->modelName() << "stopped transmitting. Removing from list.";
                        _cameraLabels.removeOne(pCamera->modelName());
                        _cameras.removeOne(pCamera);
                        emit cameraLabelsChanged();
                        emit camerasChanged();

                        pCamera->deleteLater();
                        delete pInfo;

                        // There will always be at least one camera in the list, so we don't need to check if the list is empty.
                        // We specifically don't use setCurrentCamera since that checks for a index change. But in this case we may be using the same index.
                        _currentCameraIndex = 0;
                        emit currentCameraChanged();
                        emit streamChanged();
                    }

                    _cameraInfoRequest.remove(sCompID);

                    //-- Exit loop.
                    return;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleCaptureStatus(const mavlink_message_t &message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_camera_capture_status_t cap;
        mavlink_msg_camera_capture_status_decode(&message, &cap);
        pCamera->handleCaptureStatus(cap);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleStorageInfo(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_storage_information_t st;
        mavlink_msg_storage_information_decode(&message, &st);
        pCamera->handleStorageInfo(st);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleCameraSettings(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_camera_settings_t settings;
        mavlink_msg_camera_settings_decode(&message, &settings);
        pCamera->handleSettings(settings);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleParamAck(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_param_ext_ack_t ack;
        mavlink_msg_param_ext_ack_decode(&message, &ack);
        pCamera->handleParamAck(ack);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleParamValue(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_param_ext_value_t value;
        mavlink_msg_param_ext_value_decode(&message, &value);
        pCamera->handleParamValue(value);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleVideoStreamInfo(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_video_stream_information_t streamInfo;
        mavlink_msg_video_stream_information_decode(&message, &streamInfo);
        pCamera->handleVideoInfo(&streamInfo);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleVideoStreamStatus(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_video_stream_status_t streamStatus;
        mavlink_msg_video_stream_status_decode(&message, &streamStatus);
        pCamera->handleVideoStatus(&streamStatus);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleBatteryStatus(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_battery_status_t batteryStatus;
        mavlink_msg_battery_status_decode(&message, &batteryStatus);
        pCamera->handleBatteryStatus(batteryStatus);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleTrackingImageStatus(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_camera_tracking_image_status_t tis;
        mavlink_msg_camera_tracking_image_status_decode(&message, &tis);
        pCamera->handleTrackingImageStatus(&tis);
    }
}

static void _requestCameraInfoCommandResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    auto  cameraInfo = static_cast<QGCCameraManager::CameraStruct*>(resultHandlerData);

    if (ack.result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "MAV_CMD_REQUEST_CAMERA_INFORMATION failed. compId" << cameraInfo->compID << "Result:" << ack.result << "FailureCode:" << failureCode;
    }
}

static void _requestCameraInfoMessageResultHandler(void* resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, [[maybe_unused]] const mavlink_message_t& message)
{
    auto cameraInfo = static_cast<QGCCameraManager::CameraStruct*>(resultHandlerData);

    if (result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "MAV_CMD_REQUEST_MESSAGE:MAVLINK_MSG_ID_CAMERA_INFORMATION failed. Falling back to MAV_CMD_REQUEST_CAMERA_INFORMATION. compId" << cameraInfo->compID << "Result:" << result << "FailureCode:" << failureCode;

        Vehicle::MavCmdAckHandlerInfo_t ackHandlerInfo;
        ackHandlerInfo.resultHandler        = _requestCameraInfoCommandResultHandler;
        ackHandlerInfo.resultHandlerData    = cameraInfo;
        ackHandlerInfo.progressHandler      = nullptr;
        ackHandlerInfo.progressHandlerData  = nullptr;

        cameraInfo->vehicle->sendMavCommandWithHandler(&ackHandlerInfo, cameraInfo->compID, MAV_CMD_REQUEST_CAMERA_INFORMATION, 1 /* request camera capabilities */);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_requestCameraInfo(CameraStruct* pInfo)
{
    qCDebug(CameraManagerLog) << Q_FUNC_INFO << pInfo->compID;

    // We first try using the newish MAV_CMD_REQUEST_MESSAGE mechanism
    _vehicle->requestMessage(_requestCameraInfoMessageResultHandler, pInfo, pInfo->compID, MAVLINK_MSG_ID_CAMERA_INFORMATION);
}

//----------------------------------------------------------------------------------------
void
QGCCameraManager::_activeJoystickChanged(Joystick* joystick)
{
    qCDebug(CameraManagerLog) << "Joystick changed";
    if(_activeJoystick) {
        disconnect(_activeJoystick, &Joystick::stepZoom,            this, &QGCCameraManager::_stepZoom);
        disconnect(_activeJoystick, &Joystick::startContinuousZoom, this, &QGCCameraManager::_startZoom);
        disconnect(_activeJoystick, &Joystick::stopContinuousZoom,  this, &QGCCameraManager::_stopZoom);
        disconnect(_activeJoystick, &Joystick::stepCamera,          this, &QGCCameraManager::_stepCamera);
        disconnect(_activeJoystick, &Joystick::stepStream,          this, &QGCCameraManager::_stepStream);
        disconnect(_activeJoystick, &Joystick::triggerCamera,       this, &QGCCameraManager::_triggerCamera);
        disconnect(_activeJoystick, &Joystick::startVideoRecord,    this, &QGCCameraManager::_startVideoRecording);
        disconnect(_activeJoystick, &Joystick::stopVideoRecord,     this, &QGCCameraManager::_stopVideoRecording);
        disconnect(_activeJoystick, &Joystick::toggleVideoRecord,   this, &QGCCameraManager::_toggleVideoRecording);
    }
    _activeJoystick = joystick;
    if(_activeJoystick) {
        connect(_activeJoystick, &Joystick::stepZoom,               this, &QGCCameraManager::_stepZoom);
        connect(_activeJoystick, &Joystick::startContinuousZoom,    this, &QGCCameraManager::_startZoom);
        connect(_activeJoystick, &Joystick::stopContinuousZoom,     this, &QGCCameraManager::_stopZoom);
        connect(_activeJoystick, &Joystick::stepCamera,             this, &QGCCameraManager::_stepCamera);
        connect(_activeJoystick, &Joystick::stepStream,             this, &QGCCameraManager::_stepStream);
        connect(_activeJoystick, &Joystick::triggerCamera,          this, &QGCCameraManager::_triggerCamera);
        connect(_activeJoystick, &Joystick::startVideoRecord,       this, &QGCCameraManager::_startVideoRecording);
        connect(_activeJoystick, &Joystick::stopVideoRecord,        this, &QGCCameraManager::_stopVideoRecording);
        connect(_activeJoystick, &Joystick::toggleVideoRecord,      this, &QGCCameraManager::_toggleVideoRecording);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_triggerCamera()
{
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->takePhoto();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_startVideoRecording()
{
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->startVideoRecording();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stopVideoRecording()
{
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->stopVideoRecording();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_toggleVideoRecording()
{
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->toggleVideoRecording();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stepZoom(int direction)
{
    if(_lastZoomChange.elapsed() > 40) {
        _lastZoomChange.start();
        qCDebug(CameraManagerLog) << "Step Camera Zoom" << direction;
        auto pCamera = currentCameraInstance();
        if(pCamera) {
            pCamera->stepZoom(direction);
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_startZoom(int direction)
{
    qCDebug(CameraManagerLog) << "Start Camera Zoom" << direction;
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->startZoom(direction);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stopZoom()
{
    qCDebug(CameraManagerLog) << "Stop Camera Zoom";
    auto pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->stopZoom();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stepCamera(int direction)
{
    if(_lastCameraChange.elapsed() > 1000) {
        _lastCameraChange.start();
        qCDebug(CameraManagerLog) << "Step Camera" << direction;
        int c = _currentCameraIndex + direction;
        if(c < 0) c = _cameras.count() - 1;
        if(c >= _cameras.count()) c = 0;
        setCurrentCamera(c);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stepStream(int direction)
{
    if(_lastCameraChange.elapsed() > 1000) {
        _lastCameraChange.start();
        auto pCamera = currentCameraInstance();
        if(pCamera) {
            qCDebug(CameraManagerLog) << "Step Camera Stream" << direction;
            int c = pCamera->currentStream() + direction;
            if(c < 0) c = pCamera->streams()->count() - 1;
            if(c >= pCamera->streams()->count()) c = 0;
            pCamera->setCurrentStream(c);
        }
    }
}

const QVariantList &QGCCameraManager::cameraList()
{
    if (_cameraList.isEmpty()) {
        const QList<CameraMetaData*> cams = _parseCameraMetaData(QStringLiteral(":/json/CameraMetaData.json"));
        _cameraList.reserve(cams.size());

        for (CameraMetaData* cam : cams) {
            _cameraList << QVariant::fromValue(cam);
        }
    }

    return _cameraList;
}

QList<CameraMetaData*> QGCCameraManager::_parseCameraMetaData(const QString &jsonFilePath)
{
    QList<CameraMetaData*> cameraList;

    QString errorString;
    int version;
    const QJsonObject jsonObject = JsonHelper::openInternalQGCJsonFile(jsonFilePath, "CameraMetaData", 1, 1, version, errorString);
    if (!errorString.isEmpty()) {
        qCWarning(CameraManagerLog) << "Internal Error:" << errorString;
        return cameraList;
    }

    static const QList<JsonHelper::KeyValidateInfo> rootKeyInfoList = {
        { "cameraMetaData", QJsonValue::Array, true }
    };
    if (!JsonHelper::validateKeys(jsonObject, rootKeyInfoList, errorString)) {
        qCWarning(CameraManagerLog) << errorString;
        return cameraList;
    }

    static const QList<JsonHelper::KeyValidateInfo> cameraKeyInfoList = {
        { "canonicalName", QJsonValue::String, true },
        { "brand", QJsonValue::String, true },
        { "model", QJsonValue::String, true },
        { "sensorWidth", QJsonValue::Double, true },
        { "sensorHeight", QJsonValue::Double, true },
        { "imageWidth", QJsonValue::Double, true },
        { "imageHeight", QJsonValue::Double, true },
        { "focalLength", QJsonValue::Double, true },
        { "landscape", QJsonValue::Bool, true },
        { "fixedOrientation", QJsonValue::Bool, true },
        { "minTriggerInterval", QJsonValue::Double, true },
        { "deprecatedTranslatedName", QJsonValue::String, true },
    };
    const QJsonArray cameraInfo = jsonObject["cameraMetaData"].toArray();
    for (const QJsonValue &jsonValue : cameraInfo) {
        if (!jsonValue.isObject()) {
            qCWarning(CameraManagerLog) << "Entry in CameraMetaData array is not object";
            return cameraList;
        }

        const QJsonObject obj = jsonValue.toObject();
        if (!JsonHelper::validateKeys(obj, cameraKeyInfoList, errorString)) {
            qCWarning(CameraManagerLog) << errorString;
            return cameraList;
        }

        const QString canonicalName = obj["canonicalName"].toString();
        const QString brand = obj["brand"].toString();
        const QString model = obj["model"].toString();
        const double sensorWidth = obj["sensorWidth"].toDouble();
        const double sensorHeight = obj["sensorHeight"].toDouble();
        const double imageWidth = obj["imageWidth"].toDouble();
        const double imageHeight = obj["imageHeight"].toDouble();
        const double focalLength = obj["focalLength"].toDouble();
        const bool landscape = obj["landscape"].toBool();
        const bool fixedOrientation = obj["fixedOrientation"].toBool();
        const double minTriggerInterval = obj["minTriggerInterval"].toDouble();
        const QString deprecatedTranslatedName = obj["deprecatedTranslatedName"].toString();

        CameraMetaData *const metaData = new CameraMetaData(
            canonicalName, brand, model, sensorWidth, sensorHeight,
            imageWidth, imageHeight, focalLength, landscape,
            fixedOrientation, minTriggerInterval, deprecatedTranslatedName);
        cameraList.append(metaData);
    }

    return cameraList;
}
