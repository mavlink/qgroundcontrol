/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "JoystickManager.h"

QGC_LOGGING_CATEGORY(CameraManagerLog, "CameraManagerLog")

//-----------------------------------------------------------------------------
QGCCameraManager::CameraStruct::CameraStruct(QObject* parent, uint8_t compID_)
    : QObject(parent)
    , compID(compID_)
{
}

//-----------------------------------------------------------------------------
QGCCameraManager::QGCCameraManager(Vehicle *vehicle)
    : _vehicle(vehicle)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qCDebug(CameraManagerLog) << "QGCCameraManager Created";
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &QGCCameraManager::_vehicleReady);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &QGCCameraManager::_mavlinkMessageReceived);
    connect(&_cameraTimer, &QTimer::timeout, this, &QGCCameraManager::_cameraTimeout);
    _cameraTimer.setSingleShot(false);
    _lastZoomChange.start();
    _lastCameraChange.start();
    _cameraTimer.start(500);
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
        emit streamChanged();
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
            JoystickManager *pJoyMgr = qgcApp()->toolbox()->joystickManager();
            _activeJoystickChanged(pJoyMgr->activeJoystick());
            connect(pJoyMgr, &JoystickManager::activeJoystickChanged, this, &QGCCameraManager::_activeJoystickChanged);
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    //-- Only pay attention to camera components, as identified by their compId
    if(message.sysid == _vehicle->id() && (message.compid >= MAV_COMP_ID_CAMERA && message.compid <= MAV_COMP_ID_CAMERA6)) {
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
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleHeartbeat(const mavlink_message_t &message)
{
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&message, &heartbeat);
    //-- First time hearing from this one?
    QString sCompID = QString::number(message.compid);
    if(!_cameraInfoRequest.contains(sCompID)) {
        qCDebug(CameraManagerLog) << "Hearbeat from " << message.compid;
        CameraStruct* pInfo = new CameraStruct(this, message.compid);
        pInfo->lastHeartbeat.start();
        _cameraInfoRequest[sCompID] = pInfo;
        //-- Request camera info
        _requestCameraInfo(message.compid);
    } else {
        if(_cameraInfoRequest[sCompID]) {
            CameraStruct* pInfo = _cameraInfoRequest[sCompID];
            //-- Check if we have indeed received the camera info
            if(pInfo->infoReceived) {
                //-- We have it. Just update the heartbeat timeout
                pInfo->lastHeartbeat.start();
            } else {
                //-- Try again. Maybe.
                if(pInfo->lastHeartbeat.elapsed() > 2000) {
                    if(pInfo->tryCount > 3) {
                        if(!pInfo->gaveUp) {
                            pInfo->gaveUp = true;
                            qWarning() << "Giving up requesting camera info from" << _vehicle->id() << message.compid;
                        }
                    } else {
                        pInfo->tryCount++;
                        //-- Request camera info again.
                        _requestCameraInfo(message.compid);
                    }
                }
            }
        } else {
            qWarning() << "_cameraInfoRequest[" << sCompID << "] is null";
        }
    }
}

//-----------------------------------------------------------------------------
QGCCameraControl*
QGCCameraManager::currentCameraInstance()
{
    if(_currentCamera < _cameras.count() && _cameras.count()) {
        QGCCameraControl* pCamera = qobject_cast<QGCCameraControl*>(_cameras[_currentCamera]);
        return pCamera;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraManager::currentStreamInstance()
{
    QGCCameraControl* pCamera = currentCameraInstance();
    if(pCamera) {
        QGCVideoStreamInfo* pInfo = pCamera->currentStreamInstance();
        return pInfo;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraManager::thermalStreamInstance()
{
    QGCCameraControl* pCamera = currentCameraInstance();
    if(pCamera) {
        QGCVideoStreamInfo* pInfo = pCamera->thermalStreamInstance();
        return pInfo;
    }
    return nullptr;
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
    //qWarning() << "Camera component id not found:" << id;
    return nullptr;
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_handleCameraInfo(const mavlink_message_t& message)
{
    //-- Have we requested it?
    QString sCompID = QString::number(message.compid);
    if(_cameraInfoRequest.contains(sCompID) && !_cameraInfoRequest[sCompID]->infoReceived) {
        //-- Flag it as done
        _cameraInfoRequest[sCompID]->infoReceived = true;
        mavlink_camera_information_t info;
        mavlink_msg_camera_information_decode(&message, &info);
        qCDebug(CameraManagerLog) << "_handleCameraInfo:" << reinterpret_cast<const char*>(info.model_name) << reinterpret_cast<const char*>(info.vendor_name) << "Comp ID:" << message.compid;
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
QGCCameraManager::_cameraTimeout()
{
    //-- Iterate cameras
    foreach(QString sCompID, _cameraInfoRequest.keys()) {
        if(_cameraInfoRequest[sCompID]) {
            CameraStruct* pInfo = _cameraInfoRequest[sCompID];
            //-- Have we received a camera info message?
            if(pInfo->infoReceived) {
                //-- Has the camera stopped talking to us?
                if(pInfo->lastHeartbeat.elapsed() > 5000) {
                    //-- Camera is gone. Remove it.
                    bool autoStream = false;
                    QGCCameraControl* pCamera = _findCamera(pInfo->compID);
                    if(pCamera) {
                        qWarning() << "Camera" << pCamera->modelName() << "stopped transmitting. Removing from list.";
                        int idx = _cameraLabels.indexOf(pCamera->modelName());
                        if(idx >= 0) {
                            _cameraLabels.removeAt(idx);
                        }
                        idx = _cameras.indexOf(pCamera);
                        if(idx >= 0) {
                            _cameras.removeAt(idx);
                        }
                        autoStream = pCamera->autoStream();
                        pCamera->deleteLater();
                        delete pInfo;
                    }
                    _cameraInfoRequest.remove(sCompID);
                    emit cameraLabelsChanged();
                    //-- If we have another camera, switch current camera.
                    if(_cameras.count()) {
                        setCurrentCamera(0);
                    } else {
                        //-- We're out of cameras
                        emit camerasChanged();
                        if(autoStream) {
                            emit streamChanged();
                        }
                    }
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
QGCCameraManager::_handleVideoStreamInfo(const mavlink_message_t& message)
{
    QGCCameraControl* pCamera = _findCamera(message.compid);
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
    QGCCameraControl* pCamera = _findCamera(message.compid);
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
    QGCCameraControl* pCamera = _findCamera(message.compid);
    if(pCamera) {
        mavlink_battery_status_t batteryStatus;
        mavlink_msg_battery_status_decode(&message, &batteryStatus);
        pCamera->handleBatteryStatus(batteryStatus);
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
    QGCCameraControl* pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->takePhoto();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_startVideoRecording()
{
    QGCCameraControl* pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->startVideo();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stopVideoRecording()
{
    QGCCameraControl* pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->stopVideo();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_toggleVideoRecording()
{
    QGCCameraControl* pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->toggleVideo();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stepZoom(int direction)
{
    if(_lastZoomChange.elapsed() > 40) {
        _lastZoomChange.start();
        qCDebug(CameraManagerLog) << "Step Camera Zoom" << direction;
        QGCCameraControl* pCamera = currentCameraInstance();
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
    QGCCameraControl* pCamera = currentCameraInstance();
    if(pCamera) {
        pCamera->startZoom(direction);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraManager::_stopZoom()
{
    qCDebug(CameraManagerLog) << "Stop Camera Zoom";
    QGCCameraControl* pCamera = currentCameraInstance();
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
        int c = _currentCamera + direction;
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
        QGCCameraControl* pCamera = currentCameraInstance();
        if(pCamera) {
            qCDebug(CameraManagerLog) << "Step Camera Stream" << direction;
            int c = pCamera->currentStream() + direction;
            if(c < 0) c = pCamera->streams()->count() - 1;
            if(c >= pCamera->streams()->count()) c = 0;
            pCamera->setCurrentStream(c);
        }
    }
}


