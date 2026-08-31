#pragma once

#include "DigiviewConnection.h"
#include "MAVLinkEnums.h"
#include "sv_mavlink_dialect/sv_mavlink_dialect.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

#include <array>
#include <cstddef>
#include <cstdint>

#include <QtCore/QVector>

struct CameraTrackingState {
    uint8_t sttStatus = 0;       // SV_STT_STATUS_OFF, RUNNING, etc.
    float confidence = 0.0f;
    uint16_t trackId = 0;
    int16_t viewId = -1;
    bool lockTarget = false;
    uint8_t targetingMode = 0;
    bool hasActiveTarget = false;
    bool hasTargetState = false;
};

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
    Q_PROPERTY(bool hasSensorParameters READ hasSensorParameters NOTIFY hasSensorParametersChanged)
    Q_PROPERTY(quint32 sensorMinExposure READ sensorMinExposure NOTIFY sensorMinExposureChanged)
    Q_PROPERTY(quint32 sensorMaxExposure READ sensorMaxExposure NOTIFY sensorMaxExposureChanged)
    Q_PROPERTY(quint32 sensorMinGain READ sensorMinGain NOTIFY sensorMinGainChanged)
    Q_PROPERTY(quint32 sensorMaxGain READ sensorMaxGain NOTIFY sensorMaxGainChanged)
    Q_PROPERTY(float sensorTargetBrightness READ sensorTargetBrightness NOTIFY sensorTargetBrightnessChanged)
    Q_PROPERTY(bool hasDetectionParameters READ hasDetectionParameters NOTIFY hasDetectionParametersChanged)
    Q_PROPERTY(int detectionMode READ detectionMode NOTIFY detectionModeChanged)
    Q_PROPERTY(int detectionSortingMode READ detectionSortingMode NOTIFY detectionSortingModeChanged)
    Q_PROPERTY(float detectionTrackConfidenceThreshold READ detectionTrackConfidenceThreshold NOTIFY
               detectionTrackConfidenceThresholdChanged)
    Q_PROPERTY(float detectionScanConfidenceThreshold READ detectionScanConfidenceThreshold NOTIFY
               detectionScanConfidenceThresholdChanged)
    Q_PROPERTY(float detectionTrackBoxOverlap READ detectionTrackBoxOverlap NOTIFY detectionTrackBoxOverlapChanged)
    Q_PROPERTY(float detectionScanBoxOverlap READ detectionScanBoxOverlap NOTIFY detectionScanBoxOverlapChanged)
    Q_PROPERTY(int detectionCreationScoreScale READ detectionCreationScoreScale NOTIFY detectionCreationScoreScaleChanged)
    Q_PROPERTY(int detectionBonusDetectionScale READ detectionBonusDetectionScale NOTIFY detectionBonusDetectionScaleChanged)
    Q_PROPERTY(int detectionBonusRedetectionScale READ detectionBonusRedetectionScale NOTIFY
               detectionBonusRedetectionScaleChanged)
    Q_PROPERTY(int detectionMissedDetectionPenalty READ detectionMissedDetectionPenalty NOTIFY
               detectionMissedDetectionPenaltyChanged)
    Q_PROPERTY(int detectionMissedRedetectionPenalty READ detectionMissedRedetectionPenalty NOTIFY
               detectionMissedRedetectionPenaltyChanged)
    Q_PROPERTY(bool hasSttParameters READ hasSttParameters NOTIFY hasSttParametersChanged)
    Q_PROPERTY(int sttStatus READ sttStatus NOTIFY sttStatusChanged)
    Q_PROPERTY(int sttCamId READ sttCamId NOTIFY sttCamIdChanged)
    Q_PROPERTY(float sttConfidence READ sttConfidence NOTIFY sttConfidenceChanged)
    Q_PROPERTY(bool sttLockTarget READ sttLockTarget NOTIFY sttLockTargetChanged)
    Q_PROPERTY(QVariantList cameraStates READ cameraStates NOTIFY cameraStatesChanged)

public:
    static constexpr uint8_t kDefaultSenderSystemId = 255;
    static constexpr uint8_t kDefaultSenderComponentId = MAV_COMP_ID_MISSIONPLANNER;

    static constexpr size_t kMaxCameras = 6;

    QVariantList cameraStates() const;

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
    bool hasSensorParameters() const { return _hasSensorParameters; }
    quint32 sensorMinExposure() const { return _sensorMinExposure; }
    quint32 sensorMaxExposure() const { return _sensorMaxExposure; }
    quint32 sensorMinGain() const { return _sensorMinGain; }
    quint32 sensorMaxGain() const { return _sensorMaxGain; }
    float sensorTargetBrightness() const { return _sensorTargetBrightness; }
    bool hasDetectionParameters() const { return _hasDetectionParameters; }
    int detectionMode() const { return _detectionMode; }
    int detectionSortingMode() const { return _detectionSortingMode; }
    float detectionTrackConfidenceThreshold() const { return _detectionTrackConfidenceThreshold; }
    float detectionScanConfidenceThreshold() const { return _detectionScanConfidenceThreshold; }
    float detectionTrackBoxOverlap() const { return _detectionTrackBoxOverlap; }
    float detectionScanBoxOverlap() const { return _detectionScanBoxOverlap; }
    int detectionCreationScoreScale() const { return _detectionCreationScoreScale; }
    int detectionBonusDetectionScale() const { return _detectionBonusDetectionScale; }
    int detectionBonusRedetectionScale() const { return _detectionBonusRedetectionScale; }
    int detectionMissedDetectionPenalty() const { return _detectionMissedDetectionPenalty; }
    int detectionMissedRedetectionPenalty() const { return _detectionMissedRedetectionPenalty; }

    void setHost(const QString& host);
    void setPort(quint16 port);
    void setListenPort(quint16 listenPort);
    void setStreamName(const QString& streamName);
    void setSenderSystemId(int senderSystemId);
    void setSenderComponentId(int senderComponentId);

    Q_INVOKABLE bool connectToHost();
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void disconnectFromHost(bool preventAutomaticReconnect);

    Q_INVOKABLE void sendSystemStatusParameters(uint8_t status, uint8_t error, float jetson_temp);
    Q_INVOKABLE void sendAIParameters(uint8_t run_ai, QString scan_model_name);
    Q_INVOKABLE void sendModelParameters(QString model_name);
    Q_INVOKABLE void sendSetVideoOutput(
        QString stream_name, uint16_t width, uint16_t height, uint8_t fps,
        uint8_t layout, uint8_t detection_overlay_mode);
    Q_INVOKABLE bool setDetectionTracking(int cam, int view_id, bool lock_target);
    Q_INVOKABLE bool clearDetectionTracking(int cam);
    Q_INVOKABLE void requestVideoOutputParameters();
    Q_INVOKABLE void requestSensorParameters();
    Q_INVOKABLE void requestDetectionParameters();
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
    Q_INVOKABLE bool sendCamTargetingParameters(
        QString stream_name, uint8_t cam_id, uint8_t targeting_mode, uint8_t euler_delta,
        float yaw, float pitch, float roll, uint8_t lock_flags,
        float x_offset, float y_offset,
        float target_latitude, float target_longitude, float target_altitude,
        uint16_t track_id, int16_t view_id, uint8_t lock_target);
    Q_INVOKABLE void sendCamOpticsAndControlParameters(
        QString stream_name, uint8_t cam_id, int8_t zoom, float fov);
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
    Q_INVOKABLE bool sendSingleTargetTrackingParameters(
        uint8_t command, QString stream_name, uint8_t cam_id,
        float x_offset, float y_offset,
        uint8_t detection_id, uint16_t zoom_level, float confidence,
        float yaw_global, float pitch_global,
        uint8_t rel_frame_of_reference, float yaw_rel, float pitch_rel,
        quint64 publish_timestamp_us, uint8_t status, uint8_t lock_target);
    Q_INVOKABLE bool setSingleTargetTrackingTarget(int cam_id, float x_offset, float y_offset);
    Q_INVOKABLE bool setCameraCursorTarget(int cam_id, float x_offset, float y_offset);
    Q_INVOKABLE bool setCameraManualTarget(int cam_id, float latitude, float longitude, float altitude);
    Q_INVOKABLE bool stopSingleTargetTracking(int cam_id);
    Q_INVOKABLE bool lockCurrentTarget(int cameraSlot);
    Q_INVOKABLE bool clearCurrentTarget(int cameraSlot);
    Q_INVOKABLE void sendCalibrationParameters(
        uint8_t cam_id, uint8_t calib_command, uint8_t calib_status);
    Q_INVOKABLE void sendNavigationParameters(
        float altitude, float visual_lat, float visual_lon,
        float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
        float visual_vel_x, float visual_vel_y, float visual_vel_z);

    bool hasSttParameters() const { return _hasSttParameters; }
    int sttStatus() const { return _sttStatus; }
    int sttCamId() const { return _sttCamId; }
    float sttConfidence() const { return _sttConfidence; }
    bool sttLockTarget() const { return _sttLockTarget != 0; }

    Q_INVOKABLE void requestSingleTargetTrackingParameters();


    //////////////////////////////////////////////////////////
    ///////////// Helper-functions ///////////////////////////
    //////////////////////////////////////////////////////////

    Q_INVOKABLE void changeEuler(int camId, float yaw, float pitch);
    Q_INVOKABLE void changeZoom(int camId, float zoom);
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void takePhoto();

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
    void hasSensorParametersChanged();
    void sensorMinExposureChanged();
    void sensorMaxExposureChanged();
    void sensorMinGainChanged();
    void sensorMaxGainChanged();
    void sensorTargetBrightnessChanged();
    void sensorParametersChanged();
    void hasDetectionParametersChanged();
    void detectionModeChanged();
    void detectionSortingModeChanged();
    void detectionTrackConfidenceThresholdChanged();
    void detectionScanConfidenceThresholdChanged();
    void detectionTrackBoxOverlapChanged();
    void detectionScanBoxOverlapChanged();
    void detectionCreationScoreScaleChanged();
    void detectionBonusDetectionScaleChanged();
    void detectionBonusRedetectionScaleChanged();
    void detectionMissedDetectionPenaltyChanged();
    void detectionMissedRedetectionPenaltyChanged();
    void detectionParametersChanged();
    void messageDecoded(quint32 messageId);
    void systemStatusParametersReceived(uint8_t status, uint8_t error, float jetson_temp);
    void aiParametersReceived(uint8_t run_ai, const QString& scan_model_name);
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
        const QString& stream_name, uint8_t cam_id, int8_t zoom, float fov);
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
    void hasSttParametersChanged();
    void sttStatusChanged();
    void sttCamIdChanged();
    void sttConfidenceChanged();
    void sttLockTargetChanged();
    void cameraStatesChanged();

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
    bool _sendCamTargetingParameters(const mavlink_cam_targeting_parameters_t& payload);
    bool _sendSingleTargetTrackingParameters(const mavlink_single_target_tracking_parameters_t& payload);
    void _rememberCamTargeting(const mavlink_cam_targeting_parameters_t& payload);
    void _rememberSingleTargetTracking(const mavlink_single_target_tracking_parameters_t& payload);
    void _rememberInboundCamTargeting(const mavlink_cam_targeting_parameters_t& payload);
    void _rememberInboundSingleTargetTracking(const mavlink_single_target_tracking_parameters_t& payload);
    void _establishRemoteSession(uint8_t systemId, uint8_t componentId);
    void _resetRemoteSession();
    void _resetRemoteSessionForSenderIdentityChange();

    struct ActiveTarget {
        enum class Type : uint8_t {
            None,
            CamTargeting,
            SingleTargetTracking,
            PendingDetection,
            Detection,
        };

        Type type = Type::None;
        mavlink_cam_targeting_parameters_t camTargeting {};
        mavlink_single_target_tracking_parameters_t singleTargetTracking {};
    };

    DigiviewConnection* _connection = nullptr;
    uint8_t _senderSystemId = kDefaultSenderSystemId;
    uint8_t _senderComponentId = kDefaultSenderComponentId;
    uint8_t _remoteSystemId = 0;
    uint8_t _remoteComponentId = 0;
    bool _logicalSessionActive = false;
    bool _remoteIdentityValid = false;
    bool _remoteComponentPinnedByVideoOutputParameters = false;
    bool _pendingVideoOutputParametersRequest = false;
    bool _pendingSensorParametersRequest = true;
    bool _pendingDetectionParametersRequest = true;
    bool _pendingSingleTargetTrackingParametersRequest = true;
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

    bool _hasSensorParameters = false;
    quint32 _sensorMinExposure = 0;
    quint32 _sensorMaxExposure = 0;
    quint32 _sensorMinGain = 0;
    quint32 _sensorMaxGain = 0;
    float _sensorTargetBrightness = 0.0f;

    bool _hasDetectionParameters = false;
    uint8_t _detectionMode = 0;
    uint8_t _detectionSortingMode = 0;
    float _detectionTrackConfidenceThreshold = 0.0f;
    float _detectionScanConfidenceThreshold = 0.0f;
    float _detectionTrackBoxOverlap = 0.0f;
    float _detectionScanBoxOverlap = 0.0f;
    uint8_t _detectionCreationScoreScale = 0;
    uint8_t _detectionBonusDetectionScale = 0;
    uint8_t _detectionBonusRedetectionScale = 0;
    uint8_t _detectionMissedDetectionPenalty = 0;
    uint8_t _detectionMissedRedetectionPenalty = 0;

    bool _hasSttParameters = false;
    uint8_t _sttStatus = 0; // 0=OFF, 1=INIT, 2=RUNNING, 3=DROPPED
    uint8_t _sttCamId = 0;
    float _sttConfidence = 0.0f;
    uint8_t _sttLockTarget = 0;
    std::array<CameraTrackingState, kMaxCameras> _cameraStates{};
    std::array<ActiveTarget, kMaxCameras> _activeTargets{};
};
