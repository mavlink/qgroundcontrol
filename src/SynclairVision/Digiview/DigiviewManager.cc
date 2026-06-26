#include "DigiviewManager.h"

#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "sv_mavlink_dialect/sv_mavlink_dialect.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <cstring>
#include <limits>

QGC_LOGGING_CATEGORY(DigiviewManagerLog, "Digiview.Manager")

Q_APPLICATION_STATIC(DigiviewManager, _digiviewManagerInstance);

namespace {

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

} // namespace

DigiviewManager* DigiviewManager::instance()
{
    return _digiviewManagerInstance();
}

DigiviewManager::DigiviewManager(QObject* parent)
    : QObject(parent)
    , _connection(new DigiviewConnection(this))
    , _senderSystemId(252)
    , _senderComponentId(69)
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

    QTimer::singleShot(0, this, &DigiviewManager::connectToHost);
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
    _connection->setHost(host);
}

void DigiviewManager::setPort(quint16 port)
{
    _connection->setPort(port);
}

void DigiviewManager::setListenPort(quint16 listenPort)
{
    _connection->setListenPort(listenPort);
}

void DigiviewManager::setSenderSystemId(int senderSystemId)
{
    if ((senderSystemId < 0) || (senderSystemId > std::numeric_limits<uint8_t>::max())) {
        qCWarning(DigiviewManagerLog) << "Ignoring invalid Digiview sender system id" << senderSystemId;
        return;
    }

    const uint8_t senderId = static_cast<uint8_t>(senderSystemId);
    if (senderId == _senderSystemId) {
        return;
    }

    _senderSystemId = senderId;
    emit senderIdentityChanged();
}

void DigiviewManager::setSenderComponentId(int senderComponentId)
{
    if ((senderComponentId < 0) || (senderComponentId > std::numeric_limits<uint8_t>::max())) {
        qCWarning(DigiviewManagerLog) << "Ignoring invalid Digiview sender component id" << senderComponentId;
        return;
    }

    const uint8_t componentId = static_cast<uint8_t>(senderComponentId);
    if (componentId == _senderComponentId) {
        return;
    }

    _senderComponentId = componentId;
    emit senderIdentityChanged();
}

bool DigiviewManager::connectToHost()
{
    return _connection->connectToEndpoint();
}

void DigiviewManager::disconnectFromHost()
{
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
    uint16_t track_id, quint64 publish_timestamp_us)
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
    quint64 publish_timestamp_us, uint8_t status)
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

void DigiviewManager::_handleMessage(const mavlink_message_t& message)
{
    if (_lastReceivedMessageId != message.msgid) {
        _lastReceivedMessageId = message.msgid;
        emit lastReceivedMessageIdChanged();
    }

    switch (message.msgid) {
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
        mavlink_video_output_parameters_t payload;
        mavlink_msg_video_output_parameters_decode(&message, &payload);

        QVector<int> viewsX;
        QVector<int> viewsY;
        QVector<int> viewsW;
        QVector<int> viewsH;
        viewsX.reserve(4);
        viewsY.reserve(4);
        viewsW.reserve(4);
        viewsH.reserve(4);

        for (int i = 0; i < 4; ++i) {
            viewsX.append(payload.views_x[i]);
            viewsY.append(payload.views_y[i]);
            viewsW.append(payload.views_w[i]);
            viewsH.append(payload.views_h[i]);
        }

        emit videoOutputParametersReceived(
            stringFromCharBuf(payload.stream_name, 16),
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
            static_cast<quint64>(payload.publish_timestamp_us));
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
            payload.status);
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
        qCDebug(DigiviewManagerLog) << "Unhandled Digiview MAVLink message" << message.msgid;
        break;
    }

    emit messageDecoded(message.msgid);
}

void DigiviewManager::_sendMessage(const mavlink_message_t& message)
{
    if (!_connection->sendMessage(message)) {
        qCWarning(DigiviewManagerLog) << "Failed to send Digiview MAVLink message" << message.msgid << _connection->lastError();
    }
}
