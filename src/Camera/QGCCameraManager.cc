/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCameraManager.h"
#include "QGCApplication.h"
#include "JoystickManager.h"
#include "SimulatedCameraControl.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "CameraMetaData.h"

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(CameraManagerLog, "CameraManagerLog")

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

    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &QGCCameraManager::_vehicleReady);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &QGCCameraManager::_mavlinkMessageReceived);
    connect(&_camerasLostHeartbeatTimer, &QTimer::timeout, this, &QGCCameraManager::_checkForLostCameras);

    _camerasLostHeartbeatTimer.setSingleShot(false);
    _lastZoomChange.start();
    _lastCameraChange.start();
    _camerasLostHeartbeatTimer.start(500);
}

QGCCameraManager::~QGCCameraManager()
{
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
        if(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle() == _vehicle) {
            _vehicleReadyState = true;
            JoystickManager *pJoyMgr = qgcApp()->toolbox()->joystickManager();
            _activeJoystickChanged(pJoyMgr->activeJoystick());
            connect(pJoyMgr, &JoystickManager::activeJoystickChanged, this, &QGCCameraManager::_activeJoystickChanged);
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
        const CameraMetaData *metaData = nullptr;

        metaData = new CameraMetaData(
                    // Canon S100 @ 5.2mm f/2
                    "Canon S100 PowerShot",     // canonical name saved in plan file
                    tr("Canon"),                // brand
                    tr("S100 PowerShot"),       // model
                    7.6,                        // sensorWidth
                    5.7,                        // sensorHeight
                    4000,                       // imageWidth
                    3000,                       // imageHeight
                    5.2,                        // focalLength
                    true,                       // true: landscape orientation
                    false,                      // true: camera is fixed orientation
                    0,                          // minimum trigger interval
                    tr("Canon S100 PowerShot")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //tr("Canon EOS-M 22mm f/2"),
                    "Canon EOS-M 22mm",
                    tr("Canon"),
                    tr("EOS-M 22mm"),
                    22.3,                   // sensorWidth
                    14.9,                   // sensorHeight
                    5184,                   // imageWidth
                    3456,                   // imageHeight
                    22,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    0,                      // minimum trigger interval
                    tr("Canon EOS-M 22mm")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Canon G9X @ 10.2mm f/2
                    "Canon G9 X PowerShot",
                    tr("Canon"),
                    tr("G9 X PowerShot"),
                    13.2,                       // sensorWidth
                    8.8,                        // sensorHeight
                    5488,                       // imageWidth
                    3680,                       // imageHeight
                    10.2,                       // focalLength
                    true,                       // true: landscape orientation
                    false,                      // true: camera is fixed orientation
                    0,                          // minimum trigger interval
                    tr("Canon G9 X PowerShot")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Canon SX260 HS @ 4.5mm f/3.5
                    "Canon SX260 HS PowerShot",
                    tr("Canon"),
                    tr("SX260 HS PowerShot"),
                    6.17,                           // sensorWidth
                    4.55,                           // sensorHeight
                    4000,                           // imageWidth
                    3000,                           // imageHeight
                    4.5,                            // focalLength
                    true,                           // true: landscape orientation
                    false,                          // true: camera is fixed orientation
                    0,                              // minimum trigger interval
                    tr("Canon SX260 HS PowerShot")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "GoPro Hero 4",
                    tr("GoPro"),
                    tr("Hero 4"),
                    6.17,               // sensorWidth
                    4.55,               // sendsorHeight
                    4000,               // imageWidth
                    3000,               // imageHeight
                    2.98,               // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0,                  // minTriggerInterval
                    tr("GoPro Hero 4")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Parrot Sequioa RGB",
                    tr("Parrot"),
                    tr("Sequioa RGB"),
                    6.17,                       // sensorWidth
                    4.63,                       // sendsorHeight
                    4608,                       // imageWidth
                    3456,                       // imageHeight
                    4.9,                        // focalLength
                    true,                       // landscape
                    false,                      // fixedOrientation
                    1,                          // minTriggerInterval
                    tr("Parrot Sequioa RGB"));   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Parrot Sequioa Monochrome",
                    tr("Parrot"),
                    tr("Sequioa Monochrome"),
                    4.8,                                // sensorWidth
                    3.6,                                // sendsorHeight
                    1280,                               // imageWidth
                    960,                                // imageHeight
                    4.0,                                // focalLength
                    true,                               // landscape
                    false,                              // fixedOrientation
                    0.8,                                // minTriggerInterval
                    tr("Parrot Sequioa Monochrome"));    // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "RedEdge",
                    tr("RedEdge"),
                    tr("RedEdge"),
                    4.8,            // sensorWidth
                    3.6,            // sendsorHeight
                    1280,           // imageWidth
                    960,            // imageHeight
                    5.5,            // focalLength
                    true,           // landscape
                    false,          // fixedOrientation
                    0,              // minTriggerInterval
                    tr("RedEdge"));  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Ricoh GR II 18.3mm f/2.8
                    "Ricoh GR II",
                    tr("Ricoh"),
                    tr("GR II"),
                    23.7,               // sensorWidth
                    15.7,               // sendsorHeight
                    4928,               // imageWidth
                    3264,               // imageHeight
                    18.3,               // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0,                  // minTriggerInterval
                    tr("Ricoh GR II"));  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sentera Double 4K Sensor",
                    tr("Sentera"),
                    tr("Double 4K Sensor"),
                    6.2,                // sensorWidth
                    4.65,               // sendsorHeight
                    4000,               // imageWidth
                    3000,               // imageHeight
                    5.4,                // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0.8,                // minTriggerInterval
                    tr("Sentera Double 4K Sensor"));// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sentera NDVI Single Sensor",
                    tr("Sentera"),
                    tr("NDVI Single Sensor"),
                    4.68,               // sensorWidth
                    3.56,               // sendsorHeight
                    1248,               // imageWidth
                    952,                // imageHeight
                    4.14,               // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0.5,                // minTriggerInterval
                    tr("Sentera NDVI Single Sensor"));// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sentera 6X Sensor",
                    tr("Sentera"),
                    tr("6X Sensor"),
                    6.57,               // sensorWidth
                    4.93,               // sendsorHeight
                    1904,               // imageWidth
                    1428,               // imageHeight
                    8.0,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    0.2,                // minimum trigger interval
                    tr(""));             // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sentera 65R Sensor",
                    tr("Sentera"),
                    tr("65R Sensor"),
                    29.9,                // sensorWidth
                    22.4,                // sendsorHeight
                    9344,                // imageWidth
                    7000,                // imageHeight
                    27.4,                // focalLength
                    true,                // landscape
                    false,               // fixedOrientation
                    0.3,                 // minTriggerInterval
                    tr(""));              // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-6000-body-kit#product_details_default
                    // Sony a6000 Sony 16mm f/2.8"
                    "Sony a6000 16mm",
                    tr("Sony"),
                    tr("a6000 16mm"),
                    23.5,                   // sensorWidth
                    15.6,                   // sensorHeight
                    6000,                   // imageWidth
                    4000,                   // imageHeight
                    16,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    1.0,                    // minimum trigger interval
                    tr("Sony a6000 16mm"));  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a6000 35mm",
                    tr("Sony"),
                    tr("a6000 35mm"),
                    23.5,               // sensorWidth
                    15.6,               // sensorHeight
                    6000,               // imageWidth
                    4000,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "");
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a6300 Zeiss 21mm f/2.8",
                    tr("Sony"),
                    tr("a6300 Zeiss 21mm f/2.8"),
                    23.5,               // sensorWidth
                    15.6,               // sensorHeight
                    6000,               // imageWidth
                    4000,               // imageHeight
                    21,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    tr("Sony a6300 Zeiss 21mm f/2.8"));// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a6300 Sony 28mm f/2.0",
                    tr("Sony"),
                    tr("a6300 Sony 28mm f/2.0"),
                    23.5,                               // sensorWidth
                    15.6,                               // sensorHeight
                    6000,                               // imageWidth
                    4000,                               // imageHeight
                    28,                                 // focalLength
                    true,                               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                                // minimum trigger interval
                    tr("Sony a6300 Sony 28mm f/2.0"));   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7R II Zeiss 21mm f/2.8",
                    tr("Sony"),
                    tr("a7R II Zeiss 21mm f/2.8"),
                    35.814,                             // sensorWidth
                    23.876,                             // sensorHeight
                    7952,                               // imageWidth
                    5304,                               // imageHeight
                    21,                                 // focalLength
                    true,                               // true: landscape orientation
                    true,                               // true: camera is fixed orientation
                    1.0,                                // minimum trigger interval
                    tr("Sony a7R II Zeiss 21mm f/2.8")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7R II Sony 28mm f/2.0",
                    tr("Sony"),
                    tr("a7R II Sony 28mm f/2.0"),
                    35.814,             // sensorWidth
                    23.876,             // sensorHeight
                    7952,               // imageWidth
                    5304,               // imageHeight
                    28,                 // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    tr("Sony a7R II Sony 28mm f/2.0"));// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7r III 35mm",
                    tr("Sony"),
                    tr("a7r III 35mm"),
                    35.9,               // sensorWidth
                    24.0,               // sensorHeight
                    7952,               // imageWidth
                    5304,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "");
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7r IV 35mm",
                    tr("Sony"),
                    tr("a7r IV 35mm"),
                    35.7,               // sensorWidth
                    23.8,               // sensorHeight
                    9504,               // imageWidth
                    6336,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,               // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "");
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony DSC-QX30U @ 4.3mm f/3.5",
                    tr("Sony"),
                    tr("DSC-QX30U @ 4.3mm f/3.5"),
                    7.82,                               // sensorWidth
                    5.865,                              // sensorHeight
                    5184,                               // imageWidth
                    3888,                               // imageHeight
                    4.3,                                // focalLength
                    true,                               // true: landscape orientation
                    false,                              // true: camera is fixed orientation
                    2.0,                                // minimum trigger interval
                    tr("Sony DSC-QX30U @ 4.3mm f/3.5")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony DSC-RX0",
                    tr("Sony"),
                    tr("DSC-RX0"),
                    13.2,               // sensorWidth
                    8.8,                // sensorHeight
                    4800,               // imageWidth
                    3200,               // imageHeight
                    7.7,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    0,                  // minimum trigger interval
                    tr("Sony DSC-RX0")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony DSC-RX1R II 35mm",
                    tr("Sony"),
                    tr("DSC-RX1R II 35mm"),
                    35.9,             // sensorWidth
                    24.0,             // sensorHeight
                    7952,               // imageWidth
                    5304,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "");
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
                    //-- http://www.sony.com/electronics/camera-lenses/sel16f28/specifications
                    //tr("Sony ILCE-QX1 Sony 16mm f/2.8"),
                    "Sony ILCE-QX1",
                    tr("Sony"),
                    tr("ILCE-QX1"),
                    23.2,                   // sensorWidth
                    15.4,                   // sensorHeight
                    5456,                   // imageWidth
                    3632,                   // imageHeight
                    16,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    0,                      // minimum trigger interval
                    tr("Sony ILCE-QX1"));    // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
                    // Sony NEX-5R Sony 20mm f/2.8"
                    "Sony NEX-5R 20mm",
                    tr("Sony"),
                    tr("NEX-5R 20mm"),
                    23.2,                   // sensorWidth
                    15.4,                   // sensorHeight
                    4912,                   // imageWidth
                    3264,                   // imageHeight
                    20,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    1,                      // minimum trigger interval
                    tr("Sony NEX-5R 20mm")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Sony RX100 II @ 10.4mm f/1.8
                    "Sony RX100 II 28mm",
                    tr("Sony"),
                    tr("RX100 II 28mm"),
                    13.2,                // sensorWidth
                    8.8,                 // sensorHeight
                    5472,                // imageWidth
                    3648,                // imageHeight
                    10.4,                // focalLength
                    true,                // true: landscape orientation
                    false,               // true: camera is fixed orientation
                    0,                   // minimum trigger interval
                    tr("Sony RX100 II 28mm"));// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec CGOET",
                    tr("Yuneec"),
                    tr("CGOET"),
                    5.6405,             // sensorWidth
                    3.1813,             // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    3.5,                // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec CGOET")); // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec E10T",
                    tr("Yuneec"),
                    tr("E10T"),
                    5.6405,             // sensorWidth
                    3.1813,             // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    23,                 // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec E10T"));  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec E50",
                    tr("Yuneec"),
                    tr("E50"),
                    6.2372,             // sensorWidth
                    4.7058,             // sensorHeight
                    4000,               // imageWidth
                    3000,               // imageHeight
                    7.2,                // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec E50"));   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec E90",
                    tr("Yuneec"),
                    tr("E90"),
                    13.3056,            // sensorWidth
                    8.656,              // sensorHeight
                    5472,               // imageWidth
                    3648,               // imageHeight
                    8.29,               // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec E90"));   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Flir Duo R",
                    tr("Flir"),
                    tr("Duo R"),
                    160,                // sensorWidth
                    120,                // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    1.9,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    0,                  // minimum trigger interval
                    tr("Flir Duo R"));   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Flir Duo Pro R",
                    tr("Flir"),
                    tr("Duo Pro R"),
                    10.88,                // sensorWidth
                    8.704,                // sensorHeight
                    640,               // imageWidth
                    512,               // imageHeight
                    19,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                  // minimum trigger interval
                    "");   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Workswell Wiris Security Thermal Camera",
                    tr("Workswell"),
                    tr("Wiris Security"),
                    13.6,                // sensorWidth
                    10.2,                // sensorHeight
                    800,               // imageWidth
                    600,               // imageHeight
                    35,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.8,                  // minimum trigger interval
                    "");   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Workswell Wiris Security Visual Camera",
                    tr("Workswell"),
                    tr("Wiris Security"),
                    4.826,                // sensorWidth
                    3.556,                // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    4.3,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.8,                  // minimum trigger interval
                    "");   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
        (void) _cameraList.append(QVariant::fromValue(metaData));
    }

    return _cameraList;
}
