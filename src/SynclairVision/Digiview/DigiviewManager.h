#pragma once

#include "DigiviewConnection.h"
#include "MAVLinkEnums.h"
#include "digiview_commons/public_enums.hpp"
#include "sv_mavlink_dialect/mavlink.h"

#include <QtCore/QObject>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <QtCore/QVector>

struct CameraTrackingState {
    uint8_t sttStatus = static_cast<uint8_t>(single_target_tracking_status::OFF);
    float confidence = 0.0f;
    uint16_t trackId = 0;
    int16_t viewId = -1;
    bool lockTarget = false;
    uint8_t targetingMode = View::DIRECTIONAL;
    bool hasActiveTarget = false;
    bool hasTargetState = false;
};

class DigiviewManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(quint16 listenPort READ listenPort WRITE setListenPort NOTIFY listenPortChanged)
    Q_PROPERTY(quint16 legacyTcpControlPort READ legacyTcpControlPort WRITE setLegacyTcpControlPort NOTIFY
               legacyTcpControlPortChanged)
    Q_PROPERTY(QString streamName READ streamName WRITE setStreamName NOTIFY streamNameChanged)
    Q_PROPERTY(int senderSystemId READ senderSystemId WRITE setSenderSystemId NOTIFY senderIdentityChanged)
    Q_PROPERTY(int senderComponentId READ senderComponentId WRITE setSenderComponentId NOTIFY senderIdentityChanged)
    Q_PROPERTY(bool connected READ uiConnected NOTIFY sessionActiveChanged)
    Q_PROPERTY(bool transportConnected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool sessionActive READ sessionActive NOTIFY sessionActiveChanged)
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
    Q_PROPERTY(float detectionScanConfidenceThreshold READ detectionScanConfidenceThreshold NOTIFY
               detectionScanConfidenceThresholdChanged)
    Q_PROPERTY(float detectionScanBoxOverlap READ detectionScanBoxOverlap NOTIFY detectionScanBoxOverlapChanged)
    Q_PROPERTY(int detectionCreationScoreScale READ detectionCreationScoreScale NOTIFY detectionCreationScoreScaleChanged)
    Q_PROPERTY(int detectionBonusDetectionScale READ detectionBonusDetectionScale NOTIFY detectionBonusDetectionScaleChanged)
    Q_PROPERTY(int detectionBonusRedetectionScale READ detectionBonusRedetectionScale NOTIFY
               detectionBonusRedetectionScaleChanged)
    Q_PROPERTY(int detectionMissedDetectionPenalty READ detectionMissedDetectionPenalty NOTIFY
               detectionMissedDetectionPenaltyChanged)
    Q_PROPERTY(int detectionMissedRedetectionPenalty READ detectionMissedRedetectionPenalty NOTIFY
               detectionMissedRedetectionPenaltyChanged)
    Q_PROPERTY(QVariantList cameraStates READ cameraStates NOTIFY cameraStatesChanged)
    Q_PROPERTY(bool hasAIParameters READ hasAIParameters NOTIFY hasAIParametersChanged)
    Q_PROPERTY(bool aiEnabled READ aiEnabled NOTIFY aiEnabledChanged)
    Q_PROPERTY(QString selectedScanModel READ selectedScanModel NOTIFY selectedScanModelChanged)
    Q_PROPERTY(QStringList availableScanModels READ availableScanModels NOTIFY availableScanModelsChanged)
    Q_PROPERTY(bool aiModelDiscoveryLoading READ aiModelDiscoveryLoading NOTIFY aiModelDiscoveryLoadingChanged)
    Q_PROPERTY(bool aiModelDiscoveryReady READ aiModelDiscoveryReady NOTIFY aiModelDiscoveryReadyChanged)
    Q_PROPERTY(bool restartBusy READ restartBusy NOTIFY restartBusyChanged)
    Q_PROPERTY(int restartProgress READ restartProgress NOTIFY restartProgressChanged)
    Q_PROPERTY(QString restartFailure READ restartFailure NOTIFY restartFailureChanged)
    Q_PROPERTY(quint64 restartGeneration READ restartGeneration NOTIFY restartGenerationChanged)

public:
    static constexpr uint8_t kDefaultSenderSystemId = 255;
    static constexpr uint8_t kDefaultSenderComponentId = MAV_COMP_ID_MISSIONPLANNER;

    static constexpr size_t kMaxCameras = 6;

    QVariantList cameraStates() const;

    explicit DigiviewManager(QObject* parent = nullptr);
    ~DigiviewManager() override;

    QString host() const;
    quint16 port() const;
    quint16 listenPort() const;
    quint16 legacyTcpControlPort() const;
    QString streamName() const { return _streamName; }
    int senderSystemId() const { return _senderSystemId; }
    int senderComponentId() const { return _senderComponentId; }
    bool hasAIParameters() const { return _hasAIParameters; }
    bool aiEnabled() const { return _aiEnabled; }
    QString selectedScanModel() const { return _selectedScanModel; }
    QStringList availableScanModels() const { return _availableScanModels; }
    bool aiModelDiscoveryLoading() const { return _aiModelDiscoveryLoading; }
    bool aiModelDiscoveryReady() const { return _aiModelDiscoveryReady; }
    bool restartBusy() const { return _restartBusy; }
    int restartProgress() const { return _restartProgress; }
    QString restartFailure() const { return _restartFailure; }
    quint64 restartGeneration() const { return _restartGeneration; }
    bool connected() const;
    bool uiConnected() const { return _connection->usingLegacyTcpControl() ? sessionActive() : connected(); }
    bool sessionActive() const { return _logicalSessionActive; }
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
    float detectionScanConfidenceThreshold() const { return _detectionScanConfidenceThreshold; }
    float detectionScanBoxOverlap() const { return _detectionScanBoxOverlap; }
    int detectionCreationScoreScale() const { return _detectionCreationScoreScale; }
    int detectionBonusDetectionScale() const { return _detectionBonusDetectionScale; }
    int detectionBonusRedetectionScale() const { return _detectionBonusRedetectionScale; }
    int detectionMissedDetectionPenalty() const { return _detectionMissedDetectionPenalty; }
    int detectionMissedRedetectionPenalty() const { return _detectionMissedRedetectionPenalty; }

    void setHost(const QString& host);
    void setPort(quint16 port);
    void setListenPort(quint16 listenPort);
    void setLegacyTcpControlPort(quint16 port);
    void setStreamName(const QString& streamName);
    void setSenderSystemId(int senderSystemId);
    void setSenderComponentId(int senderComponentId);

    Q_INVOKABLE bool connectToHost();
    Q_INVOKABLE void disconnectFromHost();

    Q_INVOKABLE void sendSystemStatusParameters(uint8_t status, uint8_t error, float jetson_temp);
    Q_INVOKABLE bool sendAIParameters(uint8_t run_ai, QString scan_model_name);
    Q_INVOKABLE bool sendModelParameters(QString model_name);
    Q_INVOKABLE bool setVideoOutputLayout(int layoutMode);
    Q_INVOKABLE bool setDetectionOverlayMode(int detectionOverlayMode);
    Q_INVOKABLE bool setDetectionTracking(int cam, int view_id, bool lock_target);
    Q_INVOKABLE bool clearDetectionTracking(int cam);
    Q_INVOKABLE bool requestSystemStatusParameters();
    Q_INVOKABLE bool requestModelParameters();
    Q_INVOKABLE bool requestVideoOutputParameters();
    Q_INVOKABLE bool requestCaptureParameters();
    Q_INVOKABLE bool requestSensorParameters();
    Q_INVOKABLE bool requestDetectionParameters();
    Q_INVOKABLE bool requestTrackedDetectionParameters();
    Q_INVOKABLE bool requestCalibrationParameters(int cameraId);
    Q_INVOKABLE bool sendCaptureParameters(
        QString stream_name, uint8_t cap_single_image, uint8_t record_video,
        uint16_t images_captured, uint16_t videos_captured);
    Q_INVOKABLE void sendDetectionParameters(
        uint8_t mode, uint8_t sorting_mode,
        float scan_confidence_threshold, float scan_box_overlap,
        uint8_t creation_score_scale, uint8_t bonus_detection_scale,
        uint8_t bonus_redetection_scale, uint8_t missed_detection_penalty,
        uint8_t missed_redetection_penalty);
    Q_INVOKABLE bool sendTrackedDetectionParameters(
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
    Q_INVOKABLE bool sendCamOffsetParameters(
        QString stream_name, uint8_t cam_id,
        float x, float y,
        float yaw_global, float pitch_global, float yaw_rel, float pitch_rel);
    Q_INVOKABLE void sendSensorParameters(
        uint32_t min_exposure, uint32_t max_exposure,
        uint32_t min_gain, uint32_t max_gain,
        float target_brightness);
    Q_INVOKABLE bool sendCamDepthEstimationParameters(
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
    Q_INVOKABLE bool sendCalibrationParameters(int cameraId, int calibrationCommand);
    Q_INVOKABLE bool sendNavigationParameters(
        float altitude, float visual_lat, float visual_lon,
        float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
        float visual_vel_x, float visual_vel_y, float visual_vel_z);

    Q_INVOKABLE bool requestSingleTargetTrackingParameters();
    Q_INVOKABLE bool applyAndRestart(
        const QVariantMap& videoOutputOverlay, int width, int height, bool aiEnabled, const QString& model);


    //////////////////////////////////////////////////////////
    ///////////// Helper-functions ///////////////////////////
    //////////////////////////////////////////////////////////

    Q_INVOKABLE void changeEuler(int camId, float yaw, float pitch);
    Q_INVOKABLE void changeZoom(int camId, float zoom);
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE bool takePhoto();

signals:
    void hostChanged();
    void portChanged();
    void listenPortChanged();
    void legacyTcpControlPortChanged();
    void streamNameChanged();
    void senderIdentityChanged();
    void connectedChanged();
    void sessionActiveChanged();
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
    void detectionScanConfidenceThresholdChanged();
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
        float scan_confidence_threshold, float scan_box_overlap,
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
    void calibrationParametersReceived(
        uint8_t cam_id, uint8_t calib_command, uint8_t calib_status,
        uint8_t completed_face_mask, uint8_t mag_progress_percent);
    void navigationParametersReceived(
        float altitude, float visual_lat, float visual_lon,
        float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
        float visual_vel_x, float visual_vel_y, float visual_vel_z);
    void cameraStatesChanged();
    void hasAIParametersChanged();
    void aiEnabledChanged();
    void selectedScanModelChanged();
    void availableScanModelsChanged();
    void aiModelDiscoveryLoadingChanged();
    void aiModelDiscoveryReadyChanged();
    void commandRejected(const QString& reason);
    void restartBusyChanged();
    void restartProgressChanged();
    void restartFailureChanged();
    void restartGenerationChanged();

private:
#ifdef QGC_UNITTEST_BUILD
    friend class DigiviewManagerTest;
#endif

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
    bool _requestParameters(uint32_t messageId, float parameter3 = 0.0F, bool* pendingRequest = nullptr);
    bool _sendVideoOutputUpdate(std::optional<uint8_t> layoutMode, std::optional<uint8_t> detectionOverlayMode);
    bool _sendVideoOutputParameters(const mavlink_video_output_parameters_t& payload);
    bool _rejectUnsupportedSet(const QString& parameterName);
    bool _sendCamTargetingParameters(const mavlink_cam_targeting_parameters_t& payload);
    bool _sendSingleTargetTrackingParameters(const mavlink_single_target_tracking_parameters_t& payload);
    void _rememberCamTargeting(const mavlink_cam_targeting_parameters_t& payload);
    void _rememberSingleTargetTracking(const mavlink_single_target_tracking_parameters_t& payload);
    void _rememberInboundCamTargeting(const mavlink_cam_targeting_parameters_t& payload);
    void _rememberInboundSingleTargetTracking(const mavlink_single_target_tracking_parameters_t& payload);
    void _establishRemoteSession(uint8_t systemId, uint8_t componentId);
    void _resetRemoteSession();
    void _resetRemoteSessionForSenderIdentityChange();
    void _videoOutputTransactionTimedOut();
    void _aiTransactionTimedOut();
    void _requestAiAuthoritativeState();
    void _finishAiModelDiscovery();
    void _aiModelDiscoveryTimedOut();
    void _restartVideoConfirmed();
    void _restartAiConfirmed();
    void _restartObservationTimedOut();
    void _restartReconnect();
    void _restartCheckRefreshedState();
    void _setRestartProgress(int progress);
    void _finishRestart(bool success, const QString& failure = {});
    void _cancelRestartForSessionChange();
    bool _trafficEligible() const { return _logicalSessionActive && _connection->connected(); }
    void _reapplyEndpointIfSessionActive();

    struct VideoOutputLayoutSnapshot {
        std::optional<uint16_t> width;
        std::optional<uint16_t> height;
        uint8_t layoutMode = 0;
        uint8_t detectionOverlayMode = 0;
        uint8_t numUserViews = 0;

        bool operator==(const VideoOutputLayoutSnapshot&) const = default;
    };

    struct VideoOutputTransaction {
        quint64 generation = 0;
        VideoOutputLayoutSnapshot requested;
        QDeadlineTimer deadline;
        bool awaitingAuthoritativeState = false;
        bool stateGetIssued = false;
    };

    struct AiTransaction {
        quint64 generation = 0;
        quint64 sessionGeneration = 0;
        bool enabled = false;
        QString model;
        QDeadlineTimer deadline;
        bool awaitingAuthoritativeState = false;
        bool stateGetIssued = false;
    };

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
    bool _automaticReconnectAllowed = true;
    bool _remoteIdentityValid = false;
    bool _pendingVideoOutputParametersRequest = false;
    bool _pendingSensorParametersRequest = true;
    bool _pendingDetectionParametersRequest = true;
    bool _pendingSingleTargetTrackingParametersRequest = true;
    std::optional<VideoOutputTransaction> _videoOutputTransaction;
    QTimer _videoOutputTransactionTimer;
    quint64 _nextVideoOutputTransactionGeneration = 0;
    quint64 _videoOutputTransactionTimerGeneration = 0;
    std::optional<AiTransaction> _aiTransaction;
    QTimer _aiTransactionTimer;
    QTimer _aiVerificationGetTimer;
    QTimer _aiModelDiscoverySettleTimer;
    QTimer _aiModelDiscoveryDeadlineTimer;
    bool _aiVerificationGetPending = false;
    quint64 _nextAiTransactionGeneration = 0;
    quint64 _aiTransactionTimerGeneration = 0;
    quint64 _remoteSessionGeneration = 0;
    QTimer _restartObservationTimer;
    quint64 _restartGeneration = 0;
    bool _restartBusy = false;
    int _restartProgress = 0;
    QString _restartFailure;
    bool _expectedRestartArmed = false;
    bool _restartDownObserved = false;
    bool _restartQuitSent = false;
    QTimer _restartReconnectTimer;
    quint64 _restartReconnectGeneration = 0;
    int _restartReconnectAttempts = 0;
    mavlink_video_output_parameters_t _stagedVideoOutput {};
    bool _stagedAiEnabled = false;
    QString _stagedModel;
    QElapsedTimer _unexpectedHeartbeatWarningTimer;
    QString _streamName = QStringLiteral("stream");
    quint32 _lastReceivedMessageId = 0;
    bool _hasVideoOutputParameters = false;
    QString _videoOutputStreamName;
    int _videoOutputWidth = 0;
    int _videoOutputHeight = 0;
    int _videoOutputFps = 0;
    int _videoOutputLayoutMode = Layout::LAYOUT_1;
    int _videoOutputDetectionOverlayMode = Layout::DET_OVERLAY_NONE;
    int _videoOutputNumUserViews = 0;
    QVariantList _videoOutputViews;
    QVariantMap _videoOutputDetectionOverlayRect;
    int _videoOutputSingleDetectionSize = 0;
    mavlink_video_output_parameters_t _videoOutputParameters {};

    bool _hasAIParameters = false;
    bool _aiEnabled = false;
    QString _selectedScanModel;
    QStringList _availableScanModels;
    bool _aiModelDiscoveryLoading = false;
    bool _aiModelDiscoveryReady = false;

    bool _hasSensorParameters = false;
    quint32 _sensorMinExposure = 0;
    quint32 _sensorMaxExposure = 0;
    quint32 _sensorMinGain = 0;
    quint32 _sensorMaxGain = 0;
    float _sensorTargetBrightness = 0.0f;

    bool _hasDetectionParameters = false;
    uint8_t _detectionMode = 0;
    uint8_t _detectionSortingMode = 0;
    float _detectionScanConfidenceThreshold = 0.0f;
    float _detectionScanBoxOverlap = 0.0f;
    uint8_t _detectionCreationScoreScale = 0;
    uint8_t _detectionBonusDetectionScale = 0;
    uint8_t _detectionBonusRedetectionScale = 0;
    uint8_t _detectionMissedDetectionPenalty = 0;
    uint8_t _detectionMissedRedetectionPenalty = 0;

    std::array<CameraTrackingState, kMaxCameras> _cameraStates{};
    std::array<ActiveTarget, kMaxCameras> _activeTargets{};
};
