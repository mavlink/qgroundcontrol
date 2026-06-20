#include "SvMavlinkHandler.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "LinkInterface.h"
#include "sv_mavlink_dialect/sv_mavlink_dialect.h"
#include <QMetaObject>
#include <QVariant>
#include <QDebug>

SvMavlinkHandler::SvMavlinkHandler(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
}

void SvMavlinkHandler::registerCameraItem(QQuickItem* svCamItem)
{
    _svCamItem = svCamItem;
    qDebug() << "SvMavlinkHandler: Registered QML camera item successfully.";
}

void SvMavlinkHandler::copyStringToCharBuf(const QString& src, char* dest, int size)
{
    memset(dest, 0, size);
    QByteArray ba = src.toUtf8();
    strncpy(dest, ba.constData(), size - 1);
}

// Helper: send a message on the vehicle's primary link.
// Mirrors the pattern used throughout QGC (PlanManager, ParameterManager, etc.).
void SvMavlinkHandler::_sendMessage(mavlink_message_t& msg)
{
    if (!_vehicle) return;

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qWarning() << "SvMavlinkHandler: no primary link available, message dropped";
        return;
    }

    _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

// --------------------------------------------------------------------------
// OUTGOING PIPELINE  (called from QML to send data TO the drone)
// --------------------------------------------------------------------------

void SvMavlinkHandler::sendSystemStatusParameters(uint8_t status, uint8_t error, float jetson_temp)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_system_status_parameters_t payload{};

    payload.status      = status;
    payload.error       = error;
    payload.jetson_temp = jetson_temp;

    mavlink_msg_system_status_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendAIParameters(uint8_t run_ai, QString track_model_name, QString scan_model_name)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_ai_parameters_t payload{};

    payload.run_ai = run_ai;
    copyStringToCharBuf(track_model_name, payload.track_model_name, 16);
    copyStringToCharBuf(scan_model_name,  payload.scan_model_name,  16);

    mavlink_msg_ai_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendModelParameters(QString model_name)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_model_parameters_t payload{};

    copyStringToCharBuf(model_name, payload.model_name, 16);

    mavlink_msg_model_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendVideoOutputParameters(
    QString stream_name, uint16_t width, uint16_t height, uint8_t fps,
    uint8_t layout_mode, uint8_t detection_overlay_mode, uint8_t num_user_views,
    QVector<int> views_x, QVector<int> views_y, QVector<int> views_w, QVector<int> views_h,
    uint16_t detection_overlay_x, uint16_t detection_overlay_y,
    uint16_t detection_overlay_w, uint16_t detection_overlay_h,
    uint16_t single_detection_size)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_video_output_parameters_t payload{};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.width                  = width;
    payload.height                 = height;
    payload.fps                    = fps;
    payload.layout_mode            = layout_mode;
    payload.detection_overlay_mode = detection_overlay_mode;
    payload.num_user_views         = num_user_views;

    for (int i = 0; i < 4; ++i) {
        payload.views_x[i] = (i < views_x.size()) ? static_cast<uint16_t>(views_x[i]) : 0;
        payload.views_y[i] = (i < views_y.size()) ? static_cast<uint16_t>(views_y[i]) : 0;
        payload.views_w[i] = (i < views_w.size()) ? static_cast<uint16_t>(views_w[i]) : 0;
        payload.views_h[i] = (i < views_h.size()) ? static_cast<uint16_t>(views_h[i]) : 0;
    }

    payload.detection_overlay_x    = detection_overlay_x;
    payload.detection_overlay_y    = detection_overlay_y;
    payload.detection_overlay_w    = detection_overlay_w;
    payload.detection_overlay_h    = detection_overlay_h;
    payload.single_detection_size  = single_detection_size;

    mavlink_msg_video_output_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendCaptureParameters(
    QString stream_name, uint8_t cap_single_image, uint8_t record_video,
    uint16_t images_captured, uint16_t videos_captured)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_capture_parameters_t payload{};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cap_single_image = cap_single_image;
    payload.record_video     = record_video;
    payload.images_captured  = images_captured;
    payload.videos_captured  = videos_captured;

    mavlink_msg_capture_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendDetectionParameters(
    uint8_t mode, uint8_t sorting_mode,
    float track_confidence_threshold, float scan_confidence_threshold,
    float track_box_overlap, float scan_box_overlap,
    uint8_t creation_score_scale, uint8_t bonus_detection_scale,
    uint8_t bonus_redetection_scale, uint8_t missed_detection_penalty,
    uint8_t missed_redetection_penalty)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_detection_parameters_t payload{};

    payload.mode                        = mode;
    payload.sorting_mode                = sorting_mode;
    payload.track_confidence_threshold  = track_confidence_threshold;
    payload.scan_confidence_threshold   = scan_confidence_threshold;
    payload.track_box_overlap           = track_box_overlap;
    payload.scan_box_overlap            = scan_box_overlap;
    payload.creation_score_scale        = creation_score_scale;
    payload.bonus_detection_scale       = bonus_detection_scale;
    payload.bonus_redetection_scale     = bonus_redetection_scale;
    payload.missed_detection_penalty    = missed_detection_penalty;
    payload.missed_redetection_penalty  = missed_redetection_penalty;

    mavlink_msg_detection_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendTrackedDetectionParameters(
    uint8_t index, uint8_t score, uint8_t total_detections, int16_t type,
    float yaw_global, float pitch_global, uint8_t rel_frame_of_reference,
    float yaw_rel, float pitch_rel,
    float latitude, float longitude, float altitude,
    float distance, float width, float height,
    uint16_t track_id, quint64 publish_timestamp_us)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_tracked_detection_parameters_t payload{};

    payload.index                  = index;
    payload.score                  = score;
    payload.total_detections       = total_detections;
    payload.type                   = type;
    payload.yaw_global             = yaw_global;
    payload.pitch_global           = pitch_global;
    payload.rel_frame_of_reference = rel_frame_of_reference;
    payload.yaw_rel                = yaw_rel;
    payload.pitch_rel              = pitch_rel;
    payload.latitude               = latitude;
    payload.longitude              = longitude;
    payload.altitude               = altitude;
    payload.distance               = distance;
    payload.width                  = width;
    payload.height                 = height;
    payload.track_id               = track_id;
    payload.publish_timestamp_us   = static_cast<uint64_t>(publish_timestamp_us);

    mavlink_msg_tracked_detection_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendCamTargetingParameters(
    QString stream_name, uint8_t cam_id, uint8_t targeting_mode, uint8_t euler_delta,
    float yaw, float pitch, float roll, uint8_t lock_flags,
    float x_offset, float y_offset,
    float target_latitude, float target_longitude, float target_altitude,
    int16_t detection_id)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_cam_targeting_parameters_t payload{};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id           = cam_id;
    payload.targeting_mode   = targeting_mode;
    payload.euler_delta      = euler_delta;
    payload.yaw              = yaw;
    payload.pitch            = pitch;
    payload.roll             = roll;
    payload.lock_flags       = lock_flags;
    payload.x_offset         = x_offset;
    payload.y_offset         = y_offset;
    payload.target_latitude  = target_latitude;
    payload.target_longitude = target_longitude;
    payload.target_altitude  = target_altitude;
    payload.detection_id     = detection_id;

    mavlink_msg_cam_targeting_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendCamOpticsAndControlParameters(
    QString stream_name, uint8_t cam_id, int8_t zoom, float fov, uint8_t crop_mode)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_cam_optics_and_control_parameters_t payload{};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id    = cam_id;
    payload.zoom      = zoom;
    payload.fov       = fov;
    payload.crop_mode = crop_mode;

    mavlink_msg_cam_optics_and_control_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendCamOffsetParameters(
    QString stream_name, uint8_t cam_id,
    float x, float y,
    float yaw_global, float pitch_global, float yaw_rel, float pitch_rel)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_cam_offset_parameters_t payload{};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id      = cam_id;
    payload.x           = x;
    payload.y           = y;
    payload.yaw_global  = yaw_global;
    payload.pitch_global = pitch_global;
    payload.yaw_rel     = yaw_rel;
    payload.pitch_rel   = pitch_rel;

    mavlink_msg_cam_offset_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendSensorParameters(
    uint32_t min_exposure, uint32_t max_exposure,
    uint32_t min_gain, uint32_t max_gain,
    float target_brightness)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_sensor_parameters_t payload{};

    payload.min_exposure      = min_exposure;
    payload.max_exposure      = max_exposure;
    payload.min_gain          = min_gain;
    payload.max_gain          = max_gain;
    payload.target_brightness = target_brightness;

    mavlink_msg_sensor_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendCamDepthEstimationParameters(
    QString stream_name, uint8_t cam_id, uint8_t depth_estimation_mode, float depth)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_cam_depth_estimation_parameters_t payload{};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id                = cam_id;
    payload.depth_estimation_mode = depth_estimation_mode;
    payload.depth                 = depth;

    mavlink_msg_cam_depth_estimation_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendSingleTargetTrackingParameters(
    uint8_t command, QString stream_name, uint8_t cam_id,
    float x_offset, float y_offset,
    uint8_t detection_id, uint16_t zoom_level, float confidence,
    float yaw_global, float pitch_global,
    uint8_t rel_frame_of_reference, float yaw_rel, float pitch_rel,
    quint64 publish_timestamp_us, uint8_t status)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_single_target_tracking_parameters_t payload{};

    payload.command                = command;
    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id                 = cam_id;
    payload.x_offset               = x_offset;
    payload.y_offset               = y_offset;
    payload.detection_id           = detection_id;
    payload.zoom_level             = zoom_level;
    payload.confidence             = confidence;
    payload.yaw_global             = yaw_global;
    payload.pitch_global           = pitch_global;
    payload.rel_frame_of_reference = rel_frame_of_reference;
    payload.yaw_rel                = yaw_rel;
    payload.pitch_rel              = pitch_rel;
    payload.publish_timestamp_us   = static_cast<uint64_t>(publish_timestamp_us);
    payload.status                 = status;

    mavlink_msg_single_target_tracking_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendCalibrationParameters(
    uint8_t cam_id, uint8_t calib_command, uint8_t calib_status)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_calibration_parameters_t payload{};

    payload.cam_id        = cam_id;
    payload.calib_command = calib_command;
    payload.calib_status  = calib_status;

    mavlink_msg_calibration_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

void SvMavlinkHandler::sendNavigationParameters(
    float altitude, float visual_lat, float visual_lon,
    float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
    float visual_vel_x, float visual_vel_y, float visual_vel_z)
{
    if (!_vehicle) return;

    mavlink_message_t msg;
    mavlink_navigation_parameters_t payload{};

    payload.altitude                    = altitude;
    payload.visual_lat                  = visual_lat;
    payload.visual_lon                  = visual_lon;
    payload.next_waypoint_target_yaw    = next_waypoint_target_yaw;
    payload.next_waypoint_target_pitch  = next_waypoint_target_pitch;
    payload.next_waypoint_target_roll   = next_waypoint_target_roll;
    payload.visual_vel_x                = visual_vel_x;
    payload.visual_vel_y                = visual_vel_y;
    payload.visual_vel_z                = visual_vel_z;

    mavlink_msg_navigation_parameters_encode(
        _vehicle->id(), _vehicle->defaultComponentId(), &msg, &payload);
    _sendMessage(msg);
}

// --------------------------------------------------------------------------
// INCOMING PIPELINE  (received from drone via Vehicle -> dispatched to QML)
// --------------------------------------------------------------------------

void SvMavlinkHandler::handleMessage(const mavlink_message_t& message)
{
    // Only handle messages from our own vehicle's system id.
    if (!_vehicle || message.sysid != _vehicle->id()) return;

    // Drop silently if QML target is not registered yet.
    if (!_svCamItem) return;

    switch (message.msgid) {

        case MAVLINK_MSG_ID_SYSTEM_STATUS_PARAMETERS: {
            mavlink_system_status_parameters_t p;
            mavlink_msg_system_status_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "systemStatusCallback",
                Q_ARG(QVariant, p.status),
                Q_ARG(QVariant, p.error),
                Q_ARG(QVariant, p.jetson_temp));
            break;
        }

        case MAVLINK_MSG_ID_AI_PARAMETERS: {
            mavlink_ai_parameters_t p;
            mavlink_msg_ai_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "aiCallback",
                Q_ARG(QVariant, p.run_ai),
                Q_ARG(QVariant, QString::fromLatin1(p.track_model_name, strnlen(p.track_model_name, 16))),
                Q_ARG(QVariant, QString::fromLatin1(p.scan_model_name,  strnlen(p.scan_model_name,  16))));
            break;
        }

        case MAVLINK_MSG_ID_MODEL_PARAMETERS: {
            mavlink_model_parameters_t p;
            mavlink_msg_model_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "modelCallback",
                Q_ARG(QVariant, QString::fromLatin1(p.model_name, strnlen(p.model_name, 16))));
            break;
        }

        case MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS: {
            mavlink_video_output_parameters_t p;
            mavlink_msg_video_output_parameters_decode(&message, &p);

            QVector<int> vx, vy, vw, vh;
            for (int i = 0; i < 4; ++i) {
                vx.append(p.views_x[i]);
                vy.append(p.views_y[i]);
                vw.append(p.views_w[i]);
                vh.append(p.views_h[i]);
            }
            QMetaObject::invokeMethod(_svCamItem, "videoOutputCallback",
                Q_ARG(QVariant, QString::fromLatin1(p.stream_name, strnlen(p.stream_name, 16))),
                Q_ARG(QVariant, p.width),
                Q_ARG(QVariant, p.height),
                Q_ARG(QVariant, p.fps),
                Q_ARG(QVariant, p.layout_mode),
                Q_ARG(QVariant, p.detection_overlay_mode),
                Q_ARG(QVariant, p.num_user_views),
                Q_ARG(QVariant, QVariant::fromValue(vx)),
                Q_ARG(QVariant, QVariant::fromValue(vy)),
                Q_ARG(QVariant, QVariant::fromValue(vw)),
                Q_ARG(QVariant, QVariant::fromValue(vh)),
                Q_ARG(QVariant, p.detection_overlay_x),
                Q_ARG(QVariant, p.detection_overlay_y),
                Q_ARG(QVariant, p.detection_overlay_w),
                Q_ARG(QVariant, p.detection_overlay_h),
                Q_ARG(QVariant, p.single_detection_size));
            break;
        }

        case MAVLINK_MSG_ID_CAPTURE_PARAMETERS: {
            mavlink_capture_parameters_t p;
            mavlink_msg_capture_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "captureCallback",
                Q_ARG(QVariant, QString::fromLatin1(p.stream_name, strnlen(p.stream_name, 16))),
                Q_ARG(QVariant, p.cap_single_image),
                Q_ARG(QVariant, p.record_video),
                Q_ARG(QVariant, p.images_captured),
                Q_ARG(QVariant, p.videos_captured));
            break;
        }

        case MAVLINK_MSG_ID_DETECTION_PARAMETERS: {
            mavlink_detection_parameters_t p;
            mavlink_msg_detection_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "detectionParametersCallback",
                Q_ARG(QVariant, p.mode),
                Q_ARG(QVariant, p.sorting_mode),
                Q_ARG(QVariant, p.track_confidence_threshold),
                Q_ARG(QVariant, p.scan_confidence_threshold),
                Q_ARG(QVariant, p.track_box_overlap),
                Q_ARG(QVariant, p.scan_box_overlap),
                Q_ARG(QVariant, p.creation_score_scale),
                Q_ARG(QVariant, p.bonus_detection_scale),
                Q_ARG(QVariant, p.bonus_redetection_scale),
                Q_ARG(QVariant, p.missed_detection_penalty),
                Q_ARG(QVariant, p.missed_redetection_penalty));
            break;
        }

        case MAVLINK_MSG_ID_TRACKED_DETECTION_PARAMETERS: {
            mavlink_tracked_detection_parameters_t p;
            mavlink_msg_tracked_detection_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "trackedDetectionCallback",
                Q_ARG(QVariant, p.index),
                Q_ARG(QVariant, p.score),
                Q_ARG(QVariant, p.total_detections),
                Q_ARG(QVariant, p.type),
                Q_ARG(QVariant, p.yaw_global),
                Q_ARG(QVariant, p.pitch_global),
                Q_ARG(QVariant, p.rel_frame_of_reference),
                Q_ARG(QVariant, p.yaw_rel),
                Q_ARG(QVariant, p.pitch_rel),
                Q_ARG(QVariant, p.latitude),
                Q_ARG(QVariant, p.longitude),
                Q_ARG(QVariant, p.altitude),
                Q_ARG(QVariant, p.distance),
                Q_ARG(QVariant, p.width),
                Q_ARG(QVariant, p.height),
                Q_ARG(QVariant, p.track_id),
                Q_ARG(QVariant, static_cast<quint64>(p.publish_timestamp_us)));
            break;
        }

        case MAVLINK_MSG_ID_CAM_TARGETING_PARAMETERS: {
            mavlink_cam_targeting_parameters_t p;
            mavlink_msg_cam_targeting_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "camTargetingCallback",
                Q_ARG(QVariant, QString::fromLatin1(p.stream_name, strnlen(p.stream_name, 16))),
                Q_ARG(QVariant, p.cam_id),
                Q_ARG(QVariant, p.targeting_mode),
                Q_ARG(QVariant, p.euler_delta),
                Q_ARG(QVariant, p.yaw),
                Q_ARG(QVariant, p.pitch),
                Q_ARG(QVariant, p.roll),
                Q_ARG(QVariant, p.lock_flags),
                Q_ARG(QVariant, p.x_offset),
                Q_ARG(QVariant, p.y_offset),
                Q_ARG(QVariant, p.target_latitude),
                Q_ARG(QVariant, p.target_longitude),
                Q_ARG(QVariant, p.target_altitude),
                Q_ARG(QVariant, p.detection_id));
            break;
        }

        case MAVLINK_MSG_ID_CAM_OPTICS_AND_CONTROL_PARAMETERS: {
            mavlink_cam_optics_and_control_parameters_t p;
            mavlink_msg_cam_optics_and_control_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "camOpticsAndControlCallback",
                Q_ARG(QVariant, QString::fromLatin1(p.stream_name, strnlen(p.stream_name, 16))),
                Q_ARG(QVariant, p.cam_id),
                Q_ARG(QVariant, p.zoom),
                Q_ARG(QVariant, p.fov),
                Q_ARG(QVariant, p.crop_mode));
            break;
        }

        case MAVLINK_MSG_ID_CAM_OFFSET_PARAMETERS: {
            mavlink_cam_offset_parameters_t p;
            mavlink_msg_cam_offset_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "camOffsetCallback",
                Q_ARG(QVariant, QString::fromLatin1(p.stream_name, strnlen(p.stream_name, 16))),
                Q_ARG(QVariant, p.cam_id),
                Q_ARG(QVariant, p.x),
                Q_ARG(QVariant, p.y),
                Q_ARG(QVariant, p.yaw_global),
                Q_ARG(QVariant, p.pitch_global),
                Q_ARG(QVariant, p.yaw_rel),
                Q_ARG(QVariant, p.pitch_rel));
            break;
        }

        case MAVLINK_MSG_ID_SENSOR_PARAMETERS: {
            mavlink_sensor_parameters_t p;
            mavlink_msg_sensor_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "sensorSettingsCallback",
                Q_ARG(QVariant, static_cast<quint32>(p.min_exposure)),
                Q_ARG(QVariant, static_cast<quint32>(p.max_exposure)),
                Q_ARG(QVariant, static_cast<quint32>(p.min_gain)),
                Q_ARG(QVariant, static_cast<quint32>(p.max_gain)),
                Q_ARG(QVariant, p.target_brightness));
            break;
        }

        case MAVLINK_MSG_ID_CAM_DEPTH_ESTIMATION_PARAMETERS: {
            mavlink_cam_depth_estimation_parameters_t p;
            mavlink_msg_cam_depth_estimation_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "camDepthEstimationCallback",
                Q_ARG(QVariant, QString::fromLatin1(p.stream_name, strnlen(p.stream_name, 16))),
                Q_ARG(QVariant, p.cam_id),
                Q_ARG(QVariant, p.depth_estimation_mode),
                Q_ARG(QVariant, p.depth));
            break;
        }

        case MAVLINK_MSG_ID_SINGLE_TARGET_TRACKING_PARAMETERS: {
            mavlink_single_target_tracking_parameters_t p;
            mavlink_msg_single_target_tracking_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "singleTargetTrackingCallback",
                Q_ARG(QVariant, p.command),
                Q_ARG(QVariant, QString::fromLatin1(p.stream_name, strnlen(p.stream_name, 16))),
                Q_ARG(QVariant, p.cam_id),
                Q_ARG(QVariant, p.x_offset),
                Q_ARG(QVariant, p.y_offset),
                Q_ARG(QVariant, p.detection_id),
                Q_ARG(QVariant, p.zoom_level),
                Q_ARG(QVariant, p.confidence),
                Q_ARG(QVariant, p.yaw_global),
                Q_ARG(QVariant, p.pitch_global),
                Q_ARG(QVariant, p.rel_frame_of_reference),
                Q_ARG(QVariant, p.yaw_rel),
                Q_ARG(QVariant, p.pitch_rel),
                Q_ARG(QVariant, static_cast<quint64>(p.publish_timestamp_us)),
                Q_ARG(QVariant, p.status));
            break;
        }

        case MAVLINK_MSG_ID_CALIBRATION_PARAMETERS: {
            mavlink_calibration_parameters_t p;
            mavlink_msg_calibration_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "calibrationCallback",
                Q_ARG(QVariant, p.cam_id),
                Q_ARG(QVariant, p.calib_command),
                Q_ARG(QVariant, p.calib_status));
            break;
        }

        case MAVLINK_MSG_ID_NAVIGATION_PARAMETERS: {
            mavlink_navigation_parameters_t p;
            mavlink_msg_navigation_parameters_decode(&message, &p);
            QMetaObject::invokeMethod(_svCamItem, "navigationCallback",
                Q_ARG(QVariant, p.altitude),
                Q_ARG(QVariant, p.visual_lat),
                Q_ARG(QVariant, p.visual_lon),
                Q_ARG(QVariant, p.next_waypoint_target_yaw),
                Q_ARG(QVariant, p.next_waypoint_target_pitch),
                Q_ARG(QVariant, p.next_waypoint_target_roll),
                Q_ARG(QVariant, p.visual_vel_x),
                Q_ARG(QVariant, p.visual_vel_y),
                Q_ARG(QVariant, p.visual_vel_z));
            break;
        }

        default:
            break;
    }
}