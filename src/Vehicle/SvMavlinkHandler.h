#pragma once

#include <QObject>
#include <QQuickItem>
#include <QString>
#include <QVector>
#include "mavlink_types.h"

class Vehicle;

class SvMavlinkHandler : public QObject
{
    Q_OBJECT

public:
    explicit SvMavlinkHandler(Vehicle* vehicle, QObject* parent = nullptr);

    Q_INVOKABLE void registerCameraItem(QQuickItem* svCamItem);
    void handleMessage(const mavlink_message_t& message);

    Q_INVOKABLE void sendSystemStatusParameters(uint8_t status, uint8_t error, float jetson_temp);
    Q_INVOKABLE void sendAIParameters(uint8_t run_ai, QString track_model_name, QString scan_model_name);
    Q_INVOKABLE void sendModelParameters(QString model_name);
    Q_INVOKABLE void sendVideoOutputParameters(
        QString stream_name, uint16_t width, uint16_t height, uint8_t fps,
        uint8_t layout_mode, uint8_t detection_overlay_mode, uint8_t num_user_views,
        QVector<int> views_x, QVector<int> views_y, QVector<int> views_w, QVector<int> views_h,
        uint16_t detection_overlay_x, uint16_t detection_overlay_y,
        uint16_t detection_overlay_w, uint16_t detection_overlay_h,
        uint16_t single_detection_size);

    Q_INVOKABLE void sendCaptureParameters(
        QString stream_name, uint8_t cap_single_image, uint8_t record_video,
        uint16_t images_captured, uint16_t videos_captured);

    Q_INVOKABLE void sendDetectionParameters(
        uint8_t mode, uint8_t sorting_mode,
        float track_confidence_threshold, float scan_confidence_threshold,
        float track_box_overlap, float scan_box_overlap,
        uint8_t creation_score_scale, uint8_t bonus_detection_scale,
        uint8_t bonus_redetection_scale, uint8_t missed_detection_penalty,
        uint8_t missed_redetection_penalty);

    Q_INVOKABLE void sendTrackedDetectionParameters(
        uint8_t index, uint8_t score, uint8_t total_detections, int16_t type,
        float yaw_global, float pitch_global, uint8_t rel_frame_of_reference,
        float yaw_rel, float pitch_rel,
        float latitude, float longitude, float altitude,
        float distance, float width, float height,
        uint16_t track_id, quint64 publish_timestamp_us);

    Q_INVOKABLE void sendCamTargetingParameters(
        QString stream_name, uint8_t cam_id, uint8_t targeting_mode, uint8_t euler_delta,
        float yaw, float pitch, float roll, uint8_t lock_flags,
        float x_offset, float y_offset,
        float target_latitude, float target_longitude, float target_altitude,
        int16_t detection_id);

    Q_INVOKABLE void sendCamOpticsAndControlParameters(
        QString stream_name, uint8_t cam_id, int8_t zoom, float fov, uint8_t crop_mode);

    Q_INVOKABLE void sendCamOffsetParameters(
        QString stream_name, uint8_t cam_id,
        float x, float y,
        float yaw_global, float pitch_global, float yaw_rel, float pitch_rel);

    Q_INVOKABLE void sendSensorParameters(
        uint32_t min_exposure, uint32_t max_exposure,
        uint32_t min_gain, uint32_t max_gain,
        float target_brightness);

    Q_INVOKABLE void sendCamDepthEstimationParameters(
        QString stream_name, uint8_t cam_id, uint8_t depth_estimation_mode, float depth);

    Q_INVOKABLE void sendSingleTargetTrackingParameters(
        uint8_t command, QString stream_name, uint8_t cam_id,
        float x_offset, float y_offset,
        uint8_t detection_id, uint16_t zoom_level, float confidence,
        float yaw_global, float pitch_global,
        uint8_t rel_frame_of_reference, float yaw_rel, float pitch_rel,
        quint64 publish_timestamp_us, uint8_t status);

    Q_INVOKABLE void sendCalibrationParameters(
        uint8_t cam_id, uint8_t calib_command, uint8_t calib_status);

    Q_INVOKABLE void sendNavigationParameters(
        float altitude, float visual_lat, float visual_lon,
        float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
        float visual_vel_x, float visual_vel_y, float visual_vel_z);

private:
    Vehicle*     _vehicle   = nullptr;
    QQuickItem*  _svCamItem = nullptr;

    void _sendMessage(mavlink_message_t& msg);
    void copyStringToCharBuf(const QString& src, char* dest, int size);
};