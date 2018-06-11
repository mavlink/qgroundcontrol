/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCApplication.h"
#include "QGCCameraManager.h"

QGC_LOGGING_CATEGORY(CameraManagerLog, "CameraManagerLog")

//-----------------------------------------------------------------------------
QGCCameraManager::QGCCameraManager(Vehicle *vehicle)
    : _vehicle(vehicle)
    , _vehicleReadyState(false)
    , _currentTask(0)
    , _currentCamera(0)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qCDebug(CameraManagerLog) << "QGCCameraManager Created";
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &QGCCameraManager::_vehicleReady);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &QGCCameraManager::_mavlinkMessageReceived);
}

//-----------------------------------------------------------------------------
QGCCameraManager::~QGCCameraManager()
{
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::setCurrentCamera(int sel)
{
    if(sel != _currentCamera && sel >= 0 && sel < _cameras.count()) {
        _currentCamera = sel;
        emit currentCameraChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_vehicleReady(bool ready)
{
    qCDebug(CameraManagerLog) << "_vehicleReady(" << ready << ")";
    if(ready) {
        if(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle() == _vehicle) {
            _vehicleReadyState = true;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if(message.sysid == _vehicle->id()) {
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
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleHeartbeat(const mavlink_message_t &message)
{
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&message, &heartbeat);
    //-- If this heartbeat is from a different node within the vehicle
    if(_vehicleReadyState && _vehicle->id() == message.sysid && _vehicle->defaultComponentId() != message.compid) {
        if(!_cameraInfoRequested.contains(message.compid)) {
            _cameraInfoRequested[message.compid] = true;
            //-- Request camera info
            _requestCameraInfo(message.compid);
        }
    }
}

//-----------------------------------------------------------------------------
QGCCameraControl*
QGCCameraManager::_findCamera(int id)
{
    for(int i = 0; i < _cameras.count(); i++) {
        if(_cameras[i]) {
            QGCCameraControl* pCamera = qobject_cast<QGCCameraControl*>(_cameras[i]);
            if(pCamera) {
                if(pCamera->compID() == id) {
                    return pCamera;
                }
            } else {
                qCritical() << "Null QGCCameraControl instance";
            }
        }
    }
    qWarning() << "Camera component id not found:" << id;
    return NULL;
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleCameraInfo(const mavlink_message_t& message)
{
    //-- Have we requested it?
    if(_cameraInfoRequested.contains(message.compid) && _cameraInfoRequested[message.compid]) {
        //-- Flag it as done
        _cameraInfoRequested[message.compid] = false;
        mavlink_camera_information_t info;
        mavlink_msg_camera_information_decode(&message, &info);
        qCDebug(CameraManagerLog) << "_handleCameraInfo:" << (const char*)(void*)&info.model_name[0] << (const char*)(void*)&info.vendor_name[0] << "Comp ID:" << message.compid;
        QGCCameraControl* pCamera = _vehicle->firmwarePlugin()->createCameraControl(&info, _vehicle, message.compid, this);
        if(pCamera) {
            QQmlEngine::setObjectOwnership(pCamera, QQmlEngine::CppOwnership);
            _cameras.append(pCamera);
            _cameraLabels << pCamera->modelName();
            emit camerasChanged();
            emit cameraLabelsChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleCaptureStatus(const mavlink_message_t &message)
{
    QGCCameraControl* pCamera = _findCamera(message.compid);
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
    QGCCameraControl* pCamera = _findCamera(message.compid);
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
    QGCCameraControl* pCamera = _findCamera(message.compid);
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
    QGCCameraControl* pCamera = _findCamera(message.compid);
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
    QGCCameraControl* pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_param_ext_value_t value;
        mavlink_msg_param_ext_value_decode(&message, &value);
        pCamera->handleParamValue(value);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_requestCameraInfo(int compID)
{
    qCDebug(CameraManagerLog) << "_requestCameraInfo(" << compID << ")";
    if(_vehicle) {
        _vehicle->sendMavCommand(
            compID,                                 // target component
            MAV_CMD_REQUEST_CAMERA_INFORMATION,     // command id
            false,                                  // showError
            1);                                     // Do Request
    }
}
