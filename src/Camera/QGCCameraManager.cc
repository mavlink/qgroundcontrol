/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCameraManager.h"
#include "CameraMetaData.h"
#include "FirmwarePlugin.h"
#include "Joystick.h"
#include "JoystickManager.h"
#include "MavlinkCameraControl.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "SimulatedCameraControl.h"

#include <cmath>
#include "GimbalControllerSettings.h"
#include "SettingsManager.h"
#include <numbers>

constexpr double kPi = std::numbers::pi_v<double>;

QGC_LOGGING_CATEGORY(CameraManagerLog, "Camera.QGCCameraManager")

namespace {
    constexpr int kHeartbeatTickMs = 500;
    constexpr int kSilentTimeoutMs = 5000;
    constexpr int kMaxRetryCount = 10;
}

QVariantList QGCCameraManager::_cameraList;

static void _requestFovOnZoom_Handler(
    void* user,
    MAV_RESULT result,
    Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
    const mavlink_message_t& message)
{
    auto* mgr = static_cast<QGCCameraManager*>(user);

    if (result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "CAMERA_FOV_STATUS request failed, result:"
                                  << result << "failure:" << failureCode;
        return;
    }

    if (message.msgid != MAVLINK_MSG_ID_CAMERA_FOV_STATUS) {
        qCDebug(CameraManagerLog) << "Unexpected msg id:" << message.msgid;
        return;
    }

    mavlink_camera_fov_status_t fov{};

    mavlink_msg_camera_fov_status_decode(&message, &fov);

    if (!mgr) return;
}

/*===========================================================================*/

QGCCameraManager::CameraStruct::CameraStruct(QGCCameraManager *manager_, uint8_t compID_, Vehicle *vehicle_)
    : manager(manager_)
    , compID(compID_)
    , vehicle(vehicle_)
{
    qCDebug(CameraManagerLog) << this;
    backoffTimer.setSingleShot(true);
}

QGCCameraManager::CameraStruct::~CameraStruct()
{
    qCDebug(CameraManagerLog) << this;
}

/*===========================================================================*/

QGCCameraManager::QGCCameraManager(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _simulatedCameraControl(new SimulatedCameraControl(vehicle, this))
{
    qCDebug(CameraManagerLog) << this;

    (void) qRegisterMetaType<CameraMetaData*>("CameraMetaData*");

    _addCameraControlToLists(_simulatedCameraControl);

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &QGCCameraManager::_vehicleReady);
    (void) connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &QGCCameraManager::_mavlinkMessageReceived);
    (void) connect(&_camerasLostHeartbeatTimer, &QTimer::timeout, this, &QGCCameraManager::_checkForLostCameras);

    _camerasLostHeartbeatTimer.setSingleShot(false);
    _lastZoomChange.start();
    _lastCameraChange.start();
    _camerasLostHeartbeatTimer.start(kHeartbeatTickMs);
}

QGCCameraManager::~QGCCameraManager()
{
    // Stop all camera info request timers and clean up
    for (auto* cameraInfo : _cameraInfoRequest) {
        cameraInfo->backoffTimer.stop();
        QObject::disconnect(&cameraInfo->backoffTimer, nullptr, nullptr, nullptr);
    }
    qDeleteAll(_cameraInfoRequest);
    _cameraInfoRequest.clear();

    // Stop the main heartbeat timer
    _camerasLostHeartbeatTimer.stop();

    qCDebug(CameraManagerLog) << this;
}

void QGCCameraManager::setCurrentCamera(int sel)
{
    if ((sel != _currentCameraIndex) && (sel >= 0) && (sel < _cameras.count())) {
        _currentCameraIndex = sel;
        emit currentCameraChanged();
        emit streamChanged();
    }
}

void QGCCameraManager::_vehicleReady(bool ready)
{
    qCDebug(CameraManagerLog) << ready;
    if (!ready) {
        return;
    }
    if (MultiVehicleManager::instance()->activeVehicle() != _vehicle) {
        return;
    }

    _vehicleReadyState = true;
    _activeJoystickChanged(JoystickManager::instance()->activeJoystick());
    (void) connect(JoystickManager::instance(), &JoystickManager::activeJoystickChanged, this, &QGCCameraManager::_activeJoystickChanged, Qt::UniqueConnection);
}

void QGCCameraManager::_mavlinkMessageReceived(const mavlink_message_t &message)
{
    // Only pay attention to camera components (MAV_COMP_ID_CAMERA..CAMERA6)
    // and the autopilot (it might proxy a non-MAVLink camera).
    const bool fromAutopilot = message.compid == MAV_COMP_ID_AUTOPILOT1;
    const bool fromCamera = (message.compid >= MAV_COMP_ID_CAMERA) && (message.compid <= MAV_COMP_ID_CAMERA6);
    if ((message.sysid == _vehicle->id()) && (fromAutopilot || fromCamera)) {
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
        case MAVLINK_MSG_ID_CAMERA_FOV_STATUS:
            _handleCameraFovStatus(message);
            break;
        default:
            break;
        }
    }
}

void QGCCameraManager::_handleHeartbeat(const mavlink_message_t &message)
{
    const QString sCompID = QString::number(message.compid);

    if (!_cameraInfoRequest.contains(sCompID)) {
        qCDebug(CameraManagerLog) << "Heartbeat from" << QGCMAVLink::compIdToString(message.compid);
        CameraStruct *pInfo = new CameraStruct(this, message.compid, _vehicle);
        pInfo->lastHeartbeat.start();
        _cameraInfoRequest[sCompID] = pInfo;
        _requestCameraInfo(pInfo);
        return;
    }

    CameraStruct *pInfo = _cameraInfoRequest[sCompID];
    if (!pInfo) {
        qCWarning(CameraManagerLog) << sCompID << "is null";
        return;
    }

    if (pInfo->infoReceived) {
        pInfo->lastHeartbeat.start();
        return;
    }

    if (pInfo->lastHeartbeat.elapsed() > kSilentTimeoutMs) {
        qCDebug(CameraManagerLog) << "Camera" << QGCMAVLink::compIdToString(message.compid) << "reappeared after being silent. Resetting retry count and requesting info.";
        pInfo->retryCount = 0;
        pInfo->backoffTimer.stop();
        pInfo->lastHeartbeat.start();
        _requestCameraInfo(pInfo);
        return;
    }

    pInfo->lastHeartbeat.start();
}

MavlinkCameraControl *QGCCameraManager::currentCameraInstance()
{
    if ((_currentCameraIndex < _cameras.count()) && !_cameras.isEmpty()) {
        MavlinkCameraControl *pCamera = qobject_cast<MavlinkCameraControl*>(_cameras[_currentCameraIndex]);
        return pCamera;
    }
    return nullptr;
}

QGCVideoStreamInfo *QGCCameraManager::currentStreamInstance()
{
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        QGCVideoStreamInfo *pInfo = pCamera->currentStreamInstance();
        return pInfo;
    }
    return nullptr;
}

QGCVideoStreamInfo *QGCCameraManager::thermalStreamInstance()
{
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        QGCVideoStreamInfo *pInfo = pCamera->thermalStreamInstance();
        return pInfo;
    }
    return nullptr;
}

MavlinkCameraControl *QGCCameraManager::_findCamera(int id)
{
    for (int i = 0; i < _cameras.count(); i++) {
        if (!_cameras[i]) {
            continue;
        }
        MavlinkCameraControl *pCamera = qobject_cast<MavlinkCameraControl*>(_cameras[i]);
        if (!pCamera) {
            qCCritical(CameraManagerLog) << "Invalid MavlinkCameraControl instance";
            continue;
        }
        if (pCamera->compID() == id) {
            return pCamera;
        }
    }

    // qCWarning(CameraManagerLog) << "Camera component id not found:" << id;
    return nullptr;
}

void QGCCameraManager::_addCameraControlToLists(MavlinkCameraControl *cameraControl)
{
    if (qobject_cast<SimulatedCameraControl*>(cameraControl)) {
        qCDebug(CameraManagerLog) << "Adding simulated camera to list";
    } else {
        qCDebug(CameraManagerLog) << "Adding real camera to list - simulated camera will be removed if present";
    }

    _cameras.append(cameraControl);
    _cameraLabels.append(cameraControl->modelName());
    emit camerasChanged();
    emit cameraLabelsChanged();

    // If simulated camera is in list, remove it when a real camera appears
    if ((_cameras.count() == 2) && (_cameras[0] == _simulatedCameraControl)) {
        (void) _cameras.removeAt(0);
        (void) _cameraLabels.removeAt(0);
        emit camerasChanged();
        emit cameraLabelsChanged();
        emit currentCameraChanged();
    }
}

void QGCCameraManager::_handleCameraInfo(const mavlink_message_t& message)
{
    const QString sCompID = QString::number(message.compid);
    if (!_cameraInfoRequest.contains(sCompID)) {
        qCDebug(CameraManagerLog) << "Ignoring - Camera info not requested for component" << QGCMAVLink::compIdToString(message.compid);
        return;
    }
    if (_cameraInfoRequest[sCompID]->infoReceived) {
        qCDebug(CameraManagerLog) << "Ignoring - Already received camera info for component" << QGCMAVLink::compIdToString(message.compid);
        return;
    }

    mavlink_camera_information_t info{};
    mavlink_msg_camera_information_decode(&message, &info);
    qCDebug(CameraManagerLog) << "Camera information received from" << QGCMAVLink::compIdToString(message.compid)
                          << "Model:" << reinterpret_cast<const char*>(info.model_name);
    qCDebug(CameraManagerLog) << "Creating MavlinkCameraControl for camera";

    MavlinkCameraControl *pCamera = _vehicle->firmwarePlugin()->createCameraControl(&info, _vehicle, message.compid, this);
    if (pCamera) {
        _addCameraControlToLists(pCamera);

        _cameraInfoRequest[sCompID]->infoReceived = true;
        _cameraInfoRequest[sCompID]->retryCount = 0;
        _cameraInfoRequest[sCompID]->backoffTimer.stop();
        qCDebug(CameraManagerLog) << "Success for compId" << QGCMAVLink::compIdToString(message.compid) << "- reset retry counter";
    }

    double aspect = std::numeric_limits<double>::quiet_NaN();

    if (info.resolution_h > 0 && info.resolution_v > 0) {
        aspect = double(info.resolution_v) / double(info.resolution_h);
    } else if (info.sensor_size_h > 0.f && info.sensor_size_v > 0.f) {
        aspect = double(info.sensor_size_v) / double(info.sensor_size_h);
    }

    _aspectByCompId.insert(message.compid, aspect);

    const double sensorH = static_cast<double>(info.sensor_size_h);
    const double sensorV = static_cast<double>(info.sensor_size_v);
    const double focal   = static_cast<double>(info.focal_length);
}

void QGCCameraManager::_checkForLostCameras()
{
    QList<QString> stale;
    for (auto it = _cameraInfoRequest.cbegin(), end = _cameraInfoRequest.cend(); it != end; ++it) {
        const auto *info = it.value();
        if (info && info->infoReceived && (info->lastHeartbeat.elapsed() > kSilentTimeoutMs)) {
            stale.push_back(it.key());
        }
    }
    if (stale.isEmpty()) {
        return;
    }

    bool removedAny = false;
    for (const QString& key : std::as_const(stale)) {
        CameraStruct* pInfo = _cameraInfoRequest.take(key);
        if (!pInfo) {
            continue;
        }

        MavlinkCameraControl* pCamera = _findCamera(pInfo->compID);
        if (pCamera) {
            const int idx = _cameras.indexOf(pCamera);
            if (idx >= 0) {
                qCDebug(CameraManagerLog) << "Removing lost camera" << QGCMAVLink::compIdToString(pInfo->compID);
                removedAny = true;
                (void) _cameraLabels.removeAt(idx);
                (void) _cameras.removeAt(idx);
                pCamera->deleteLater();
            }
        }

        delete pInfo;
    }

    if (!removedAny) {
        return;
    }

    if (_cameras.isEmpty()) {
        _addCameraControlToLists(_simulatedCameraControl);
    }

    emit cameraLabelsChanged();
    emit camerasChanged();

    if (_currentCameraIndex != 0) {
        _currentCameraIndex = 0;
        emit currentCameraChanged();
    }
    emit streamChanged();
}

void QGCCameraManager::_handleCaptureStatus(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_camera_capture_status_t cap{};
        mavlink_msg_camera_capture_status_decode(&message, &cap);
        pCamera->handleCaptureStatus(cap);
    }
}

void QGCCameraManager::_handleStorageInfo(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_storage_information_t st{};
        mavlink_msg_storage_information_decode(&message, &st);
        pCamera->handleStorageInfo(st);
    }
}

void QGCCameraManager::_handleCameraSettings(const mavlink_message_t& message)
{
    auto pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_camera_settings_t settings{};
        mavlink_msg_camera_settings_decode(&message, &settings);
        pCamera->handleSettings(settings);

        const int newZoom = static_cast<int>(settings.zoomLevel);
        if (QThread::currentThread() == thread()) {
            _setCurrentZoomLevel(newZoom);
        } else {
            QMetaObject::invokeMethod(
                this,
                "_setCurrentZoomLevel",
                Qt::QueuedConnection,
                Q_ARG(int, newZoom)
            );
        }

        requestCameraFovForComp(message.compid);
    }
}

void QGCCameraManager::_handleParamAck(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_param_ext_ack_t ack{};
        mavlink_msg_param_ext_ack_decode(&message, &ack);
        pCamera->handleParamAck(ack);
    }
}

void QGCCameraManager::_handleParamValue(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_param_ext_value_t value{};
        mavlink_msg_param_ext_value_decode(&message, &value);
        pCamera->handleParamValue(value);
    }
}

void QGCCameraManager::_handleVideoStreamInfo(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_video_stream_information_t streamInfo{};
        mavlink_msg_video_stream_information_decode(&message, &streamInfo);
        pCamera->handleVideoInfo(&streamInfo);
        emit streamChanged();
    }
}

void QGCCameraManager::_handleVideoStreamStatus(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_video_stream_status_t streamStatus{};
        mavlink_msg_video_stream_status_decode(&message, &streamStatus);
        pCamera->handleVideoStatus(&streamStatus);
    }
}

void QGCCameraManager::_handleBatteryStatus(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_battery_status_t batteryStatus{};
        mavlink_msg_battery_status_decode(&message, &batteryStatus);
        pCamera->handleBatteryStatus(batteryStatus);
    }
}

void QGCCameraManager::_handleTrackingImageStatus(const mavlink_message_t &message)
{
    MavlinkCameraControl *pCamera = _findCamera(message.compid);
    if (pCamera) {
        mavlink_camera_tracking_image_status_t tis{};
        mavlink_msg_camera_tracking_image_status_decode(&message, &tis);
        pCamera->handleTrackingImageStatus(&tis);
    }
}

static void _handleCameraInfoRetry(QGCCameraManager::CameraStruct *cameraInfo);

static void _requestCameraInfoCommandResultHandler(void *resultHandlerData, int /*compId*/, const mavlink_command_ack_t &ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    auto *cameraInfo = static_cast<QGCCameraManager::CameraStruct*>(resultHandlerData);

    if (ack.result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "MAV_CMD_REQUEST_CAMERA_INFORMATION failed. compId" << QGCMAVLink::compIdToString(cameraInfo->compID)
                                    << "Result:" << QGCMAVLink::mavResultToString(ack.result)
                                    << "FailureCode:" << Vehicle::mavCmdResultFailureCodeToString(failureCode)
                                    << "retryCount:" << cameraInfo->retryCount;
        _handleCameraInfoRetry(cameraInfo);
    }
}

static void _requestCameraInfoMessageResultHandler(void *resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, [[maybe_unused]] const mavlink_message_t &message)
{
    auto *cameraInfo = static_cast<QGCCameraManager::CameraStruct*>(resultHandlerData);

    if (result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraManagerLog) << "MAV_CMD_REQUEST_MESSAGE:MAVLINK_MSG_ID_CAMERA_INFORMATION failed. compId" << QGCMAVLink::compIdToString(cameraInfo->compID)
                                    << "Result:" << QGCMAVLink::mavResultToString(result)
                                    << "FailureCode:" << Vehicle::requestMessageResultHandlerFailureCodeToString(failureCode)
                                    << "retryCount:" << cameraInfo->retryCount;
        _handleCameraInfoRetry(cameraInfo);
    }
}

static void _requestCameraInfoHelper(QGCCameraManager *manager, QGCCameraManager::CameraStruct *pInfo)
{
    // Give up after max attempts
    if (pInfo->retryCount >= kMaxRetryCount) {
        qCDebug(CameraManagerLog) << "Giving up requesting camera info after" << pInfo->retryCount << "attempts for compId" << QGCMAVLink::compIdToString(pInfo->compID);
        return;
    }

    // Alternate between REQUEST_MESSAGE and REQUEST_CAMERA_INFORMATION
    if ((pInfo->retryCount % 2) == 0) {
        qCDebug(CameraManagerLog) << "Using MAV_CMD_REQUEST_MESSAGE:CAMERA_INFORMATION for compId" << QGCMAVLink::compIdToString(pInfo->compID);
        manager->vehicle()->requestMessage(_requestCameraInfoMessageResultHandler, pInfo, pInfo->compID, MAVLINK_MSG_ID_CAMERA_INFORMATION);
    } else {
        qCDebug(CameraManagerLog) << "Using MAV_CMD_REQUEST_CAMERA_INFORMATION for compId" << QGCMAVLink::compIdToString(pInfo->compID);

        Vehicle::MavCmdAckHandlerInfo_t ackHandlerInfo{};
        ackHandlerInfo.resultHandler        = _requestCameraInfoCommandResultHandler;
        ackHandlerInfo.resultHandlerData    = pInfo;
        ackHandlerInfo.progressHandler      = nullptr;
        ackHandlerInfo.progressHandlerData  = nullptr;

        pInfo->vehicle->sendMavCommandWithHandler(&ackHandlerInfo, pInfo->compID, MAV_CMD_REQUEST_CAMERA_INFORMATION, 1 /* request camera capabilities */);
    }
}

static void _handleCameraInfoRetry(QGCCameraManager::CameraStruct *cameraInfo)
{
    if (!cameraInfo) {
        return;
    }

    QGCCameraManager *manager = cameraInfo->manager;
    if (!manager) {
        qCDebug(CameraManagerLog) << "manager is unavailable for compId" << QGCMAVLink::compIdToString(cameraInfo->compID);
        return;
    }

    cameraInfo->retryCount++;

    // For even attempts >= 2, use exponential backoff
    if ((cameraInfo->retryCount >= 2) && ((cameraInfo->retryCount % 2) == 0)) {
        const int delaySeconds = 1 << (cameraInfo->retryCount / 2);
        const int delayMs = delaySeconds * 1000;

        qCDebug(CameraManagerLog) << "Waiting" << delaySeconds << "seconds before retry for compId" << QGCMAVLink::compIdToString(cameraInfo->compID);

        cameraInfo->backoffTimer.stop();
        (void) QObject::disconnect(&cameraInfo->backoffTimer, nullptr, nullptr, nullptr);

        // Capture compID by value and look up the struct
        const uint8_t compId = cameraInfo->compID;
        QPointer<QGCCameraManager> mgrGuard(manager);

        (void) QObject::connect(&cameraInfo->backoffTimer, &QTimer::timeout,
                                &cameraInfo->backoffTimer,  // context ensures timer is still alive
                                [mgrGuard, compId]() {
            if (!mgrGuard) {
                return;
            }

            auto* info = mgrGuard->findCameraStruct(compId);
            if (info) {
                _requestCameraInfoHelper(mgrGuard.data(), info);
            }
        });

        cameraInfo->backoffTimer.start(delayMs);
    } else {
        _requestCameraInfoHelper(manager, cameraInfo);
    }
}

void QGCCameraManager::_requestCameraInfo(CameraStruct *pInfo)
{
    if (!pInfo) {
        return;
    }
    _requestCameraInfoHelper(this, pInfo);
}

void QGCCameraManager::_activeJoystickChanged(Joystick *joystick)
{
    qCDebug(CameraManagerLog) << "Joystick changed";
    if (_activeJoystick) {
        (void) disconnect(_activeJoystick, &Joystick::stepZoom,            this, &QGCCameraManager::_stepZoom);
        (void) disconnect(_activeJoystick, &Joystick::startContinuousZoom, this, &QGCCameraManager::_startZoom);
        (void) disconnect(_activeJoystick, &Joystick::stopContinuousZoom,  this, &QGCCameraManager::_stopZoom);
        (void) disconnect(_activeJoystick, &Joystick::stepCamera,          this, &QGCCameraManager::_stepCamera);
        (void) disconnect(_activeJoystick, &Joystick::stepStream,          this, &QGCCameraManager::_stepStream);
        (void) disconnect(_activeJoystick, &Joystick::triggerCamera,       this, &QGCCameraManager::_triggerCamera);
        (void) disconnect(_activeJoystick, &Joystick::startVideoRecord,    this, &QGCCameraManager::_startVideoRecording);
        (void) disconnect(_activeJoystick, &Joystick::stopVideoRecord,     this, &QGCCameraManager::_stopVideoRecording);
        (void) disconnect(_activeJoystick, &Joystick::toggleVideoRecord,   this, &QGCCameraManager::_toggleVideoRecording);
    }

    _activeJoystick = joystick;

    if (_activeJoystick) {
        (void) connect(_activeJoystick, &Joystick::stepZoom,            this, &QGCCameraManager::_stepZoom, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::startContinuousZoom, this, &QGCCameraManager::_startZoom, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stopContinuousZoom,  this, &QGCCameraManager::_stopZoom, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stepCamera,          this, &QGCCameraManager::_stepCamera, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stepStream,          this, &QGCCameraManager::_stepStream, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::triggerCamera,       this, &QGCCameraManager::_triggerCamera, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::startVideoRecord,    this, &QGCCameraManager::_startVideoRecording, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::stopVideoRecord,     this, &QGCCameraManager::_stopVideoRecording, Qt::UniqueConnection);
        (void) connect(_activeJoystick, &Joystick::toggleVideoRecord,   this, &QGCCameraManager::_toggleVideoRecording, Qt::UniqueConnection);
    }
}

void QGCCameraManager::_triggerCamera()
{
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->takePhoto();
    }
}

void QGCCameraManager::_startVideoRecording()
{
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->startVideoRecording();
    }
}

void QGCCameraManager::_stopVideoRecording()
{
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->stopVideoRecording();
    }
}

void QGCCameraManager::_toggleVideoRecording()
{
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->toggleVideoRecording();
    }
}

void QGCCameraManager::_stepZoom(int direction)
{
    if (_lastZoomChange.elapsed() > 40) {
        _lastZoomChange.start();
        qCDebug(CameraManagerLog) << "Step Camera Zoom" << direction;
        MavlinkCameraControl *pCamera = currentCameraInstance();
        if (pCamera) {
            pCamera->stepZoom(direction);
        }
    }
}

void QGCCameraManager::_startZoom(int direction)
{
    qCDebug(CameraManagerLog) << "Start Camera Zoom" << direction;
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->startZoom(direction);
    }
}

void QGCCameraManager::_stopZoom()
{
    qCDebug(CameraManagerLog) << "Stop Camera Zoom";
    MavlinkCameraControl *pCamera = currentCameraInstance();
    if (pCamera) {
        pCamera->stopZoom();
    }
}

void QGCCameraManager::_stepCamera(int direction)
{
    if (_lastCameraChange.elapsed() > 1000) {
        _lastCameraChange.start();
        qCDebug(CameraManagerLog) << "Step Camera" << direction;
        int camera = _currentCameraIndex + direction;
        if (camera < 0) {
            camera = _cameras.count() - 1;
        } else if (camera >= _cameras.count()) {
            camera = 0;
        }
        setCurrentCamera(camera);
    }
}

void QGCCameraManager::_stepStream(int direction)
{
    if (_lastCameraChange.elapsed() > 1000) {
        _lastCameraChange.start();
        MavlinkCameraControl *pCamera = currentCameraInstance();
        if (pCamera) {
            qCDebug(CameraManagerLog) << "Step Camera Stream" << direction;
            int stream = pCamera->currentStream() + direction;
            if (stream < 0) {
                stream = pCamera->streams()->count() - 1;
            } else if (stream >= pCamera->streams()->count()) {
                stream = 0;
            }
            pCamera->setCurrentStream(stream);
        }
    }
}

const QVariantList &QGCCameraManager::cameraList() const
{
    if (_cameraList.isEmpty()) {
        const QList<CameraMetaData*> cams = CameraMetaData::parseCameraMetaData();
        _cameraList.reserve(cams.size());

        for (CameraMetaData *cam : cams) {
            _cameraList << QVariant::fromValue(cam);
        }
    }
    return _cameraList;
}

void QGCCameraManager::requestCameraFovForComp(int compId) {
    if (!_vehicle) {
        qCWarning(CameraManagerLog) << "requestCameraFovForComp: vehicle is null";
        return;
    }
    _vehicle->requestMessage(_requestFovOnZoom_Handler, /*user*/this,
                             compId, MAVLINK_MSG_ID_CAMERA_FOV_STATUS);
}

//-----------------------------------------------------------------------------
double QGCCameraManager::aspectForComp(int compId) const {
    auto it = _aspectByCompId.constFind(compId);
    return (it == _aspectByCompId.cend())
           ? std::numeric_limits<double>::quiet_NaN()
           : it.value();
}

double QGCCameraManager::currentCameraAspect(){
    if (auto* cam = currentCameraInstance()) {
        return aspectForComp(cam->compID());
    }
    return std::numeric_limits<double>::quiet_NaN();
}
void QGCCameraManager::_handleCameraFovStatus(const mavlink_message_t& message)
{
    mavlink_camera_fov_status_t fov{};
    mavlink_msg_camera_fov_status_decode(&message, &fov);

    if (!std::isfinite(fov.hfov) || fov.hfov <= 0.0 || fov.hfov >= 180.0) {
        return;
    }

    double aspect = aspectForComp(message.compid);
    if (!std::isfinite(aspect) || aspect <= 0.0) {
        aspect = 16.0 / 9.0;
    }

    const double hfovRad = fov.hfov * kPi / 180.0;
    const double vfovRad = 2.0 * std::atan(std::tan(hfovRad * 0.5) * aspect);
    const double vfovDeg = vfovRad * 180.0 / kPi;

    if (!std::isfinite(vfovDeg) || vfovDeg <= 0.0 || vfovDeg >= 180.0) {
        qCWarning(CameraManagerLog) << "Invalid calculated VFOV:" << vfovDeg
                                    << "hfov:" << fov.hfov
                                    << "aspect:" << aspect
                                    << "compId:" << message.compid;
        return;
    }

    auto* settings = SettingsManager::instance()->gimbalControllerSettings();
    settings->CameraHFov()->setRawValue(fov.hfov);
    settings->CameraVFov()->setRawValue(vfovDeg);
}

void QGCCameraManager::_setCurrentZoomLevel(int level)
{
    if (_zoomValueCurrent == level) {
        return;
    }
    _zoomValueCurrent = level;
    emit currentZoomLevelChanged();
}

int QGCCameraManager::currentZoomLevel() const
{
    return _zoomValueCurrent;
}
