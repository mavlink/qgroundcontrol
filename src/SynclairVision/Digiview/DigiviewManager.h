#pragma once

#include "DigiviewConnection.h"
#include "MAVLinkEnums.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

#include <cstdint>

#include <QVector>

class DigiviewManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(quint16 listenPort READ listenPort WRITE setListenPort NOTIFY listenPortChanged)
    Q_PROPERTY(QString streamName READ streamName WRITE setStreamName NOTIFY streamNameChanged)
    Q_PROPERTY(int senderSystemId READ senderSystemId WRITE setSenderSystemId NOTIFY senderIdentityChanged)
    Q_PROPERTY(int senderComponentId READ senderComponentId WRITE setSenderComponentId NOTIFY senderIdentityChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(quint32 lastReceivedMessageId READ lastReceivedMessageId NOTIFY lastReceivedMessageIdChanged)
    Q_PROPERTY(bool hasVideoOutputParameters READ hasVideoOutputParameters NOTIFY hasVideoOutputParametersChanged)
    Q_PROPERTY(QString videoOutputStreamName READ videoOutputStreamName NOTIFY videoOutputStreamNameChanged)
    Q_PROPERTY(int videoOutputWidth READ videoOutputWidth NOTIFY videoOutputWidthChanged)
    Q_PROPERTY(int videoOutputHeight READ videoOutputHeight NOTIFY videoOutputHeightChanged)
    Q_PROPERTY(int videoOutputFps READ videoOutputFps NOTIFY videoOutputFpsChanged)
    Q_PROPERTY(int videoOutputLayoutMode READ videoOutputLayoutMode NOTIFY videoOutputLayoutModeChanged)
    Q_PROPERTY(int videoOutputDetectionOverlayMode READ videoOutputDetectionOverlayMode NOTIFY
               videoOutputDetectionOverlayModeChanged)
    Q_PROPERTY(int videoOutputNumUserViews READ videoOutputNumUserViews NOTIFY videoOutputNumUserViewsChanged)
    Q_PROPERTY(QVariantList videoOutputViews READ videoOutputViews NOTIFY videoOutputViewsChanged)
    Q_PROPERTY(QVariantMap videoOutputDetectionOverlayRect READ videoOutputDetectionOverlayRect NOTIFY
               videoOutputDetectionOverlayRectChanged)
    Q_PROPERTY(int videoOutputSingleDetectionSize READ videoOutputSingleDetectionSize NOTIFY
               videoOutputSingleDetectionSizeChanged)

public:
    static constexpr uint8_t kDefaultSenderSystemId = 255;
    static constexpr uint8_t kDefaultSenderComponentId = MAV_COMP_ID_MISSIONPLANNER;

    explicit DigiviewManager(QObject* parent = nullptr);
    ~DigiviewManager() override;

    static DigiviewManager* instance();

    QString host() const;
    quint16 port() const;
    quint16 listenPort() const;
    QString streamName() const { return _streamName; }
    int senderSystemId() const { return _senderSystemId; }
    int senderComponentId() const { return _senderComponentId; }
    bool connected() const;
    QString lastError() const;
    quint32 lastReceivedMessageId() const { return _lastReceivedMessageId; }
    bool hasVideoOutputParameters() const { return _hasVideoOutputParameters; }
    QString videoOutputStreamName() const { return _videoOutputStreamName; }
    int videoOutputWidth() const { return _videoOutputWidth; }
    int videoOutputHeight() const { return _videoOutputHeight; }
    int videoOutputFps() const { return _videoOutputFps; }
    int videoOutputLayoutMode() const { return _videoOutputLayoutMode; }
    int videoOutputDetectionOverlayMode() const { return _videoOutputDetectionOverlayMode; }
    int videoOutputNumUserViews() const { return _videoOutputNumUserViews; }
    QVariantList videoOutputViews() const { return _videoOutputViews; }
    QVariantMap videoOutputDetectionOverlayRect() const { return _videoOutputDetectionOverlayRect; }
    int videoOutputSingleDetectionSize() const { return _videoOutputSingleDetectionSize; }

    void setHost(const QString& host);
    void setPort(quint16 port);
    void setListenPort(quint16 listenPort);
    void setStreamName(const QString& streamName);
    void setSenderSystemId(int senderSystemId);
    void setSenderComponentId(int senderComponentId);

    Q_INVOKABLE bool connectToHost();
    Q_INVOKABLE void disconnectFromHost();

    Q_INVOKABLE void sendSystemStatusParameters(uint8_t status, uint8_t error, float jetson_temp);
    Q_INVOKABLE void sendAIParameters(uint8_t run_ai, QString track_model_name, QString scan_model_name);
    Q_INVOKABLE void sendModelParameters(QString model_name);
    Q_INVOKABLE void sendSetVideoOutput(
        QString stream_name, uint16_t width, uint16_t height, uint8_t fps,
        uint8_t layout, uint8_t detection_overlay_mode);
    Q_INVOKABLE void requestVideoOutputParameters();
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
        uint16_t track_id, quint64 publish_timestamp_us, uint8_t view_id);
    Q_INVOKABLE void sendCamTargetingParameters(
        QString stream_name, uint8_t cam_id, uint8_t targeting_mode, uint8_t euler_delta,
        float yaw, float pitch, float roll, uint8_t lock_flags,
        float x_offset, float y_offset,
        float target_latitude, float target_longitude, float target_altitude,
        uint16_t track_id, int16_t view_id, uint8_t lock_target);
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
        quint64 publish_timestamp_us, uint8_t status, uint8_t lock_target);
    Q_INVOKABLE void sendCalibrationParameters(
        uint8_t cam_id, uint8_t calib_command, uint8_t calib_status);
    Q_INVOKABLE void sendNavigationParameters(
        float altitude, float visual_lat, float visual_lon,
        float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
        float visual_vel_x, float visual_vel_y, float visual_vel_z);


    //////////////////////////////////////////////////////////
    ///////////// Helper-functions ///////////////////////////
    //////////////////////////////////////////////////////////

    Q_INVOKABLE void changeEuler(int camId, float yaw, float pitch);
    Q_INVOKABLE void changeZoom(int camId, float zoom);

signals:
    void hostChanged();
    void portChanged();
    void listenPortChanged();
    void streamNameChanged();
    void senderIdentityChanged();
    void connectedChanged();
    void lastErrorChanged();
    void lastReceivedMessageIdChanged();
    void hasVideoOutputParametersChanged();
    void videoOutputStreamNameChanged();
    void videoOutputWidthChanged();
    void videoOutputHeightChanged();
    void videoOutputFpsChanged();
    void videoOutputLayoutModeChanged();
    void videoOutputDetectionOverlayModeChanged();
    void videoOutputNumUserViewsChanged();
    void videoOutputViewsChanged();
    void videoOutputDetectionOverlayRectChanged();
    void videoOutputSingleDetectionSizeChanged();
    void messageDecoded(quint32 messageId);
    void systemStatusParametersReceived(uint8_t status, uint8_t error, float jetson_temp);
    void aiParametersReceived(uint8_t run_ai, const QString& track_model_name, const QString& scan_model_name);
    void modelParametersReceived(const QString& model_name);
    void videoOutputParametersReceived(
        const QString& stream_name, uint16_t width, uint16_t height, uint8_t fps,
        uint8_t layout_mode, uint8_t detection_overlay_mode, uint8_t num_user_views,
        const QVector<int>& views_x, const QVector<int>& views_y, const QVector<int>& views_w, const QVector<int>& views_h,
        uint16_t detection_overlay_x, uint16_t detection_overlay_y,
        uint16_t detection_overlay_w, uint16_t detection_overlay_h,
        uint16_t single_detection_size);
    void captureParametersReceived(
        const QString& stream_name, uint8_t cap_single_image, uint8_t record_video,
        uint16_t images_captured, uint16_t videos_captured);
    void detectionParametersReceived(
        uint8_t mode, uint8_t sorting_mode,
        float track_confidence_threshold, float scan_confidence_threshold,
        float track_box_overlap, float scan_box_overlap,
        uint8_t creation_score_scale, uint8_t bonus_detection_scale,
        uint8_t bonus_redetection_scale, uint8_t missed_detection_penalty,
        uint8_t missed_redetection_penalty);
    void trackedDetectionParametersReceived(
        uint8_t index, uint8_t score, uint8_t total_detections, int16_t type,
        float yaw_global, float pitch_global, uint8_t rel_frame_of_reference,
        float yaw_rel, float pitch_rel,
        float latitude, float longitude, float altitude,
        float distance, float width, float height,
        uint16_t track_id, quint64 publish_timestamp_us, uint8_t view_id);
    void camTargetingParametersReceived(
        const QString& stream_name, uint8_t cam_id, uint8_t targeting_mode, uint8_t euler_delta,
        float yaw, float pitch, float roll, uint8_t lock_flags,
        float x_offset, float y_offset,
        float target_latitude, float target_longitude, float target_altitude,
        uint16_t track_id, int16_t view_id, uint8_t lock_target);
    void camOpticsAndControlParametersReceived(
        const QString& stream_name, uint8_t cam_id, int8_t zoom, float fov, uint8_t crop_mode);
    void camOffsetParametersReceived(
        const QString& stream_name, uint8_t cam_id,
        float x, float y,
        float yaw_global, float pitch_global, float yaw_rel, float pitch_rel);
    void sensorParametersReceived(
        uint32_t min_exposure, uint32_t max_exposure,
        uint32_t min_gain, uint32_t max_gain,
        float target_brightness);
    void camDepthEstimationParametersReceived(
        const QString& stream_name, uint8_t cam_id, uint8_t depth_estimation_mode, float depth);
    void singleTargetTrackingParametersReceived(
        uint8_t command, const QString& stream_name, uint8_t cam_id,
        float x_offset, float y_offset,
        uint8_t detection_id, uint16_t zoom_level, float confidence,
        float yaw_global, float pitch_global,
        uint8_t rel_frame_of_reference, float yaw_rel, float pitch_rel,
        quint64 publish_timestamp_us, uint8_t status, uint8_t lock_target);
    void calibrationParametersReceived(uint8_t cam_id, uint8_t calib_command, uint8_t calib_status);
    void navigationParametersReceived(
        float altitude, float visual_lat, float visual_lon,
        float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
        float visual_vel_x, float visual_vel_y, float visual_vel_z);

private:
    template<typename Payload>
    void _encodeMessage(
        mavlink_message_t& message,
        const Payload& payload,
        uint16_t (*encodeFunction)(uint8_t, uint8_t, mavlink_message_t*, const Payload*)) const
    {
        (void) encodeFunction(_senderSystemId, _senderComponentId, &message, &payload);
    }

    void _handleMessage(const mavlink_message_t& message);
    bool _sendMessage(const mavlink_message_t& message);
    void _establishRemoteSession(uint8_t systemId, uint8_t componentId);
    void _resetRemoteSession();
    void _resetRemoteSessionForSenderIdentityChange();

    DigiviewConnection* _connection = nullptr;
    uint8_t _senderSystemId = kDefaultSenderSystemId;
    uint8_t _senderComponentId = kDefaultSenderComponentId;
    uint8_t _remoteSystemId = 0;
    uint8_t _remoteComponentId = 0;
    bool _remoteIdentityValid = false;
    bool _pendingVideoOutputParametersRequest = false;
    bool _videoOutputParametersSubscriptionActive = false;
    QString _streamName = QStringLiteral("stream");
    quint32 _lastReceivedMessageId = 0;
    bool _hasVideoOutputParameters = false;
    QString _videoOutputStreamName;
    int _videoOutputWidth = 0;
    int _videoOutputHeight = 0;
    int _videoOutputFps = 0;
    int _videoOutputLayoutMode = 0;
    int _videoOutputDetectionOverlayMode = 0;
    int _videoOutputNumUserViews = 0;
    QVariantList _videoOutputViews;
    QVariantMap _videoOutputDetectionOverlayRect;
    int _videoOutputSingleDetectionSize = 0;
};
