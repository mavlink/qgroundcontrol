#include "DigiviewLegacyTcpAdapter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <type_traits>

#include <QtCore/QDateTime>

#include "QGCLoggingCategory.h"
#include "msg_defs.hpp"
#include "sv_mavlink_dialect/sv_mavlink_dialect.h"

QGC_LOGGING_CATEGORY(DigiviewLegacyTcpAdapterLog, "Digiview.LegacyTcp.Adapter")

namespace {

constexpr uint8_t kSyntheticSystemId = 252;
constexpr uint8_t kSyntheticComponentId = 66;
constexpr float kOneShotIntervalUs = -1000.0F;
constexpr char kSupportedParameterGroups[] =
    "SYSTEM_STATUS, MODEL (GET only), VIDEO_OUTPUT, CAPTURE, DETECTION, TRACKED_DETECTION (GET only), "
    "CAM_TARGETING, CAM_OPTICS_AND_CONTROL, SENSOR, SINGLE_TARGET_TRACKING, and CALIBRATION (GET only)";

const char* deliberatelyUnsupportedParameterGroup(uint32_t messageId)
{
    switch (messageId) {
    case MAVLINK_MSG_ID_AI_PARAMETERS:
        return "AI";
    case MAVLINK_MSG_ID_MODEL_PARAMETERS:
        return "MODEL";
    case MAVLINK_MSG_ID_TRACKED_DETECTION_PARAMETERS:
        return "TRACKED_DETECTION";
    case MAVLINK_MSG_ID_CAM_OFFSET_PARAMETERS:
        return "CAM_OFFSET";
    case MAVLINK_MSG_ID_CAM_DEPTH_ESTIMATION_PARAMETERS:
        return "CAM_DEPTH_ESTIMATION";
    case MAVLINK_MSG_ID_CALIBRATION_PARAMETERS:
        return "CALIBRATION";
    case MAVLINK_MSG_ID_NAVIGATION_PARAMETERS:
        return "NAVIGATION";
    default:
        return nullptr;
    }
}

QString unsupportedParameterGroupError(uint32_t messageId, const char* operation)
{
    if (const char* const group = deliberatelyUnsupportedParameterGroup(messageId)) {
        return QStringLiteral("DigiView legacy TCP %1 does not support the %2 parameter group (MAVLink message %3); "
                              "supported groups are %4")
            .arg(QString::fromLatin1(operation), QString::fromLatin1(group))
            .arg(messageId)
            .arg(QString::fromLatin1(kSupportedParameterGroups));
    }

    return QStringLiteral("Unsupported MAVLink message %1 for DigiView legacy TCP %2; supported parameter groups are "
                          "%3")
        .arg(messageId)
        .arg(QString::fromLatin1(operation), QString::fromLatin1(kSupportedParameterGroups));
}

void finalizeNativeMessage(message& nativeMessage)
{
    nativeMessage.timestamp = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()) * 1000U;
    add_checksum_for_digiview_message(nativeMessage);
}

QByteArray nativeRecord(message& nativeMessage)
{
    finalizeNativeMessage(nativeMessage);
    return QByteArray(reinterpret_cast<const char*>(&nativeMessage), sizeof(nativeMessage));
}

bool intervalRequest(const mavlink_command_long_t& command, message& nativeMessage, QString& error)
{
    if (command.command != MAV_CMD_SET_MESSAGE_INTERVAL) {
        error = QStringLiteral("Unsupported DigiView TCP MAVLink command %1").arg(command.command);
        return false;
    }

    if (!std::isfinite(command.param1) || !std::isfinite(command.param2) || (command.param1 < 0.0F)) {
        error = QStringLiteral("Invalid DigiView TCP message interval request");
        return false;
    }

    const auto requestedMessageId = static_cast<uint32_t>(command.param1);
    if (static_cast<float>(requestedMessageId) != command.param1) {
        error = QStringLiteral("Invalid DigiView TCP subscription message id %1").arg(command.param1);
        return false;
    }

    uint8_t parameterType = 0;
    switch (requestedMessageId) {
    case MAVLINK_MSG_ID_SYSTEM_STATUS_PARAMETERS:
        parameterType = SYSTEM_STATUS;
        break;
    case MAVLINK_MSG_ID_MODEL_PARAMETERS:
        parameterType = MODEL;
        break;
    case MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS:
        parameterType = VIDEO_OUTPUT;
        break;
    case MAVLINK_MSG_ID_CAPTURE_PARAMETERS:
        parameterType = CAPTURE;
        break;
    case MAVLINK_MSG_ID_CAM_TARGETING_PARAMETERS:
        parameterType = CAM_TARGETING;
        break;
    case MAVLINK_MSG_ID_SINGLE_TARGET_TRACKING_PARAMETERS:
        parameterType = SINGLE_TARGET_TRACKING;
        break;
    case MAVLINK_MSG_ID_SENSOR_PARAMETERS:
        parameterType = SENSOR;
        break;
    case MAVLINK_MSG_ID_DETECTION_PARAMETERS:
        parameterType = DETECTION;
        break;
    case MAVLINK_MSG_ID_TRACKED_DETECTION_PARAMETERS:
        if (command.param3 != static_cast<float>(std::numeric_limits<uint8_t>::max())) {
            error = QStringLiteral("DigiView TRACKED_DETECTION GET requires index 255");
            return false;
        }
        parameterType = TRACKED_DETECTION;
        break;
    case MAVLINK_MSG_ID_CAM_OPTICS_AND_CONTROL_PARAMETERS:
        parameterType = CAM_OPTICS_AND_CONTROL;
        break;
    case MAVLINK_MSG_ID_CALIBRATION_PARAMETERS:
        if (!std::isfinite(command.param3) || (command.param3 < 0.0F)
            || (command.param3 > static_cast<float>(std::numeric_limits<uint8_t>::max()))) {
            error = QStringLiteral("Invalid DigiView CALIBRATION camera id %1").arg(command.param3);
            return false;
        }
        parameterType = CALIBRATION;
        break;
    default:
        error = unsupportedParameterGroupError(requestedMessageId, "subscription");
        return false;
    }

    pack_get_parameters(nativeMessage, parameterType);
    if (parameterType == TRACKED_DETECTION) {
        pack_tracked_detection_parameters(
            nativeMessage, 0, std::numeric_limits<uint8_t>::max(), 0, -2, 0.0F, 0.0F,
            static_cast<uint8_t>(command.param4), 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F);
    } else if (parameterType == CALIBRATION) {
        pack_calibration_parameters(nativeMessage, static_cast<uint8_t>(command.param3), CALIBRATION_CMD_NONE,
                                    CALIBRATION_STATUS_NOT_STARTED, 0, 0);
    }
    if ((command.param2 == kOneShotIntervalUs) || (command.param2 == 0.0F)) {
        nativeMessage.interval_ms = 0;
    } else if (command.param2 < 0.0F) {
        error = QStringLiteral("Unsupported DigiView legacy TCP message interval %1 us; use -1000 for a one-shot "
                               "request, 0, or a positive interval")
                    .arg(command.param2);
        return false;
    } else {
        const double intervalMs = std::ceil(static_cast<double>(command.param2) / 1000.0);
        nativeMessage.interval_ms = static_cast<uint32_t>(std::min(
            intervalMs, static_cast<double>(std::numeric_limits<uint32_t>::max())));
    }

    return true;
}

} // namespace

qsizetype DigiviewLegacyTcpAdapter::recordSize()
{
    static_assert(std::is_standard_layout_v<message> && std::is_trivially_copyable_v<message>);
    return static_cast<qsizetype>(sizeof(message));
}

QByteArray DigiviewLegacyTcpAdapter::encode(const mavlink_message_t& mavlinkMessage, QString& error)
{
    error.clear();
    message nativeMessage {};

    switch (mavlinkMessage.msgid) {
    case MAVLINK_MSG_ID_COMMAND_LONG: {
        mavlink_command_long_t parameters {};
        mavlink_msg_command_long_decode(&mavlinkMessage, &parameters);
        if (!intervalRequest(parameters, nativeMessage, error)) {
            return {};
        }
        break;
    }
    case MAVLINK_MSG_ID_SYSTEM_STATUS_PARAMETERS: {
        mavlink_system_status_parameters_t parameters {};
        mavlink_msg_system_status_parameters_decode(&mavlinkMessage, &parameters);
        nativeMessage.version = VERSION;
        nativeMessage.message_type = SET_PARAMETERS;
        pack_system_status_parameters(nativeMessage, u8_to_enum<app_status>(parameters.status), parameters.error,
                                      parameters.jetson_temp);
        break;
    }
    case MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS: {
        mavlink_video_output_parameters_t parameters {};
        mavlink_msg_video_output_parameters_decode(&mavlinkMessage, &parameters);
        if (parameters.num_user_views > 4U) {
            error = QStringLiteral("Invalid DigiView TCP video output view count %1").arg(parameters.num_user_views);
            return {};
        }

        std::array<bounding_box, 4> views {};
        for (size_t index = 0; index < views.size(); ++index) {
            views[index] = {
                parameters.views_x[index],
                parameters.views_y[index],
                parameters.views_w[index],
                parameters.views_h[index],
            };
        }
        const bounding_box detectionOverlay {
            parameters.detection_overlay_x,
            parameters.detection_overlay_y,
            parameters.detection_overlay_w,
            parameters.detection_overlay_h,
        };

        nativeMessage.version = VERSION;
        nativeMessage.message_type = SET_PARAMETERS;
        pack_video_output_parameters(nativeMessage, parameters.stream_name, parameters.width, parameters.height,
                                     parameters.fps, parameters.layout_mode, parameters.detection_overlay_mode,
                                     parameters.num_user_views, views.data(), detectionOverlay,
                                     parameters.single_detection_size);
        break;
    }
    case MAVLINK_MSG_ID_CAPTURE_PARAMETERS: {
        mavlink_capture_parameters_t parameters {};
        mavlink_msg_capture_parameters_decode(&mavlinkMessage, &parameters);
        const bool captureImage = parameters.cap_single_image == 1U;
        bool recordVideo = parameters.record_video == 1U;
        if (parameters.record_video == std::numeric_limits<uint8_t>::max()) {
            recordVideo = _recordingStateKnown && _recordingActive;
            if (!_recordingStateKnown) {
                qCWarning(DigiviewLegacyTcpAdapterLog)
                    << "DigiView TCP capture requested an unchanged recording state before one was known; assuming off";
            }
        } else if (parameters.record_video > 1U) {
            error = QStringLiteral("Invalid DigiView TCP record-video value %1").arg(parameters.record_video);
            return {};
        }
        if ((parameters.cap_single_image != std::numeric_limits<uint8_t>::max())
            && (parameters.cap_single_image > 1U)) {
            error = QStringLiteral("Invalid DigiView TCP capture-image value %1").arg(parameters.cap_single_image);
            return {};
        }

        nativeMessage.version = VERSION;
        nativeMessage.message_type = SET_PARAMETERS;
        pack_capture_parameters(nativeMessage, parameters.stream_name, captureImage, recordVideo,
                                parameters.images_captured, parameters.videos_captured);
        if (parameters.record_video != std::numeric_limits<uint8_t>::max()) {
            _recordingActive = recordVideo;
            _recordingStateKnown = true;
        }
        break;
    }
    case MAVLINK_MSG_ID_DETECTION_PARAMETERS: {
        mavlink_detection_parameters_t parameters {};
        mavlink_msg_detection_parameters_decode(&mavlinkMessage, &parameters);
        pack_set_detection_parameters(nativeMessage, parameters.mode, parameters.sorting_mode,
                                      parameters.track_confidence_threshold, parameters.scan_confidence_threshold,
                                      parameters.track_box_overlap, parameters.scan_box_overlap,
                                      parameters.creation_score_scale, parameters.bonus_detection_scale,
                                      parameters.bonus_redetection_scale, parameters.missed_detection_penalty,
                                      parameters.missed_redetection_penalty);
        break;
    }
    case MAVLINK_MSG_ID_CAM_TARGETING_PARAMETERS: {
        mavlink_cam_targeting_parameters_t parameters {};
        mavlink_msg_cam_targeting_parameters_decode(&mavlinkMessage, &parameters);
        pack_set_cam_targeting_parameters(nativeMessage, parameters.stream_name, parameters.cam_id,
                                          u8_to_enum<View::TargetingMode>(parameters.targeting_mode),
                                          parameters.euler_delta != 0U, parameters.yaw, parameters.pitch,
                                          parameters.roll, parameters.lock_flags, parameters.x_offset,
                                          parameters.y_offset, parameters.target_latitude,
                                          parameters.target_longitude, parameters.target_altitude,
                                          parameters.track_id, parameters.view_id, parameters.lock_target != 0U);
        break;
    }
    case MAVLINK_MSG_ID_CAM_OPTICS_AND_CONTROL_PARAMETERS: {
        mavlink_cam_optics_and_control_parameters_t parameters {};
        mavlink_msg_cam_optics_and_control_parameters_decode(&mavlinkMessage, &parameters);
        pack_set_cam_optics_and_control_parameters(nativeMessage, parameters.stream_name, parameters.cam_id,
                                                   parameters.zoom, parameters.fov);
        break;
    }
    case MAVLINK_MSG_ID_SENSOR_PARAMETERS: {
        mavlink_sensor_parameters_t parameters {};
        mavlink_msg_sensor_parameters_decode(&mavlinkMessage, &parameters);
        pack_set_sensor_parameters(nativeMessage, parameters.min_exposure, parameters.max_exposure,
                                   parameters.min_gain, parameters.max_gain, parameters.target_brightness);
        break;
    }
    case MAVLINK_MSG_ID_SINGLE_TARGET_TRACKING_PARAMETERS: {
        mavlink_single_target_tracking_parameters_t parameters {};
        mavlink_msg_single_target_tracking_parameters_decode(&mavlinkMessage, &parameters);
        pack_set_single_target_tracking_parameters(
            nativeMessage, u8_to_enum<single_target_tracker_command>(parameters.command), parameters.stream_name,
            parameters.cam_id, parameters.x_offset, parameters.y_offset, parameters.detection_id,
            parameters.zoom_level, parameters.confidence, parameters.yaw_global, parameters.pitch_global,
            parameters.rel_frame_of_reference, parameters.yaw_rel, parameters.pitch_rel,
            parameters.lock_target != 0U);
        break;
    }
    default:
        error = unsupportedParameterGroupError(mavlinkMessage.msgid, "send");
        return {};
    }

    return nativeRecord(nativeMessage);
}

DigiviewLegacyTcpAdapter::DecodeResult DigiviewLegacyTcpAdapter::decode(
    QByteArrayView record, mavlink_message_t& mavlinkMessage, QString& error)
{
    error.clear();
    if (record.size() != recordSize()) {
        error = QStringLiteral("Invalid DigiView TCP record size %1 (expected %2)")
                    .arg(record.size())
                    .arg(recordSize());
        return DecodeResult::Error;
    }

    message nativeMessage {};
    std::memcpy(&nativeMessage, record.data(), sizeof(nativeMessage));

    crc8 checksumGenerator(CRC8TYPE::BLUETOOTH);
    const uint8_t expectedChecksum = checksumGenerator.crc(
        reinterpret_cast<uint8_t*>(&nativeMessage), offsetof(message, checksum));
    if (nativeMessage.checksum != expectedChecksum) {
        error = QStringLiteral("Rejected DigiView TCP record with invalid checksum");
        return DecodeResult::Error;
    }
    if (nativeMessage.version != VERSION) {
        error = QStringLiteral("Unsupported DigiView TCP protocol version %1").arg(nativeMessage.version);
        return DecodeResult::Error;
    }

    if (nativeMessage.message_type != CURRENT_PARAMETERS) {
        if (nativeMessage.param_type == VIDEO_OUTPUT) {
            MAV_RESULT result = MAV_RESULT_FAILED;
            bool isVideoOutputSetResponse = true;
            switch (nativeMessage.message_type) {
            case ACKNOWLEDGEMENT:
                result = MAV_RESULT_ACCEPTED;
                break;
            case CHECKSUM_ERROR:
                error = QStringLiteral("DigiView rejected a TCP record checksum");
                break;
            case DATA_ERROR:
                error = QStringLiteral("DigiView rejected TCP record data");
                break;
            case FORBIDDEN:
                result = MAV_RESULT_DENIED;
                error = QStringLiteral("DigiView forbade a TCP control request");
                break;
            case UNKNOWN:
                error = QStringLiteral("DigiView did not recognize a TCP control request");
                break;
            default:
                isVideoOutputSetResponse = false;
                break;
            }

            if (isVideoOutputSetResponse) {
                mavlink_msg_command_ack_pack(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                             MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS, result, 0, 0, 0, 0);
                return DecodeResult::Message;
            }
        }

        switch (nativeMessage.message_type) {
        case ACKNOWLEDGEMENT:
            return DecodeResult::Ignored;
        case CHECKSUM_ERROR:
            error = QStringLiteral("DigiView rejected a TCP record checksum");
            return DecodeResult::Error;
        case DATA_ERROR:
            error = QStringLiteral("DigiView rejected TCP record data");
            return DecodeResult::Error;
        case FORBIDDEN:
            error = QStringLiteral("DigiView forbade a TCP control request");
            return DecodeResult::Error;
        case UNKNOWN:
            error = QStringLiteral("DigiView did not recognize a TCP control request");
            return DecodeResult::Error;
        default:
            error = QStringLiteral("Ignoring DigiView TCP response type %1").arg(nativeMessage.message_type);
            return DecodeResult::Ignored;
        }
    }

    switch (nativeMessage.param_type) {
    case SYSTEM_STATUS: {
        system_status_parameters nativeParameters {};
        unpack_system_status_parameters(nativeMessage, nativeParameters);
        mavlink_system_status_parameters_t parameters {};
        parameters.status = enum_to_u8(nativeParameters.status);
        parameters.error = nativeParameters.error;
        parameters.jetson_temp = nativeParameters.jetson_temp;
        mavlink_msg_system_status_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                                     &parameters);
        break;
    }
    case MODEL: {
        model_parameters nativeParameters {};
        unpack_model_parameters(nativeMessage, nativeParameters);
        mavlink_model_parameters_t parameters {};
        std::memcpy(parameters.model_name, nativeParameters.model_name, sizeof(parameters.model_name));
        mavlink_msg_model_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage, &parameters);
        break;
    }
    case VIDEO_OUTPUT: {
        video_output_parameters nativeParameters {};
        unpack_video_output_parameters(nativeMessage, nativeParameters);
        mavlink_video_output_parameters_t parameters {};
        std::memcpy(parameters.stream_name, nativeParameters.stream_name, sizeof(parameters.stream_name));
        parameters.width = nativeParameters.width;
        parameters.height = nativeParameters.height;
        parameters.fps = nativeParameters.fps;
        parameters.layout_mode = nativeParameters.layout_mode;
        parameters.detection_overlay_mode = nativeParameters.detection_overlay_mode;
        parameters.num_user_views = nativeParameters.num_user_views;
        for (size_t index = 0; index < std::size(nativeParameters.views); ++index) {
            parameters.views_x[index] = nativeParameters.views[index].x;
            parameters.views_y[index] = nativeParameters.views[index].y;
            parameters.views_w[index] = nativeParameters.views[index].w;
            parameters.views_h[index] = nativeParameters.views[index].h;
        }
        parameters.detection_overlay_x = nativeParameters.detection_overlay_box.x;
        parameters.detection_overlay_y = nativeParameters.detection_overlay_box.y;
        parameters.detection_overlay_w = nativeParameters.detection_overlay_box.w;
        parameters.detection_overlay_h = nativeParameters.detection_overlay_box.h;
        parameters.single_detection_size = nativeParameters.single_detection_size;
        mavlink_msg_video_output_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                                   &parameters);
        break;
    }
    case CAPTURE: {
        capture_parameters nativeParameters {};
        unpack_capture_parameters(nativeMessage, nativeParameters);
        mavlink_capture_parameters_t parameters {};
        std::memcpy(parameters.stream_name, nativeParameters.stream_name, sizeof(parameters.stream_name));
        parameters.cap_single_image = nativeParameters.cap_single_image ? 1U : 0U;
        parameters.record_video = nativeParameters.record_video ? 1U : 0U;
        parameters.images_captured = nativeParameters.images_captured;
        parameters.videos_captured = nativeParameters.videos_captured;
        _recordingActive = nativeParameters.record_video;
        _recordingStateKnown = true;
        mavlink_msg_capture_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                              &parameters);
        break;
    }
    case DETECTION: {
        detection_parameters nativeParameters {};
        unpack_detection_parameters(nativeMessage, nativeParameters);
        mavlink_detection_parameters_t parameters {};
        parameters.mode = nativeParameters.mode;
        parameters.sorting_mode = nativeParameters.sorting_mode;
        parameters.track_confidence_threshold = nativeParameters.track_confidence_threshold;
        parameters.scan_confidence_threshold = nativeParameters.scan_confidence_threshold;
        parameters.track_box_overlap = nativeParameters.track_box_overlap;
        parameters.scan_box_overlap = nativeParameters.scan_box_overlap;
        parameters.creation_score_scale = nativeParameters.creation_score_scale;
        parameters.bonus_detection_scale = nativeParameters.bonus_detection_scale;
        parameters.bonus_redetection_scale = nativeParameters.bonus_redetection_scale;
        parameters.missed_detection_penalty = nativeParameters.missed_detection_penalty;
        parameters.missed_redetection_penalty = nativeParameters.missed_redetection_penalty;
        mavlink_msg_detection_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                                 &parameters);
        break;
    }
    case TRACKED_DETECTION: {
        tracked_detection_parameters nativeParameters {};
        unpack_tracked_detection_parameters(nativeMessage, nativeParameters);
        mavlink_tracked_detection_parameters_t parameters {};
        parameters.index = nativeParameters.index;
        parameters.score = nativeParameters.score;
        parameters.total_detections = nativeParameters.total_detections;
        parameters.type = nativeParameters.type;
        parameters.yaw_global = nativeParameters.yaw_global;
        parameters.pitch_global = nativeParameters.pitch_global;
        parameters.rel_frame_of_reference = nativeParameters.rel_frame_of_reference;
        parameters.yaw_rel = nativeParameters.yaw_rel;
        parameters.pitch_rel = nativeParameters.pitch_rel;
        parameters.latitude = nativeParameters.latitude;
        parameters.longitude = nativeParameters.longitude;
        parameters.altitude = nativeParameters.altitude;
        parameters.distance = nativeParameters.distance;
        parameters.width = nativeParameters.width;
        parameters.height = nativeParameters.height;
        parameters.track_id = nativeParameters.track_id;
        parameters.publish_timestamp_us = nativeParameters.publish_timestamp_us;
        parameters.view_id = nativeParameters.view_id;
        mavlink_msg_tracked_detection_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                                        &parameters);
        break;
    }
    case CAM_TARGETING: {
        cam_targeting_parameters nativeParameters {};
        unpack_cam_targeting_parameters(nativeMessage, nativeParameters);
        mavlink_cam_targeting_parameters_t parameters {};
        std::memcpy(parameters.stream_name, nativeParameters.stream_name, sizeof(parameters.stream_name));
        parameters.cam_id = nativeParameters.cam_id;
        parameters.targeting_mode = enum_to_u8(nativeParameters.targeting_mode);
        parameters.euler_delta = nativeParameters.euler_delta ? 1U : 0U;
        parameters.yaw = nativeParameters.yaw;
        parameters.pitch = nativeParameters.pitch;
        parameters.roll = nativeParameters.roll;
        parameters.lock_flags = nativeParameters.lock_flags;
        parameters.x_offset = nativeParameters.x_offset;
        parameters.y_offset = nativeParameters.y_offset;
        parameters.target_latitude = nativeParameters.target_latitude;
        parameters.target_longitude = nativeParameters.target_longitude;
        parameters.target_altitude = nativeParameters.target_altitude;
        parameters.track_id = nativeParameters.track_id;
        parameters.view_id = nativeParameters.view_id;
        parameters.lock_target = nativeParameters.lock_target ? 1U : 0U;
        mavlink_msg_cam_targeting_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                                    &parameters);
        break;
    }
    case CAM_OPTICS_AND_CONTROL: {
        cam_optics_and_control_parameters nativeParameters {};
        unpack_cam_optics_and_control_parameters(nativeMessage, nativeParameters);
        mavlink_cam_optics_and_control_parameters_t parameters {};
        std::memcpy(parameters.stream_name, nativeParameters.stream_name, sizeof(parameters.stream_name));
        parameters.cam_id = nativeParameters.cam_id;
        parameters.zoom = nativeParameters.zoom;
        parameters.fov = nativeParameters.fov;
        mavlink_msg_cam_optics_and_control_parameters_encode(kSyntheticSystemId, kSyntheticComponentId,
                                                             &mavlinkMessage, &parameters);
        break;
    }
    case SENSOR: {
        sensor_parameters nativeParameters {};
        unpack_sensor_parameters(nativeMessage, nativeParameters);
        mavlink_sensor_parameters_t parameters {};
        parameters.min_exposure = nativeParameters.min_exposure;
        parameters.max_exposure = nativeParameters.max_exposure;
        parameters.min_gain = nativeParameters.min_gain;
        parameters.max_gain = nativeParameters.max_gain;
        parameters.target_brightness = nativeParameters.target_brightness;
        mavlink_msg_sensor_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                             &parameters);
        break;
    }
    case SINGLE_TARGET_TRACKING: {
        single_target_tracking_parameters nativeParameters {};
        unpack_single_target_tracking_parameters(nativeMessage, nativeParameters);
        mavlink_single_target_tracking_parameters_t parameters {};
        parameters.command = enum_to_u8(nativeParameters.command);
        std::memcpy(parameters.stream_name, nativeParameters.stream_name, sizeof(parameters.stream_name));
        parameters.cam_id = nativeParameters.cam_id;
        parameters.x_offset = nativeParameters.x_offset;
        parameters.y_offset = nativeParameters.y_offset;
        parameters.detection_id = nativeParameters.detection_id;
        parameters.zoom_level = nativeParameters.zoom_level;
        parameters.confidence = nativeParameters.confidence;
        parameters.yaw_global = nativeParameters.yaw_global;
        parameters.pitch_global = nativeParameters.pitch_global;
        parameters.rel_frame_of_reference = nativeParameters.rel_frame_of_reference;
        parameters.yaw_rel = nativeParameters.yaw_rel;
        parameters.pitch_rel = nativeParameters.pitch_rel;
        parameters.publish_timestamp_us = nativeParameters.publish_timestamp_us;
        parameters.status = enum_to_u8(nativeParameters.status);
        parameters.lock_target = nativeParameters.lock_target ? 1U : 0U;
        mavlink_msg_single_target_tracking_parameters_encode(kSyntheticSystemId, kSyntheticComponentId,
                                                              &mavlinkMessage, &parameters);
        break;
    }
    case CALIBRATION: {
        calibration_parameters nativeParameters {};
        unpack_calibration_parameters(nativeMessage, nativeParameters);
        mavlink_calibration_parameters_t parameters {};
        parameters.cam_id = nativeParameters.cam_id;
        parameters.calib_command = enum_to_u8(nativeParameters.calib_command);
        parameters.calib_status = enum_to_u8(nativeParameters.calib_status);
        mavlink_msg_calibration_parameters_encode(kSyntheticSystemId, kSyntheticComponentId, &mavlinkMessage,
                                                  &parameters);
        break;
    }
    default:
        error = QStringLiteral("Ignoring unsupported DigiView TCP current parameter type %1")
                    .arg(nativeMessage.param_type);
        return DecodeResult::Ignored;
    }

    return DecodeResult::Message;
}
