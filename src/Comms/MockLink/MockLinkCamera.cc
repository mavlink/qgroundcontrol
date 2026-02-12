#include "MockLinkCamera.h"
#include "MockLink.h"
#include "MissionManager/MissionCommandTree.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>

QGC_LOGGING_CATEGORY(MockLinkCameraLog, "Comms.MockLink.MockLinkCamera")

MockLinkCamera::MockLinkCamera(MockLink *mockLink,
                               bool captureVideo,
                               bool captureImage,
                               bool hasModes,
                               bool canCaptureImageInVideoMode,
                               bool canCaptureVideoInImageMode,
                               bool hasBasicZoom,
                               bool hasTrackingPoint,
                               bool hasTrackingRectangle)
    : _mockLink(mockLink)
{
    // Build capability flags from configuration
    uint32_t configuredFlags = 0;
    if (captureVideo)                   configuredFlags |= CAMERA_CAP_FLAGS_CAPTURE_VIDEO;
    if (captureImage)                   configuredFlags |= CAMERA_CAP_FLAGS_CAPTURE_IMAGE;
    if (hasModes)                       configuredFlags |= CAMERA_CAP_FLAGS_HAS_MODES;
    if (canCaptureImageInVideoMode)     configuredFlags |= CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE;
    if (canCaptureVideoInImageMode)     configuredFlags |= CAMERA_CAP_FLAGS_CAN_CAPTURE_VIDEO_IN_IMAGE_MODE;
    if (hasBasicZoom)                   configuredFlags |= CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM;
    if (hasTrackingPoint)               configuredFlags |= CAMERA_CAP_FLAGS_HAS_TRACKING_POINT;
    if (hasTrackingRectangle)           configuredFlags |= CAMERA_CAP_FLAGS_HAS_TRACKING_RECTANGLE;

    // Camera 1: full-featured with configurable flags + always-on features
    _cameras[0].compId   = MAV_COMP_ID_CAMERA;
    _cameras[0].capFlags = configuredFlags
                         | CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS
                         | CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM;

    // Camera 2: photo-only (always CAPTURE_IMAGE only)
    _cameras[1].compId   = MAV_COMP_ID_CAMERA2;
    _cameras[1].capFlags = CAMERA_CAP_FLAGS_CAPTURE_IMAGE;
}

MockLinkCamera::CameraState *MockLinkCamera::_findCamera(uint8_t compId)
{
    for (uint8_t i = 0; i < kNumCameras; i++) {
        if (_cameras[i].compId == compId) {
            return &_cameras[i];
        }
    }
    return nullptr;
}

void MockLinkCamera::sendCameraHeartbeats()
{
    for (uint8_t i = 0; i < kNumCameras; i++) {
        mavlink_message_t msg{};
        (void) mavlink_msg_heartbeat_pack_chan(
            _mockLink->vehicleId(),
            _cameras[i].compId,
            _mockLink->mavlinkChannel(),
            &msg,
            MAV_TYPE_CAMERA,
            MAV_AUTOPILOT_INVALID,
            0,
            0,
            MAV_STATE_ACTIVE);
        _mockLink->respondWithMavlinkMessage(msg);
    }
}

bool MockLinkCamera::handleCameraCommand(const mavlink_command_long_t &request, uint8_t targetCompId)
{
    CameraState *cam = _findCamera(targetCompId);
    if (!cam) {
        return false;
    }

    switch (request.command) {
    case MAV_CMD_REQUEST_CAMERA_INFORMATION:
        _sendCameraInformation(targetCompId);
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        return true;

    case MAV_CMD_REQUEST_CAMERA_SETTINGS:
        _sendCameraSettings(targetCompId);
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        return true;

    case MAV_CMD_REQUEST_STORAGE_INFORMATION:
        _sendStorageInformation(targetCompId);
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        return true;

    case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
        _sendCameraCaptureStatus(targetCompId);
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        return true;

    case MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION:
    {
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM)) {
            _sendCommandAck(targetCompId, request.command, MAV_RESULT_DENIED);
            return true;
        }
        const uint8_t streamId = static_cast<uint8_t>(request.param1);
        if (streamId == 0) {
            // Request all streams
            for (uint8_t s = 1; s <= kNumStreams; s++) {
                _sendVideoStreamInformation(targetCompId, s);
            }
        } else {
            _sendVideoStreamInformation(targetCompId, streamId);
        }
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        return true;
    }

    case MAV_CMD_REQUEST_VIDEO_STREAM_STATUS:
    {
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM)) {
            _sendCommandAck(targetCompId, request.command, MAV_RESULT_DENIED);
            return true;
        }
        const uint8_t streamId = static_cast<uint8_t>(request.param1);
        if (streamId == 0) {
            for (uint8_t s = 1; s <= kNumStreams; s++) {
                _sendVideoStreamStatus(targetCompId, s);
            }
        } else {
            _sendVideoStreamStatus(targetCompId, streamId);
        }
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        return true;
    }

    case MAV_CMD_SET_CAMERA_MODE:
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_HAS_MODES)) {
            _sendCommandAck(targetCompId, request.command, MAV_RESULT_DENIED);
            return true;
        }
        cam->cameraMode = static_cast<uint8_t>(request.param2);
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "mode set to" << cam->cameraMode;
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        // Send updated settings after mode change
        _sendCameraSettings(targetCompId);
        return true;

    case MAV_CMD_IMAGE_START_CAPTURE:
    {
        const float interval = request.param1;  // seconds (0 = single shot)
        const int count = static_cast<int>(request.param3);
        cam->imagesCaptured += (count > 0) ? count : 1;

        // Set capture status based on interval
        if (interval > 0) {
            // Interval capture mode
            cam->image_status = ImageCaptureIntervalCapture;
            cam->image_interval = interval;
        } else {
            // Single shot
            cam->image_status = ImageCaptureInProgress;
            cam->image_interval = 0.0f;
        }

        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "image capture started"
                                   << "interval:" << interval << "count:" << count
                                   << "total captured:" << cam->imagesCaptured;
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        // Send capture status update
        _sendCameraCaptureStatus(targetCompId);
        return true;
    }

    case MAV_CMD_IMAGE_STOP_CAPTURE:
        cam->image_status = ImageCaptureIdle;
        cam->image_interval = 0.0f;
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "image capture stopped";
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        _sendCameraCaptureStatus(targetCompId);
        return true;

    case MAV_CMD_VIDEO_START_CAPTURE:
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_CAPTURE_VIDEO)) {
            _sendCommandAck(targetCompId, request.command, MAV_RESULT_DENIED);
            return true;
        }
        cam->recording = true;
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "video recording started";
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        _sendCameraCaptureStatus(targetCompId);
        return true;

    case MAV_CMD_VIDEO_STOP_CAPTURE:
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_CAPTURE_VIDEO)) {
            _sendCommandAck(targetCompId, request.command, MAV_RESULT_DENIED);
            return true;
        }
        cam->recording = false;
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "video recording stopped";
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        _sendCameraCaptureStatus(targetCompId);
        return true;

    case MAV_CMD_STORAGE_FORMAT:
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "storage formatted";
        cam->imagesCaptured = 0;
        cam->image_status = ImageCaptureIdle;
        cam->image_interval = 0.0f;
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        _sendStorageInformation(targetCompId);
        return true;

    case MAV_CMD_SET_CAMERA_ZOOM:
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM)) {
            _sendCommandAck(targetCompId, request.command, MAV_RESULT_DENIED);
            return true;
        }
        cam->zoomLevel = request.param2;
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "zoom set to" << cam->zoomLevel;
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        _sendCameraSettings(targetCompId);
        return true;

    case MAV_CMD_SET_CAMERA_FOCUS:
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS)) {
            _sendCommandAck(targetCompId, request.command, MAV_RESULT_DENIED);
            return true;
        }
        cam->focusLevel = request.param2;
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "focus set to" << cam->focusLevel;
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        _sendCameraSettings(targetCompId);
        return true;

    case MAV_CMD_RESET_CAMERA_SETTINGS:
        cam->cameraMode = CAMERA_MODE_IMAGE;
        cam->zoomLevel = 1.0f;
        cam->focusLevel = 0.0f;
        cam->image_status = ImageCaptureIdle;
        cam->image_interval = 0.0f;
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "settings reset";
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        _sendCameraSettings(targetCompId);
        return true;

    case MAV_CMD_CAMERA_TRACK_POINT:
    case MAV_CMD_CAMERA_TRACK_RECTANGLE:
    case MAV_CMD_CAMERA_STOP_TRACKING:
        qCDebug(MockLinkCameraLog) << "Camera" << targetCompId << "tracking command" << request.command;
        _sendCommandAck(targetCompId, request.command, MAV_RESULT_ACCEPTED);
        return true;

    default:
        break;
    }

    return false;
}

bool MockLinkCamera::handleRequestMessage(const mavlink_command_long_t &request, uint8_t targetCompId)
{
    const CameraState *cam = _findCamera(targetCompId);
    if (!cam) {
        return false;
    }

    const int msgId = static_cast<int>(request.param1);

    switch (msgId) {
    case MAVLINK_MSG_ID_CAMERA_INFORMATION:
        _sendCameraInformation(targetCompId);
        _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
        return true;

    case MAVLINK_MSG_ID_CAMERA_SETTINGS:
        _sendCameraSettings(targetCompId);
        _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
        return true;

    case MAVLINK_MSG_ID_STORAGE_INFORMATION:
        _sendStorageInformation(targetCompId);
        _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
        return true;

    case MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS:
        _sendCameraCaptureStatus(targetCompId);
        _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
        return true;

    case MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION:
    {
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM)) {
            _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_DENIED);
            return true;
        }
        const uint8_t streamId = static_cast<uint8_t>(request.param2);
        if (streamId == 0) {
            for (uint8_t s = 1; s <= kNumStreams; s++) {
                _sendVideoStreamInformation(targetCompId, s);
            }
        } else {
            _sendVideoStreamInformation(targetCompId, streamId);
        }
        _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
        return true;
    }

    case MAVLINK_MSG_ID_VIDEO_STREAM_STATUS:
    {
        if (!(cam->capFlags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM)) {
            _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_DENIED);
            return true;
        }
        const uint8_t streamId = static_cast<uint8_t>(request.param2);
        if (streamId == 0) {
            for (uint8_t s = 1; s <= kNumStreams; s++) {
                _sendVideoStreamStatus(targetCompId, s);
            }
        } else {
            _sendVideoStreamStatus(targetCompId, streamId);
        }
        _sendCommandAck(targetCompId, MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
        return true;
    }

    default:
        break;
    }

    return false;
}

void MockLinkCamera::_sendCameraInformation(uint8_t compId)
{
    const CameraState *cam = _findCamera(compId);
    if (!cam) {
        return;
    }

    const int cameraIndex = compId - MAV_COMP_ID_CAMERA;

    const uint8_t vendorName[32] = "MockLink";
    const QString model = QStringLiteral("MockCam %1").arg(cameraIndex + 1);
    QByteArray modelBA = model.toLocal8Bit();
    modelBA.resize(MAVLINK_MSG_CAMERA_INFORMATION_FIELD_MODEL_NAME_LEN);

    mavlink_message_t msg{};
    (void) mavlink_msg_camera_information_pack_chan(
        _mockLink->vehicleId(),
        compId,
        _mockLink->mavlinkChannel(),
        &msg,
        0,                                              // time_boot_ms
        vendorName,
        reinterpret_cast<const uint8_t *>(modelBA.constData()),
        0,                                              // firmware_version
        0,                                              // focal_length
        0,                                              // sensor_size_h
        0,                                              // sensor_size_v
        1920,                                           // resolution_h
        1080,                                           // resolution_v
        0,                                              // lens_id
        cam->capFlags,                                  // flags
        0,                                              // cam_definition_version
        "",                                             // cam_definition_uri
        0,                                              // gimbal_device_id
        0);                                             // flags (reserved)
    _mockLink->respondWithMavlinkMessage(msg);

    qCDebug(MockLinkCameraLog) << "Sent CAMERA_INFORMATION for compId" << compId << "model" << model;
}

void MockLinkCamera::_sendCameraSettings(uint8_t compId)
{
    const CameraState *cam = _findCamera(compId);
    if (!cam) {
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_camera_settings_pack_chan(
        _mockLink->vehicleId(),
        compId,
        _mockLink->mavlinkChannel(),
        &msg,
        0,                  // time_boot_ms
        cam->cameraMode,    // mode_id
        cam->zoomLevel,     // zoomLevel
        cam->focusLevel,    // focusLevel
        0);                 // camera_device_id
    _mockLink->respondWithMavlinkMessage(msg);

    qCDebug(MockLinkCameraLog) << "Sent CAMERA_SETTINGS for compId" << compId
                               << "mode" << cam->cameraMode
                               << "zoom" << cam->zoomLevel
                               << "focus" << cam->focusLevel;
}

void MockLinkCamera::_sendStorageInformation(uint8_t compId)
{
    mavlink_message_t msg{};
    (void) mavlink_msg_storage_information_pack_chan(
        _mockLink->vehicleId(),
        compId,
        _mockLink->mavlinkChannel(),
        &msg,
        0,                                      // time_boot_ms
        1,                                      // storage_id
        1,                                      // storage_count
        STORAGE_STATUS_READY,                   // status
        static_cast<float>(kStorageTotalMiB),   // total_capacity (MiB)
        static_cast<float>(kStorageTotalMiB - kStorageFreeMiB), // used_capacity (MiB)
        static_cast<float>(kStorageFreeMiB),    // available_capacity (MiB)
        NAN,                                    // read_speed
        NAN,                                    // write_speed
        STORAGE_TYPE_SD,                        // type
        "",                                     // name
        0);                                     // storage_usage
    _mockLink->respondWithMavlinkMessage(msg);

    qCDebug(MockLinkCameraLog) << "Sent STORAGE_INFORMATION for compId" << compId;
}

void MockLinkCamera::_sendCameraCaptureStatus(uint8_t compId)
{
    const CameraState *cam = _findCamera(compId);
    if (!cam) {
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_camera_capture_status_pack_chan(
        _mockLink->vehicleId(),
        compId,
        _mockLink->mavlinkChannel(),
        &msg,
        0,                                              // time_boot_ms
        cam->image_status,                              // image_status (ImageCaptureStatus enum)
        cam->recording ? 1 : 0,                         // video_status (0=idle, 1=running)
        cam->image_interval,                            // image_interval
        0,                                              // recording_time_ms
        static_cast<float>(kStorageFreeMiB),             // available_capacity
        cam->imagesCaptured,                                // image_count
        0);                                                 // camera_device_id
    _mockLink->respondWithMavlinkMessage(msg);

    qCDebug(MockLinkCameraLog) << "Sent CAMERA_CAPTURE_STATUS for compId" << compId
                               << "status" << cam->image_status
                               << "interval" << cam->image_interval
                               << "recording" << cam->recording
                               << "images" << cam->imagesCaptured;
}

void MockLinkCamera::_sendVideoStreamInformation(uint8_t compId, uint8_t streamId)
{
    const int cameraIndex = compId - MAV_COMP_ID_CAMERA;
    const QString name = QStringLiteral("Stream %1-%2").arg(cameraIndex + 1).arg(streamId);
    QByteArray nameBA = name.toLocal8Bit();
    nameBA.resize(MAVLINK_MSG_VIDEO_STREAM_INFORMATION_FIELD_NAME_LEN);

    const QString uri = QStringLiteral("udp://127.0.0.1:5600");
    QByteArray uriBA = uri.toLocal8Bit();
    uriBA.resize(MAVLINK_MSG_VIDEO_STREAM_INFORMATION_FIELD_URI_LEN);

    mavlink_message_t msg{};
    (void) mavlink_msg_video_stream_information_pack_chan(
        _mockLink->vehicleId(),
        compId,
        _mockLink->mavlinkChannel(),
        &msg,
        streamId,                               // stream_id
        kNumStreams,                            // count
        VIDEO_STREAM_TYPE_RTPUDP,               // type
        VIDEO_STREAM_STATUS_FLAGS_RUNNING,      // flags
        30,                                     // framerate
        1920,                                   // resolution_h
        1080,                                   // resolution_v
        4000,                                   // bitrate (kbit/s)
        0,                                      // rotation
        70,                                     // hfov
        nameBA.constData(),
        uriBA.constData(),
        VIDEO_STREAM_ENCODING_H264,
        0);                                     // encoding_sub
    _mockLink->respondWithMavlinkMessage(msg);

    qCDebug(MockLinkCameraLog) << "Sent VIDEO_STREAM_INFORMATION for compId" << compId << "stream" << streamId;
}

void MockLinkCamera::_sendVideoStreamStatus(uint8_t compId, uint8_t streamId)
{
    mavlink_message_t msg{};
    (void) mavlink_msg_video_stream_status_pack_chan(
        _mockLink->vehicleId(),
        compId,
        _mockLink->mavlinkChannel(),
        &msg,
        streamId,                               // stream_id
        VIDEO_STREAM_STATUS_FLAGS_RUNNING,      // flags
        30,                                     // framerate
        1920,                                   // resolution_h
        1080,                                   // resolution_v
        4000,                                   // bitrate (kbit/s)
        0,                                      // rotation
        70,                                     // hfov
        0);                                     // encoding (reserved in status)
    _mockLink->respondWithMavlinkMessage(msg);
}

void MockLinkCamera::_sendCommandAck(uint8_t compId, uint16_t command, uint8_t result)
{
    mavlink_message_t msg{};
    (void) mavlink_msg_command_ack_pack_chan(
        _mockLink->vehicleId(),
        compId,
        _mockLink->mavlinkChannel(),
        &msg,
        command,
        result,
        0,   // progress
        0,   // result_param2
        0,   // target_system
        0);  // target_component
    _mockLink->respondWithMavlinkMessage(msg);

    QString commandName = MissionCommandTree::instance()->rawName(static_cast<MAV_CMD>(command));
    QString logMsg = QStringLiteral("Sent COMMAND_ACK for compId: %1 command: %2 result: %3")
                        .arg(compId).arg(commandName).arg(result);

    if (command == MAV_CMD_REQUEST_MESSAGE && requestedMsgId >= 0) {
        const mavlink_message_info_t* info = mavlink_get_message_info_by_id(static_cast<uint32_t>(requestedMsgId));
        QString msgName = info ? info->name : QString::number(requestedMsgId);
        logMsg += QStringLiteral(" requestedMsg: %1").arg(msgName);
    }

    qCDebug(MockLinkCameraLog) << logMsg;
}
