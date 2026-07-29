#include "DigiviewManager.h"

#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "sv_mavlink_dialect/sv_mavlink_dialect.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>

#include <cstring>
#include <limits>

#include <iostream>

QGC_LOGGING_CATEGORY(DigiviewManagerLog, "Digiview.Manager")

Q_APPLICATION_STATIC(DigiviewManager, _digiviewManagerInstance);

namespace {

constexpr uint8_t kCamTargetingLockFlagsUnchanged = 0xFF;
constexpr float kVideoOutputParametersOneShotIntervalUs = -1000.0F;
constexpr float kVideoOutputParametersSubscriptionIntervalUs = 100000.0F;

void copyStringToCharBuf(const QString& src, char* dest, int size)
{
    memset(dest, 0, static_cast<size_t>(size));

    const QByteArray utf8 = src.toUtf8();
    strncpy(dest, utf8.constData(), static_cast<size_t>(size - 1));
}

QString stringFromCharBuf(const char* src, int size)
{
    return QString::fromLatin1(src, static_cast<qsizetype>(strnlen(src, static_cast<size_t>(size))));
}

uint8_t userViewCountForLayout(uint8_t layout)
{
    switch (layout) {
    case 0:
        return 1;
    case 1:
        return 2;
    case 2:
        return 2;
    case 3:
        return 3;
    case 4:
        return 4;
    case 5:
        return 4;
    case 6:
        return 1;
    default:
        return 0;
    }
}

} // namespace

DigiviewManager* DigiviewManager::instance()
{
    return _digiviewManagerInstance();
}

DigiviewManager::DigiviewManager(QObject* parent)
    : QObject(parent)
    , _connection(new DigiviewConnection(this))
{
    connect(_connection, &DigiviewConnection::hostChanged, this, &DigiviewManager::hostChanged);
    connect(_connection, &DigiviewConnection::portChanged, this, &DigiviewManager::portChanged);
    connect(_connection, &DigiviewConnection::listenPortChanged, this, &DigiviewManager::listenPortChanged);
    connect(_connection, &DigiviewConnection::connectedChanged, this, &DigiviewManager::connectedChanged);
    connect(_connection, &DigiviewConnection::lastErrorChanged, this, &DigiviewManager::lastErrorChanged);
    connect(_connection, &DigiviewConnection::messageReceived, this, &DigiviewManager::_handleMessage);

    if (qApp) {
        connect(qApp, &QCoreApplication::aboutToQuit, this, &DigiviewManager::disconnectFromHost, Qt::QueuedConnection);
    }

}

DigiviewManager::~DigiviewManager()
{
    disconnectFromHost();
}

QString DigiviewManager::host() const
{
    return _connection->host();
}

quint16 DigiviewManager::port() const
{
    return _connection->port();
}

quint16 DigiviewManager::listenPort() const
{
    return _connection->listenPort();
}

bool DigiviewManager::connected() const
{
    return _connection->connected();
}

QString DigiviewManager::lastError() const
{
    return _connection->lastError();
}

void DigiviewManager::setHost(const QString& host)
{
    if (host.trimmed() != _connection->host()) {
        _resetRemoteSession();
    }

    _connection->setHost(host);
}

void DigiviewManager::setPort(quint16 port)
{
    if (port != _connection->port()) {
        _resetRemoteSession();
    }

    _connection->setPort(port);
}

void DigiviewManager::setListenPort(quint16 listenPort)
{
    if (listenPort != _connection->listenPort()) {
        _resetRemoteSession();
    }

    _connection->setListenPort(listenPort);
}

void DigiviewManager::setStreamName(const QString& streamName)
{
    const QString trimmedStreamName = streamName.trimmed();
    if (trimmedStreamName == _streamName) {
        return;
    }

    _resetRemoteSession();
    _streamName = trimmedStreamName;
    emit streamNameChanged();
}

void DigiviewManager::setSenderSystemId(int senderSystemId)
{
    if ((senderSystemId < 0) || (senderSystemId > std::numeric_limits<uint8_t>::max())) {
        //qCWarning(DigiviewManagerLog) << "Ignoring invalid Digiview sender system id" << senderSystemId;
        return;
    }

    const uint8_t senderId = static_cast<uint8_t>(senderSystemId);
    if (senderId == _senderSystemId) {
        return;
    }

    _senderSystemId = senderId;
    _resetRemoteSessionForSenderIdentityChange();
    emit senderIdentityChanged();
}

void DigiviewManager::setSenderComponentId(int senderComponentId)
{
    if ((senderComponentId < 0) || (senderComponentId > std::numeric_limits<uint8_t>::max())) {
        //qCWarning(DigiviewManagerLog) << "Ignoring invalid Digiview sender component id" << senderComponentId;
        return;
    }

    const uint8_t componentId = static_cast<uint8_t>(senderComponentId);
    if (componentId == _senderComponentId) {
        return;
    }

    _senderComponentId = componentId;
    _resetRemoteSessionForSenderIdentityChange();
    emit senderIdentityChanged();
}

bool DigiviewManager::connectToHost()
{
    _resetRemoteSession();
    return _connection->connectToEndpoint();
}

void DigiviewManager::disconnectFromHost()
{
    _resetRemoteSession();
    _connection->disconnectFromEndpoint();
}

void DigiviewManager::sendSystemStatusParameters(uint8_t status, uint8_t error, float jetson_temp)
{
    mavlink_message_t msg;
    mavlink_system_status_parameters_t payload {};

    payload.status = status;
    payload.error = error;
    payload.jetson_temp = jetson_temp;

    _encodeMessage(msg, payload, mavlink_msg_system_status_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendAIParameters(uint8_t run_ai, QString track_model_name, QString scan_model_name)
{
    mavlink_message_t msg;
    mavlink_ai_parameters_t payload {};

    payload.run_ai = run_ai;
    copyStringToCharBuf(track_model_name, payload.track_model_name, 16);
    copyStringToCharBuf(scan_model_name, payload.scan_model_name, 16);

    _encodeMessage(msg, payload, mavlink_msg_ai_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendModelParameters(QString model_name)
{
    mavlink_message_t msg;
    mavlink_model_parameters_t payload {};

    copyStringToCharBuf(model_name, payload.model_name, 16);

    _encodeMessage(msg, payload, mavlink_msg_model_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendSetVideoOutput(
    QString stream_name, uint16_t width, uint16_t height, uint8_t fps,
    uint8_t layout, uint8_t detection_overlay_mode)
{
    const uint8_t numUserViews = userViewCountForLayout(layout);

    sendVideoOutputParameters(
        stream_name,
        width,
        height,
        fps,
        layout,
        detection_overlay_mode,
        numUserViews,
        {},
        {},
        {},
        {},
        0,
        0,
        0,
        0,
        0);
    requestVideoOutputParameters();
}

void DigiviewManager::requestVideoOutputParameters()
{
    if (!_remoteIdentityValid) {
        _pendingVideoOutputParametersRequest = true;
        qCDebug(DigiviewManagerLog) << "Deferring MAVLink GET for VIDEO_OUTPUT_PARAMETERS until target HEARTBEAT";
        return;
    }

    mavlink_message_t msg;
    mavlink_command_long_t command {};

    command.target_system = _remoteSystemId;
    command.target_component = _remoteComponentId;
    command.command = MAV_CMD_SET_MESSAGE_INTERVAL;
    command.param1 = static_cast<float>(MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS);
    command.param2 = kVideoOutputParametersOneShotIntervalUs;

    _encodeMessage(msg, command, mavlink_msg_command_long_encode);
    qCDebug(DigiviewManagerLog) << "Sending one-shot MAVLink GET for VIDEO_OUTPUT_PARAMETERS:"
                                   << "command" << MAV_CMD_SET_MESSAGE_INTERVAL
                                   << "messageId" << MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS
                                   << "intervalUs" << command.param2
                                  << "senderSystem" << _senderSystemId
                                   << "senderComponent" << _senderComponentId
                                   << "targetSystem" << command.target_system
                                   << "targetComponent" << command.target_component;
    _pendingVideoOutputParametersRequest = !_sendMessage(msg);
}

void DigiviewManager::sendVideoOutputParameters(
    QString stream_name, uint16_t width, uint16_t height, uint8_t fps,
    uint8_t layout_mode, uint8_t detection_overlay_mode, uint8_t num_user_views,
    QVector<int> views_x, QVector<int> views_y, QVector<int> views_w, QVector<int> views_h,
    uint16_t detection_overlay_x, uint16_t detection_overlay_y,
    uint16_t detection_overlay_w, uint16_t detection_overlay_h,
    uint16_t single_detection_size)
{
    mavlink_message_t msg;
    mavlink_video_output_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.width = width;
    payload.height = height;
    payload.fps = fps;
    payload.layout_mode = layout_mode;
    payload.detection_overlay_mode = detection_overlay_mode;
    payload.num_user_views = num_user_views;

    for (int i = 0; i < 4; ++i) {
        payload.views_x[i] = (i < views_x.size()) ? static_cast<uint16_t>(views_x[i]) : 0;
        payload.views_y[i] = (i < views_y.size()) ? static_cast<uint16_t>(views_y[i]) : 0;
        payload.views_w[i] = (i < views_w.size()) ? static_cast<uint16_t>(views_w[i]) : 0;
        payload.views_h[i] = (i < views_h.size()) ? static_cast<uint16_t>(views_h[i]) : 0;
    }

    payload.detection_overlay_x = detection_overlay_x;
    payload.detection_overlay_y = detection_overlay_y;
    payload.detection_overlay_w = detection_overlay_w;
    payload.detection_overlay_h = detection_overlay_h;
    payload.single_detection_size = single_detection_size;

    _encodeMessage(msg, payload, mavlink_msg_video_output_parameters_encode);
    qCDebug(DigiviewManagerLog) << "Sending VIDEO_OUTPUT_PARAMETERS:"
                                << "stream" << stringFromCharBuf(payload.stream_name, 16)
                                << "output" << payload.width << "x" << payload.height
                                << "fps" << payload.fps
                                << "layoutMode" << payload.layout_mode
                                << "detectionOverlayMode" << payload.detection_overlay_mode
                                << "numUserViews" << payload.num_user_views
                                << "views"
                                << "(" << payload.views_x[0] << "," << payload.views_y[0] << ","
                                << payload.views_w[0] << "," << payload.views_h[0] << ")"
                                << "(" << payload.views_x[1] << "," << payload.views_y[1] << ","
                                << payload.views_w[1] << "," << payload.views_h[1] << ")"
                                << "(" << payload.views_x[2] << "," << payload.views_y[2] << ","
                                << payload.views_w[2] << "," << payload.views_h[2] << ")"
                                << "(" << payload.views_x[3] << "," << payload.views_y[3] << ","
                                << payload.views_w[3] << "," << payload.views_h[3] << ")"
                                << "detectionOverlay" << "(" << payload.detection_overlay_x << ","
                                << payload.detection_overlay_y << "," << payload.detection_overlay_w << ","
                                << payload.detection_overlay_h << ")"
                                << "singleDetectionSize" << payload.single_detection_size;

    
    _sendMessage(msg);
}

void DigiviewManager::sendCaptureParameters(
    QString stream_name, uint8_t cap_single_image, uint8_t record_video,
    uint16_t images_captured, uint16_t videos_captured)
{
    mavlink_message_t msg;
    mavlink_capture_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cap_single_image = cap_single_image;
    payload.record_video = record_video;
    payload.images_captured = images_captured;
    payload.videos_captured = videos_captured;

    _encodeMessage(msg, payload, mavlink_msg_capture_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendDetectionParameters(
    uint8_t mode, uint8_t sorting_mode,
    float track_confidence_threshold, float scan_confidence_threshold,
    float track_box_overlap, float scan_box_overlap,
    uint8_t creation_score_scale, uint8_t bonus_detection_scale,
    uint8_t bonus_redetection_scale, uint8_t missed_detection_penalty,
    uint8_t missed_redetection_penalty)
{
    mavlink_message_t msg;
    mavlink_detection_parameters_t payload {};

    payload.mode = mode;
    payload.sorting_mode = sorting_mode;
    payload.track_confidence_threshold = track_confidence_threshold;
    payload.scan_confidence_threshold = scan_confidence_threshold;
    payload.track_box_overlap = track_box_overlap;
    payload.scan_box_overlap = scan_box_overlap;
    payload.creation_score_scale = creation_score_scale;
    payload.bonus_detection_scale = bonus_detection_scale;
    payload.bonus_redetection_scale = bonus_redetection_scale;
    payload.missed_detection_penalty = missed_detection_penalty;
    payload.missed_redetection_penalty = missed_redetection_penalty;

    _encodeMessage(msg, payload, mavlink_msg_detection_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendTrackedDetectionParameters(
    uint8_t index, uint8_t score, uint8_t total_detections, int16_t type,
    float yaw_global, float pitch_global, uint8_t rel_frame_of_reference,
    float yaw_rel, float pitch_rel,
    float latitude, float longitude, float altitude,
    float distance, float width, float height,
    uint16_t track_id, quint64 publish_timestamp_us, uint8_t view_id)
{
    mavlink_message_t msg;
    mavlink_tracked_detection_parameters_t payload {};

    payload.index = index;
    payload.score = score;
    payload.total_detections = total_detections;
    payload.type = type;
    payload.yaw_global = yaw_global;
    payload.pitch_global = pitch_global;
    payload.rel_frame_of_reference = rel_frame_of_reference;
    payload.yaw_rel = yaw_rel;
    payload.pitch_rel = pitch_rel;
    payload.latitude = latitude;
    payload.longitude = longitude;
    payload.altitude = altitude;
    payload.distance = distance;
    payload.width = width;
    payload.height = height;
    payload.track_id = track_id;
    payload.publish_timestamp_us = static_cast<uint64_t>(publish_timestamp_us);
    payload.view_id = view_id;

    _encodeMessage(msg, payload, mavlink_msg_tracked_detection_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendCamTargetingParameters(
    QString stream_name, uint8_t cam_id, uint8_t targeting_mode, uint8_t euler_delta,
    float yaw, float pitch, float roll, uint8_t lock_flags,
    float x_offset, float y_offset,
    float target_latitude, float target_longitude, float target_altitude,
    uint16_t track_id, int16_t view_id, uint8_t lock_target)
{
    mavlink_message_t msg;
    mavlink_cam_targeting_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id = cam_id;
    payload.targeting_mode = targeting_mode;
    payload.euler_delta = euler_delta;
    payload.yaw = yaw;
    payload.pitch = pitch;
    payload.roll = roll;
    payload.lock_flags = lock_flags;
    payload.x_offset = x_offset;
    payload.y_offset = y_offset;
    payload.target_latitude = target_latitude;
    payload.target_longitude = target_longitude;
    payload.target_altitude = target_altitude;
    payload.track_id = track_id;
    payload.view_id = view_id;
    payload.lock_target = lock_target;

    _encodeMessage(msg, payload, mavlink_msg_cam_targeting_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendCamOpticsAndControlParameters(
    QString stream_name, uint8_t cam_id, int8_t zoom, float fov, uint8_t crop_mode)
{
    mavlink_message_t msg;
    mavlink_cam_optics_and_control_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id = cam_id;
    payload.zoom = zoom;
    payload.fov = fov;
    payload.crop_mode = crop_mode;

    _encodeMessage(msg, payload, mavlink_msg_cam_optics_and_control_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendCamOffsetParameters(
    QString stream_name, uint8_t cam_id,
    float x, float y,
    float yaw_global, float pitch_global, float yaw_rel, float pitch_rel)
{
    mavlink_message_t msg;
    mavlink_cam_offset_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id = cam_id;
    payload.x = x;
    payload.y = y;
    payload.yaw_global = yaw_global;
    payload.pitch_global = pitch_global;
    payload.yaw_rel = yaw_rel;
    payload.pitch_rel = pitch_rel;

    _encodeMessage(msg, payload, mavlink_msg_cam_offset_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendSensorParameters(
    uint32_t min_exposure, uint32_t max_exposure,
    uint32_t min_gain, uint32_t max_gain,
    float target_brightness)
{
    mavlink_message_t msg;
    mavlink_sensor_parameters_t payload {};

    payload.min_exposure = min_exposure;
    payload.max_exposure = max_exposure;
    payload.min_gain = min_gain;
    payload.max_gain = max_gain;
    payload.target_brightness = target_brightness;

    _encodeMessage(msg, payload, mavlink_msg_sensor_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendCamDepthEstimationParameters(
    QString stream_name, uint8_t cam_id, uint8_t depth_estimation_mode, float depth)
{
    mavlink_message_t msg;
    mavlink_cam_depth_estimation_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id = cam_id;
    payload.depth_estimation_mode = depth_estimation_mode;
    payload.depth = depth;

    _encodeMessage(msg, payload, mavlink_msg_cam_depth_estimation_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendSingleTargetTrackingParameters(
    uint8_t command, QString stream_name, uint8_t cam_id,
    float x_offset, float y_offset,
    uint8_t detection_id, uint16_t zoom_level, float confidence,
    float yaw_global, float pitch_global,
    uint8_t rel_frame_of_reference, float yaw_rel, float pitch_rel,
    quint64 publish_timestamp_us, uint8_t status, uint8_t lock_target)
{
    mavlink_message_t msg;
    mavlink_single_target_tracking_parameters_t payload {};

    payload.command = command;
    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id = cam_id;
    payload.x_offset = x_offset;
    payload.y_offset = y_offset;
    payload.detection_id = detection_id;
    payload.zoom_level = zoom_level;
    payload.confidence = confidence;
    payload.yaw_global = yaw_global;
    payload.pitch_global = pitch_global;
    payload.rel_frame_of_reference = rel_frame_of_reference;
    payload.yaw_rel = yaw_rel;
    payload.pitch_rel = pitch_rel;
    payload.publish_timestamp_us = static_cast<uint64_t>(publish_timestamp_us);
    payload.status = status;
    payload.lock_target = lock_target;

    _encodeMessage(msg, payload, mavlink_msg_single_target_tracking_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendCalibrationParameters(uint8_t cam_id, uint8_t calib_command, uint8_t calib_status)
{
    mavlink_message_t msg;
    mavlink_calibration_parameters_t payload {};

    payload.cam_id = cam_id;
    payload.calib_command = calib_command;
    payload.calib_status = calib_status;

    _encodeMessage(msg, payload, mavlink_msg_calibration_parameters_encode);
    _sendMessage(msg);
}

void DigiviewManager::sendNavigationParameters(
    float altitude, float visual_lat, float visual_lon,
    float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
    float visual_vel_x, float visual_vel_y, float visual_vel_z)
{
    mavlink_message_t msg;
    mavlink_navigation_parameters_t payload {};

    payload.altitude = altitude;
    payload.visual_lat = visual_lat;
    payload.visual_lon = visual_lon;
    payload.next_waypoint_target_yaw = next_waypoint_target_yaw;
    payload.next_waypoint_target_pitch = next_waypoint_target_pitch;
    payload.next_waypoint_target_roll = next_waypoint_target_roll;
    payload.visual_vel_x = visual_vel_x;
    payload.visual_vel_y = visual_vel_y;
    payload.visual_vel_z = visual_vel_z;

    _encodeMessage(msg, payload, mavlink_msg_navigation_parameters_encode);
    _sendMessage(msg);
}

////////////// HELPERS ///////////////////////////

void DigiviewManager::changeEuler(int camId, float yaw, float pitch)
{
    /*
    sendSingleTargetTrackingParameters(
        SV_STT_CMD_OFF,
        _streamName,
        camId,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        SV_STT_STATUS_OFF,
        0
    );
    */

    sendCamTargetingParameters(
        _streamName,
        camId,
        SV_TARGETING_MODE_DIRECTIONAL,
        1,
        yaw,
        pitch,
        0,
        kCamTargetingLockFlagsUnchanged,
        0, 0,
        0, 0, 0,
        0,
        -1,
        0
    );
}

void DigiviewManager::changeZoom(int camId, float zoom)
{
    sendCamOpticsAndControlParameters(
        _streamName,
        camId,
        zoom,
        0,
        0
    );
}

void DigiviewManager::_handleMessage(const mavlink_message_t& message)
{
    const mavlink_message_info_t* const messageInfo = mavlink_get_message_info_by_id(message.msgid);
    const char* const messageName = messageInfo ? messageInfo->name : "UNKNOWN";

    qCDebug(DigiviewManagerLog) << "Received Digiview MAVLink message:"
                                << "msgid" << message.msgid
                                << "name" << messageName
                                << "senderSystem" << message.sysid
                                << "senderComponent" << message.compid
                                << "payloadLength" << message.len
                                << "sequence" << message.seq;

    if (_lastReceivedMessageId != message.msgid) {
        _lastReceivedMessageId = message.msgid;
        emit lastReceivedMessageIdChanged();
    }

    if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        if (!_remoteIdentityValid) {
            if ((message.sysid == 0) || (message.compid == MAV_COMP_ID_ALL)) {
                qCWarning(DigiviewManagerLog) << "Ignoring Digiview HEARTBEAT with invalid target identity"
                                              << "senderSystem" << message.sysid
                                              << "senderComponent" << message.compid;
            } else {
                _establishRemoteSession(message.sysid, message.compid);
            }
        } else if ((message.sysid != _remoteSystemId) || (message.compid != _remoteComponentId)) {
            qCWarning(DigiviewManagerLog) << "Ignoring Digiview HEARTBEAT from different MAVLink identity"
                                          << "senderSystem" << message.sysid
                                          << "senderComponent" << message.compid;
        } else {
            _establishRemoteSession(message.sysid, message.compid);
        }
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_COMMAND_ACK: {
        mavlink_command_ack_t ack;
        mavlink_msg_command_ack_decode(&message, &ack);

        qCDebug(DigiviewManagerLog) << "Received MAVLink COMMAND_ACK:"
                                     << "senderSystem" << message.sysid
                                     << "senderComponent" << message.compid
                                     << "command" << ack.command
                                     << "result" << QGCMAVLink::mavResultToString(ack.result)
                                     << "progress" << ack.progress
                                     << "resultParam2" << ack.result_param2
                                     << "targetSystem" << ack.target_system
                                     << "targetComponent" << ack.target_component;
        break;
    }
    case MAVLINK_MSG_ID_SYSTEM_STATUS_PARAMETERS: {
        mavlink_system_status_parameters_t payload;
        mavlink_msg_system_status_parameters_decode(&message, &payload);
        emit systemStatusParametersReceived(payload.status, payload.error, payload.jetson_temp);
        break;
    }
    case MAVLINK_MSG_ID_AI_PARAMETERS: {
        mavlink_ai_parameters_t payload;
        mavlink_msg_ai_parameters_decode(&message, &payload);
        emit aiParametersReceived(payload.run_ai,
                                  stringFromCharBuf(payload.track_model_name, 16),
                                  stringFromCharBuf(payload.scan_model_name, 16));
        break;
    }
    case MAVLINK_MSG_ID_MODEL_PARAMETERS: {
        mavlink_model_parameters_t payload;
        mavlink_msg_model_parameters_decode(&message, &payload);
        emit modelParametersReceived(stringFromCharBuf(payload.model_name, 16));
        break;
    }
    case MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS: {
        if (!_remoteIdentityValid) {
            qCDebug(DigiviewManagerLog) << "Ignoring VIDEO_OUTPUT_PARAMETERS before target HEARTBEAT"
                                        << "senderSystem" << message.sysid
                                        << "senderComponent" << message.compid;
            break;
        }

        if ((message.sysid != _remoteSystemId) || (message.compid != _remoteComponentId)) {
            qCWarning(DigiviewManagerLog) << "Ignoring VIDEO_OUTPUT_PARAMETERS from different MAVLink identity"
                                          << "senderSystem" << message.sysid
                                          << "senderComponent" << message.compid;
            break;
        }

        mavlink_video_output_parameters_t payload;
        mavlink_msg_video_output_parameters_decode(&message, &payload);

        const QString streamName = stringFromCharBuf(payload.stream_name, 16);
        if (streamName != _streamName) {
            qCDebug(DigiviewManagerLog) << "Ignoring VIDEO_OUTPUT_PARAMETERS for unexpected stream"
                                        << "stream" << streamName
                                        << "expected" << _streamName;
            break;
        }

        qCDebug(DigiviewManagerLog) << "Received VIDEO_OUTPUT_PARAMETERS:"
                                    << "stream" << streamName
                                    << "output" << payload.width << "x" << payload.height
                                    << "fps" << payload.fps
                                    << "layoutMode" << payload.layout_mode
                                    << "detectionOverlayMode" << payload.detection_overlay_mode
                                    << "numUserViews" << payload.num_user_views
                                    << "views"
                                    << "(" << payload.views_x[0] << "," << payload.views_y[0] << ","
                                    << payload.views_w[0] << "," << payload.views_h[0] << ")"
                                    << "(" << payload.views_x[1] << "," << payload.views_y[1] << ","
                                    << payload.views_w[1] << "," << payload.views_h[1] << ")"
                                    << "(" << payload.views_x[2] << "," << payload.views_y[2] << ","
                                    << payload.views_w[2] << "," << payload.views_h[2] << ")"
                                    << "(" << payload.views_x[3] << "," << payload.views_y[3] << ","
                                    << payload.views_w[3] << "," << payload.views_h[3] << ")"
                                    << "detectionOverlay" << "(" << payload.detection_overlay_x << ","
                                    << payload.detection_overlay_y << "," << payload.detection_overlay_w << ","
                                    << payload.detection_overlay_h << ")"
                                    << "singleDetectionSize" << payload.single_detection_size;
        QVector<int> viewsX;
        QVector<int> viewsY;
        QVector<int> viewsW;
        QVector<int> viewsH;
        QVariantList views;
        viewsX.reserve(4);
        viewsY.reserve(4);
        viewsW.reserve(4);
        viewsH.reserve(4);
        views.reserve(4);

        for (int i = 0; i < 4; ++i) {
            viewsX.append(payload.views_x[i]);
            viewsY.append(payload.views_y[i]);
            viewsW.append(payload.views_w[i]);
            viewsH.append(payload.views_h[i]);

            QVariantMap view;
            view.insert(QStringLiteral("x"), payload.views_x[i]);
            view.insert(QStringLiteral("y"), payload.views_y[i]);
            view.insert(QStringLiteral("width"), payload.views_w[i]);
            view.insert(QStringLiteral("height"), payload.views_h[i]);
            views.append(view);
        }

        QVariantMap detectionOverlayRect;
        detectionOverlayRect.insert(QStringLiteral("x"), payload.detection_overlay_x);
        detectionOverlayRect.insert(QStringLiteral("y"), payload.detection_overlay_y);
        detectionOverlayRect.insert(QStringLiteral("width"), payload.detection_overlay_w);
        detectionOverlayRect.insert(QStringLiteral("height"), payload.detection_overlay_h);

        const int width = payload.width;
        const int height = payload.height;
        const int fps = payload.fps;
        const int layoutMode = payload.layout_mode;
        const int detectionOverlayMode = payload.detection_overlay_mode;
        const int numUserViews = payload.num_user_views;
        const int singleDetectionSize = payload.single_detection_size;

        const bool hasVideoOutputParametersChangedValue = !_hasVideoOutputParameters;
        const bool videoOutputStreamNameChangedValue = _videoOutputStreamName != streamName;
        const bool videoOutputWidthChangedValue = _videoOutputWidth != width;
        const bool videoOutputHeightChangedValue = _videoOutputHeight != height;
        const bool videoOutputFpsChangedValue = _videoOutputFps != fps;
        const bool videoOutputLayoutModeChangedValue = _videoOutputLayoutMode != layoutMode;
        const bool videoOutputDetectionOverlayModeChangedValue =
            _videoOutputDetectionOverlayMode != detectionOverlayMode;
        const bool videoOutputNumUserViewsChangedValue = _videoOutputNumUserViews != numUserViews;
        const bool videoOutputViewsChangedValue = _videoOutputViews != views;
        const bool videoOutputDetectionOverlayRectChangedValue =
            _videoOutputDetectionOverlayRect != detectionOverlayRect;
        const bool videoOutputSingleDetectionSizeChangedValue = _videoOutputSingleDetectionSize != singleDetectionSize;

        _hasVideoOutputParameters = true;
        _videoOutputStreamName = streamName;
        _videoOutputWidth = width;
        _videoOutputHeight = height;
        _videoOutputFps = fps;
        _videoOutputLayoutMode = layoutMode;
        _videoOutputDetectionOverlayMode = detectionOverlayMode;
        _videoOutputNumUserViews = numUserViews;
        _videoOutputViews = views;
        _videoOutputDetectionOverlayRect = detectionOverlayRect;
        _videoOutputSingleDetectionSize = singleDetectionSize;

        if (hasVideoOutputParametersChangedValue) {
            emit hasVideoOutputParametersChanged();
        }
        if (videoOutputStreamNameChangedValue) {
            emit videoOutputStreamNameChanged();
        }
        if (videoOutputWidthChangedValue) {
            emit videoOutputWidthChanged();
        }
        if (videoOutputHeightChangedValue) {
            emit videoOutputHeightChanged();
        }
        if (videoOutputFpsChangedValue) {
            emit videoOutputFpsChanged();
        }
        if (videoOutputLayoutModeChangedValue) {
            emit videoOutputLayoutModeChanged();
        }
        if (videoOutputDetectionOverlayModeChangedValue) {
            emit videoOutputDetectionOverlayModeChanged();
        }
        if (videoOutputNumUserViewsChangedValue) {
            emit videoOutputNumUserViewsChanged();
        }
        if (videoOutputViewsChangedValue) {
            emit videoOutputViewsChanged();
        }
        if (videoOutputDetectionOverlayRectChangedValue) {
            emit videoOutputDetectionOverlayRectChanged();
        }
        if (videoOutputSingleDetectionSizeChangedValue) {
            emit videoOutputSingleDetectionSizeChanged();
        }

        emit videoOutputParametersReceived(
            streamName,
            payload.width,
            payload.height,
            payload.fps,
            payload.layout_mode,
            payload.detection_overlay_mode,
            payload.num_user_views,
            viewsX,
            viewsY,
            viewsW,
            viewsH,
            payload.detection_overlay_x,
            payload.detection_overlay_y,
            payload.detection_overlay_w,
            payload.detection_overlay_h,
            payload.single_detection_size);
        break;
    }
    case MAVLINK_MSG_ID_CAPTURE_PARAMETERS: {
        mavlink_capture_parameters_t payload;
        mavlink_msg_capture_parameters_decode(&message, &payload);
        emit captureParametersReceived(
            stringFromCharBuf(payload.stream_name, 16),
            payload.cap_single_image,
            payload.record_video,
            payload.images_captured,
            payload.videos_captured);
        break;
    }
    case MAVLINK_MSG_ID_DETECTION_PARAMETERS: {
        mavlink_detection_parameters_t payload;
        mavlink_msg_detection_parameters_decode(&message, &payload);
        emit detectionParametersReceived(
            payload.mode,
            payload.sorting_mode,
            payload.track_confidence_threshold,
            payload.scan_confidence_threshold,
            payload.track_box_overlap,
            payload.scan_box_overlap,
            payload.creation_score_scale,
            payload.bonus_detection_scale,
            payload.bonus_redetection_scale,
            payload.missed_detection_penalty,
            payload.missed_redetection_penalty);
        break;
    }
    case MAVLINK_MSG_ID_TRACKED_DETECTION_PARAMETERS: {
        mavlink_tracked_detection_parameters_t payload;
        mavlink_msg_tracked_detection_parameters_decode(&message, &payload);
        emit trackedDetectionParametersReceived(
            payload.index,
            payload.score,
            payload.total_detections,
            payload.type,
            payload.yaw_global,
            payload.pitch_global,
            payload.rel_frame_of_reference,
            payload.yaw_rel,
            payload.pitch_rel,
            payload.latitude,
            payload.longitude,
            payload.altitude,
            payload.distance,
            payload.width,
            payload.height,
            payload.track_id,
            static_cast<quint64>(payload.publish_timestamp_us),
            payload.view_id);
        break;
    }
    case MAVLINK_MSG_ID_CAM_TARGETING_PARAMETERS: {
        mavlink_cam_targeting_parameters_t payload;
        mavlink_msg_cam_targeting_parameters_decode(&message, &payload);
        emit camTargetingParametersReceived(
            stringFromCharBuf(payload.stream_name, 16),
            payload.cam_id,
            payload.targeting_mode,
            payload.euler_delta,
            payload.yaw,
            payload.pitch,
            payload.roll,
            payload.lock_flags,
            payload.x_offset,
            payload.y_offset,
            payload.target_latitude,
            payload.target_longitude,
            payload.target_altitude,
            payload.track_id,
            payload.view_id,
            payload.lock_target);
        break;
    }
    case MAVLINK_MSG_ID_CAM_OPTICS_AND_CONTROL_PARAMETERS: {
        mavlink_cam_optics_and_control_parameters_t payload;
        mavlink_msg_cam_optics_and_control_parameters_decode(&message, &payload);
        emit camOpticsAndControlParametersReceived(
            stringFromCharBuf(payload.stream_name, 16),
            payload.cam_id,
            payload.zoom,
            payload.fov,
            payload.crop_mode);
        break;
    }
    case MAVLINK_MSG_ID_CAM_OFFSET_PARAMETERS: {
        mavlink_cam_offset_parameters_t payload;
        mavlink_msg_cam_offset_parameters_decode(&message, &payload);
        emit camOffsetParametersReceived(
            stringFromCharBuf(payload.stream_name, 16),
            payload.cam_id,
            payload.x,
            payload.y,
            payload.yaw_global,
            payload.pitch_global,
            payload.yaw_rel,
            payload.pitch_rel);
        break;
    }
    case MAVLINK_MSG_ID_SENSOR_PARAMETERS: {
        mavlink_sensor_parameters_t payload;
        mavlink_msg_sensor_parameters_decode(&message, &payload);
        emit sensorParametersReceived(
            payload.min_exposure,
            payload.max_exposure,
            payload.min_gain,
            payload.max_gain,
            payload.target_brightness);
        break;
    }
    case MAVLINK_MSG_ID_CAM_DEPTH_ESTIMATION_PARAMETERS: {
        mavlink_cam_depth_estimation_parameters_t payload;
        mavlink_msg_cam_depth_estimation_parameters_decode(&message, &payload);
        emit camDepthEstimationParametersReceived(
            stringFromCharBuf(payload.stream_name, 16),
            payload.cam_id,
            payload.depth_estimation_mode,
            payload.depth);
        break;
    }
    case MAVLINK_MSG_ID_SINGLE_TARGET_TRACKING_PARAMETERS: {
        mavlink_single_target_tracking_parameters_t payload;
        mavlink_msg_single_target_tracking_parameters_decode(&message, &payload);
        emit singleTargetTrackingParametersReceived(
            payload.command,
            stringFromCharBuf(payload.stream_name, 16),
            payload.cam_id,
            payload.x_offset,
            payload.y_offset,
            payload.detection_id,
            payload.zoom_level,
            payload.confidence,
            payload.yaw_global,
            payload.pitch_global,
            payload.rel_frame_of_reference,
            payload.yaw_rel,
            payload.pitch_rel,
            static_cast<quint64>(payload.publish_timestamp_us),
            payload.status,
            payload.lock_target);
        break;
    }
    case MAVLINK_MSG_ID_CALIBRATION_PARAMETERS: {
        mavlink_calibration_parameters_t payload;
        mavlink_msg_calibration_parameters_decode(&message, &payload);
        emit calibrationParametersReceived(payload.cam_id, payload.calib_command, payload.calib_status);
        break;
    }
    case MAVLINK_MSG_ID_NAVIGATION_PARAMETERS: {
        mavlink_navigation_parameters_t payload;
        mavlink_msg_navigation_parameters_decode(&message, &payload);
        emit navigationParametersReceived(
            payload.altitude,
            payload.visual_lat,
            payload.visual_lon,
            payload.next_waypoint_target_yaw,
            payload.next_waypoint_target_pitch,
            payload.next_waypoint_target_roll,
            payload.visual_vel_x,
            payload.visual_vel_y,
            payload.visual_vel_z);
        break;
    }
    default:
        qCDebug(DigiviewManagerLog) << "Unhandled Digiview MAVLink message:"
                                    << "msgid" << message.msgid
                                    << "name" << messageName
                                    << "senderSystem" << message.sysid
                                    << "senderComponent" << message.compid
                                    << "payloadLength" << message.len
                                    << "sequence" << message.seq;
        break;
    }

    emit messageDecoded(message.msgid);
}

bool DigiviewManager::_sendMessage(const mavlink_message_t& message)
{
    const bool sent = _connection->sendMessage(message);
    if (!sent) {
        qCWarning(DigiviewManagerLog) << "Failed to send Digiview MAVLink message" << message.msgid << _connection->lastError();
    }

    return sent;
}

void DigiviewManager::_establishRemoteSession(uint8_t systemId, uint8_t componentId)
{
    const bool initialSubscription = !_remoteIdentityValid;

    _remoteSystemId = systemId;
    _remoteComponentId = componentId;
    _remoteIdentityValid = true;

    mavlink_message_t msg;
    mavlink_command_long_t command {};

    command.target_system = _remoteSystemId;
    command.target_component = _remoteComponentId;
    command.command = MAV_CMD_SET_MESSAGE_INTERVAL;
    command.param1 = static_cast<float>(MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS);
    command.param2 = kVideoOutputParametersSubscriptionIntervalUs;

    _encodeMessage(msg, command, mavlink_msg_command_long_encode);
    if (initialSubscription) {
        qCDebug(DigiviewManagerLog) << "Starting recurring MAVLink VIDEO_OUTPUT_PARAMETERS subscription:"
                                     << "command" << MAV_CMD_SET_MESSAGE_INTERVAL
                                     << "messageId" << MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS
                                     << "intervalUs" << command.param2
                                     << "senderSystem" << _senderSystemId
                                     << "senderComponent" << _senderComponentId
                                     << "targetSystem" << command.target_system
                                     << "targetComponent" << command.target_component;
    } else {
        qCDebug(DigiviewManagerLog) << "Renewing recurring MAVLink VIDEO_OUTPUT_PARAMETERS subscription after target HEARTBEAT:"
                                     << "command" << MAV_CMD_SET_MESSAGE_INTERVAL
                                     << "messageId" << MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS
                                     << "intervalUs" << command.param2
                                     << "senderSystem" << _senderSystemId
                                     << "senderComponent" << _senderComponentId
                                     << "targetSystem" << command.target_system
                                     << "targetComponent" << command.target_component;
    }
    _videoOutputParametersSubscriptionActive = _sendMessage(msg);

    if (_pendingVideoOutputParametersRequest) {
        requestVideoOutputParameters();
    }
}

void DigiviewManager::_resetRemoteSession()
{
    _remoteSystemId = 0;
    _remoteComponentId = 0;
    _remoteIdentityValid = false;
    _pendingVideoOutputParametersRequest = false;
    _videoOutputParametersSubscriptionActive = false;

    const bool hasVideoOutputParametersChangedValue = _hasVideoOutputParameters;
    const bool videoOutputStreamNameChangedValue = !_videoOutputStreamName.isEmpty();
    const bool videoOutputWidthChangedValue = _videoOutputWidth != 0;
    const bool videoOutputHeightChangedValue = _videoOutputHeight != 0;
    const bool videoOutputFpsChangedValue = _videoOutputFps != 0;
    const bool videoOutputLayoutModeChangedValue = _videoOutputLayoutMode != 0;
    const bool videoOutputDetectionOverlayModeChangedValue = _videoOutputDetectionOverlayMode != 0;
    const bool videoOutputNumUserViewsChangedValue = _videoOutputNumUserViews != 0;
    const bool videoOutputViewsChangedValue = !_videoOutputViews.isEmpty();
    const bool videoOutputDetectionOverlayRectChangedValue = !_videoOutputDetectionOverlayRect.isEmpty();
    const bool videoOutputSingleDetectionSizeChangedValue = _videoOutputSingleDetectionSize != 0;

    _hasVideoOutputParameters = false;
    _videoOutputStreamName.clear();
    _videoOutputWidth = 0;
    _videoOutputHeight = 0;
    _videoOutputFps = 0;
    _videoOutputLayoutMode = 0;
    _videoOutputDetectionOverlayMode = 0;
    _videoOutputNumUserViews = 0;
    _videoOutputViews.clear();
    _videoOutputDetectionOverlayRect.clear();
    _videoOutputSingleDetectionSize = 0;

    if (hasVideoOutputParametersChangedValue) {
        emit hasVideoOutputParametersChanged();
    }
    if (videoOutputStreamNameChangedValue) {
        emit videoOutputStreamNameChanged();
    }
    if (videoOutputWidthChangedValue) {
        emit videoOutputWidthChanged();
    }
    if (videoOutputHeightChangedValue) {
        emit videoOutputHeightChanged();
    }
    if (videoOutputFpsChangedValue) {
        emit videoOutputFpsChanged();
    }
    if (videoOutputLayoutModeChangedValue) {
        emit videoOutputLayoutModeChanged();
    }
    if (videoOutputDetectionOverlayModeChangedValue) {
        emit videoOutputDetectionOverlayModeChanged();
    }
    if (videoOutputNumUserViewsChangedValue) {
        emit videoOutputNumUserViewsChanged();
    }
    if (videoOutputViewsChangedValue) {
        emit videoOutputViewsChanged();
    }
    if (videoOutputDetectionOverlayRectChangedValue) {
        emit videoOutputDetectionOverlayRectChanged();
    }
    if (videoOutputSingleDetectionSizeChangedValue) {
        emit videoOutputSingleDetectionSizeChanged();
    }
}

void DigiviewManager::_resetRemoteSessionForSenderIdentityChange()
{
    const bool reconnect = _connection->connected() && _remoteIdentityValid;
    const uint8_t remoteSystemId = _remoteSystemId;
    const uint8_t remoteComponentId = _remoteComponentId;
    const bool pendingVideoOutputParametersRequest = _pendingVideoOutputParametersRequest;

    _resetRemoteSession();
    _pendingVideoOutputParametersRequest = pendingVideoOutputParametersRequest;

    if (reconnect) {
        _establishRemoteSession(remoteSystemId, remoteComponentId);
    }
}
