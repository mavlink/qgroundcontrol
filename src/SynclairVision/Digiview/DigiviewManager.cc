#include "DigiviewManager.h"

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "sv_mavlink_dialect/sv_mavlink_dialect.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>

#include <cmath>
#include <cstring>
#include <limits>

QGC_LOGGING_CATEGORY(DigiviewManagerLog, "Digiview.Manager")

Q_APPLICATION_STATIC(DigiviewManager, _digiviewManagerInstance);

namespace {

constexpr uint8_t kCamTargetingLockFlagsUnchanged = 0xFF;
constexpr uint8_t kCamTargetingLockFlagsAll = 0x07;
constexpr float kOneShotIntervalUs = -1000.0F;
constexpr float kVideoOutputParametersSubscriptionIntervalUs = 100000.0F;
constexpr int kVideoOutputTransactionTimeoutMs = 2000;
constexpr uint8_t kDigiviewSystemId = 252;
constexpr uint8_t kDigiviewComponentId = 66;

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
    case Layout::LAYOUT_1:
        return 1;
    case Layout::LAYOUT_2_COLUMNS:
        return 2;
    case Layout::LAYOUT_2_ROWS:
        return 2;
    case Layout::LAYOUT_TOP_2_BOTTOM_1:
        return 3;
    case Layout::LAYOUT_TOP_2_BOTTOM_2:
        return 4;
    case Layout::LAYOUT_TOP_3_BOTTOM_1:
        return 4;
    case Layout::LAYOUT_SOURCE_FRAME:
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
    connect(_connection, &DigiviewConnection::legacyTcpControlPortChanged,
            this, &DigiviewManager::legacyTcpControlPortChanged);
    connect(_connection, &DigiviewConnection::connectedChanged, this, &DigiviewManager::connectedChanged);
    connect(_connection, &DigiviewConnection::connectedChanged, this, [this] {
        if (!_connection->connected() && _logicalSessionActive && _remoteIdentityValid) {
            _resetRemoteSession();
        }
    });
    connect(_connection, &DigiviewConnection::lastErrorChanged, this, &DigiviewManager::lastErrorChanged);
    connect(_connection, &DigiviewConnection::messageReceived, this, &DigiviewManager::_handleMessage);
    connect(&_videoOutputTransactionTimer, &QTimer::timeout,
            this, &DigiviewManager::_videoOutputTransactionTimedOut);
    _videoOutputTransactionTimer.setSingleShot(true);

    if (qApp) {
        connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { disconnectFromHost(); }, Qt::QueuedConnection);
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

quint16 DigiviewManager::legacyTcpControlPort() const
{
    return _connection->legacyTcpControlPort();
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

void DigiviewManager::setLegacyTcpControlPort(quint16 port)
{
    if (port != _connection->legacyTcpControlPort()) {
        _resetRemoteSession();
    }

    _connection->setLegacyTcpControlPort(port);
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
    _logicalSessionActive = false;
    _resetRemoteSession();
    _logicalSessionActive = _connection->connectToEndpoint();
    return _logicalSessionActive;
}

void DigiviewManager::disconnectFromHost()
{
    disconnectFromHost(false);
}

void DigiviewManager::disconnectFromHost(bool preventAutomaticReconnect)
{
    _logicalSessionActive = false;
    _resetRemoteSession();
    _connection->disconnectFromEndpoint(preventAutomaticReconnect);
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

void DigiviewManager::sendAIParameters(uint8_t run_ai, QString scan_model_name)
{
    mavlink_message_t msg;
    mavlink_ai_parameters_t payload {};

    payload.run_ai = run_ai;
    copyStringToCharBuf(scan_model_name, payload.scan_model_name, 16);

    _encodeMessage(msg, payload, mavlink_msg_ai_parameters_encode);
    _sendMessage(msg);
}

bool DigiviewManager::sendModelParameters(QString model_name)
{
    Q_UNUSED(model_name);
    return _rejectUnsupportedSet(tr("MODEL"));
}

bool DigiviewManager::setVideoOutputLayout(int layoutMode)
{
    if ((layoutMode < Layout::LAYOUT_1) || (layoutMode > Layout::LAYOUT_MAX)) {
        emit commandRejected(tr("The requested DigiView layout is invalid and was not sent."));
        return false;
    }

    return _sendVideoOutputUpdate(static_cast<uint8_t>(layoutMode), std::nullopt);
}

bool DigiviewManager::setDetectionOverlayMode(int detectionOverlayMode)
{
    if ((detectionOverlayMode < Layout::DET_OVERLAY_NONE)
        || (detectionOverlayMode > Layout::DET_OVERLAY_MAX)) {
        emit commandRejected(tr("The requested DigiView detection overlay is invalid and was not sent."));
        return false;
    }

    return _sendVideoOutputUpdate(std::nullopt, static_cast<uint8_t>(detectionOverlayMode));
}

bool DigiviewManager::_sendVideoOutputUpdate(
    std::optional<uint8_t> layoutMode, std::optional<uint8_t> detectionOverlayMode)
{
    if (_videoOutputTransaction) {
        emit commandRejected(tr("DigiView is still processing a prior video-output update. Wait for it to finish."));
        return false;
    }

    if (!_hasVideoOutputParameters) {
        emit commandRejected(
            tr("Video output parameters are not available yet. Wait for DigiView to report them before changing them."));
        return false;
    }

    // The shared UDP/TCP path sends the full validated cached payload, not undocumented partial sentinels.
    mavlink_video_output_parameters_t payload = _videoOutputParameters;
    if (layoutMode) {
        payload.layout_mode = *layoutMode;
    }
    if (detectionOverlayMode) {
        payload.detection_overlay_mode = *detectionOverlayMode;
    }
    payload.num_user_views = userViewCountForLayout(payload.layout_mode);

    ++_nextVideoOutputTransactionGeneration;
    _videoOutputTransaction = VideoOutputTransaction {
        _nextVideoOutputTransactionGeneration,
        {payload.layout_mode, payload.detection_overlay_mode, payload.num_user_views},
        QDeadlineTimer(kVideoOutputTransactionTimeoutMs),
        false,
        false,
    };
    _videoOutputTransactionTimerGeneration = _videoOutputTransaction->generation;
    _videoOutputTransactionTimer.start(kVideoOutputTransactionTimeoutMs);
    if (!_sendVideoOutputParameters(payload)) {
        _videoOutputTransactionTimer.stop();
        _videoOutputTransaction.reset();
        return false;
    }

    return true;
}

bool DigiviewManager::setDetectionTracking(
    int cam, int view_id, bool lock_target
    )

{
    const bool validCameraSlot = (cam >= 0) && (cam <= std::numeric_limits<uint8_t>::max())
        && (static_cast<size_t>(cam) < _activeTargets.size());
    const bool validViewId = (view_id >= std::numeric_limits<int16_t>::min())
        && (view_id <= std::numeric_limits<int16_t>::max())
        && (view_id >= 0) && (view_id <= std::numeric_limits<uint8_t>::max());
    if (!validCameraSlot || !validViewId) {
        return false;
    }

    const ActiveTarget previousTarget = _activeTargets[cam];

    const bool camTargetSent = sendCamTargetingParameters(
        _streamName,
        cam,
        View::DETECTION,
        true,
        0.0f,
        0.0f,
        0.0f,
        kCamTargetingLockFlagsAll,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0,
        static_cast<int16_t>(view_id),
        lock_target
    );

    // SINGLE_TARGET_TRACKING_PARAMETERS controls the STT state exposed to the UI.
    const bool sttTargetSent = sendSingleTargetTrackingParameters(
        CMD_SET_TARGET_VECTOR,
        _streamName,
        static_cast<uint8_t>(cam),
        0.0f, 0.0f,
        // TODO: Verify with middleware that view_id is a valid detection_id.
        static_cast<uint8_t>(view_id),
        0,
        0.0f, 0.0f, 0.0f,
        0, 0.0f, 0.0f,
        0,
        0,
        lock_target ? 1 : 0
    );

    if (!(camTargetSent && sttTargetSent)) {
        if (!camTargetSent && !sttTargetSent) {
            _activeTargets[cam] = previousTarget;
        }
        return false;
    }

    ActiveTarget& activeTarget = _activeTargets[cam];
    activeTarget.type = ActiveTarget::Type::Detection;
    activeTarget.camTargeting.lock_target = lock_target ? 1 : 0;
    activeTarget.singleTargetTracking.lock_target = lock_target ? 1 : 0;
    return true;
}

bool DigiviewManager::clearDetectionTracking(
    int cam
    )

{
    const bool validCameraSlot = (cam >= 0) && (cam <= std::numeric_limits<uint8_t>::max())
        && (static_cast<size_t>(cam) < _activeTargets.size());
    if (!validCameraSlot) {
        return false;
    }

    const ActiveTarget previousTarget = _activeTargets[cam];

    const bool camTargetSent = sendCamTargetingParameters(
        _streamName,
        cam,
        View::DETECTION,
        true,
        0.0f,
        0.0f,
        0.0f,
        kCamTargetingLockFlagsAll,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0,
        -1,
        0
    );

    const bool sttTargetSent = sendSingleTargetTrackingParameters(
        CMD_OFF,
        _streamName,
        static_cast<uint8_t>(cam),
        0.0f, 0.0f,
        0,
        0,
        0.0f, 0.0f, 0.0f,
        0, 0.0f, 0.0f,
        0,
        0,
        0
    );

    if (!(camTargetSent && sttTargetSent)) {
        if (!camTargetSent && !sttTargetSent) {
            _activeTargets[cam] = previousTarget;
        } else if (camTargetSent) {
            if ((previousTarget.type == ActiveTarget::Type::Detection)
                || (previousTarget.type == ActiveTarget::Type::SingleTargetTracking)) {
                _activeTargets[cam] = previousTarget;
                _activeTargets[cam].type = ActiveTarget::Type::SingleTargetTracking;
            } else {
                _activeTargets[cam] = {};
            }
        } else if (previousTarget.type == ActiveTarget::Type::Detection) {
            _activeTargets[cam] = previousTarget;
            _activeTargets[cam].type = ActiveTarget::Type::CamTargeting;
        }
        return false;
    }

    const ActiveTarget& activeTarget = _activeTargets[cam];
    if ((activeTarget.type == ActiveTarget::Type::Detection)
        || (activeTarget.type == ActiveTarget::Type::PendingDetection)
        || ((activeTarget.type == ActiveTarget::Type::CamTargeting)
            && (activeTarget.camTargeting.targeting_mode == View::DETECTION))) {
        _activeTargets[cam] = {};
    }

    return true;
}

bool DigiviewManager::_requestParameters(uint32_t messageId, float parameter3, bool* pendingRequest)
{
    if (!_remoteIdentityValid) {
        if (pendingRequest) {
            *pendingRequest = true;
        }
        qCDebug(DigiviewManagerLog) << "Deferring MAVLink one-shot GET until target HEARTBEAT"
                                    << "messageId" << messageId;
        return false;
    }

    mavlink_message_t msg;
    mavlink_command_long_t command {};

    command.target_system = _remoteSystemId;
    command.target_component = _remoteComponentId;
    command.command = MAV_CMD_SET_MESSAGE_INTERVAL;
    command.param1 = static_cast<float>(messageId);
    command.param2 = kOneShotIntervalUs;
    command.param3 = parameter3;

    _encodeMessage(msg, command, mavlink_msg_command_long_encode);
    qCDebug(DigiviewManagerLog) << "Sending one-shot MAVLink GET:"
                                << "messageId" << messageId
                                << "parameter3" << parameter3
                                << "targetSystem" << command.target_system
                                << "targetComponent" << command.target_component;
    const bool sent = _sendMessage(msg);
    if (pendingRequest) {
        *pendingRequest = !sent;
    }
    return sent;
}

bool DigiviewManager::requestSystemStatusParameters()
{
    return _requestParameters(MAVLINK_MSG_ID_SYSTEM_STATUS_PARAMETERS);
}

bool DigiviewManager::requestModelParameters()
{
    return _requestParameters(MAVLINK_MSG_ID_MODEL_PARAMETERS);
}

bool DigiviewManager::requestVideoOutputParameters()
{
    return _requestParameters(
        MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS, 0.0F, &_pendingVideoOutputParametersRequest);
}

bool DigiviewManager::requestCaptureParameters()
{
    return _requestParameters(MAVLINK_MSG_ID_CAPTURE_PARAMETERS);
}

bool DigiviewManager::requestSensorParameters()
{
    return _requestParameters(MAVLINK_MSG_ID_SENSOR_PARAMETERS, 0.0F, &_pendingSensorParametersRequest);
}

bool DigiviewManager::requestDetectionParameters()
{
    return _requestParameters(MAVLINK_MSG_ID_DETECTION_PARAMETERS, 0.0F, &_pendingDetectionParametersRequest);
}

bool DigiviewManager::requestTrackedDetectionParameters()
{
    return _requestParameters(
        MAVLINK_MSG_ID_TRACKED_DETECTION_PARAMETERS, static_cast<float>(std::numeric_limits<uint8_t>::max()));
}

bool DigiviewManager::requestCalibrationParameters(int cameraId)
{
    if ((cameraId < 0) || (cameraId > std::numeric_limits<uint8_t>::max())) {
        emit commandRejected(tr("The requested calibration camera is invalid and was not sent."));
        return false;
    }

    return _requestParameters(MAVLINK_MSG_ID_CALIBRATION_PARAMETERS, static_cast<float>(cameraId));
}

bool DigiviewManager::requestSingleTargetTrackingParameters()
{
    return _requestParameters(MAVLINK_MSG_ID_SINGLE_TARGET_TRACKING_PARAMETERS, 0.0F,
                              &_pendingSingleTargetTrackingParametersRequest);
}

bool DigiviewManager::_sendVideoOutputParameters(const mavlink_video_output_parameters_t& payload)
{
    mavlink_message_t msg;

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

    return _sendMessage(msg);
}

bool DigiviewManager::sendCaptureParameters(
    QString stream_name, uint8_t cap_single_image, uint8_t record_video,
    uint16_t images_captured, uint16_t videos_captured)
{
    if (cap_single_image != 0U) {
        emit commandRejected(tr("Still-image capture is not supported by DigiView and was not sent."));
        return false;
    }
    if (record_video > 1U) {
        emit commandRejected(tr("The requested DigiView recording state is invalid and was not sent."));
        return false;
    }

    mavlink_message_t msg;
    mavlink_capture_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cap_single_image = cap_single_image;
    payload.record_video = record_video;
    payload.images_captured = images_captured;
    payload.videos_captured = videos_captured;

    _encodeMessage(msg, payload, mavlink_msg_capture_parameters_encode);
    return _sendMessage(msg);
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

bool DigiviewManager::sendTrackedDetectionParameters(
    uint8_t index, uint8_t score, uint8_t total_detections, int16_t type,
    float yaw_global, float pitch_global, uint8_t rel_frame_of_reference,
    float yaw_rel, float pitch_rel,
    float latitude, float longitude, float altitude,
    float distance, float width, float height,
    uint16_t track_id, quint64 publish_timestamp_us, uint8_t view_id)
{
    Q_UNUSED(index);
    Q_UNUSED(score);
    Q_UNUSED(total_detections);
    Q_UNUSED(type);
    Q_UNUSED(yaw_global);
    Q_UNUSED(pitch_global);
    Q_UNUSED(rel_frame_of_reference);
    Q_UNUSED(yaw_rel);
    Q_UNUSED(pitch_rel);
    Q_UNUSED(latitude);
    Q_UNUSED(longitude);
    Q_UNUSED(altitude);
    Q_UNUSED(distance);
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(track_id);
    Q_UNUSED(publish_timestamp_us);
    Q_UNUSED(view_id);
    return _rejectUnsupportedSet(tr("TRACKED_DETECTION"));
}

bool DigiviewManager::sendCamTargetingParameters(
    QString stream_name, uint8_t cam_id, uint8_t targeting_mode, uint8_t euler_delta,
    float yaw, float pitch, float roll, uint8_t lock_flags,
    float x_offset, float y_offset,
    float target_latitude, float target_longitude, float target_altitude,
    uint16_t track_id, int16_t view_id, uint8_t lock_target)
{
    if (targeting_mode >= View::NUM_TARGETING_MODES) {
        emit commandRejected(tr("The requested DigiView targeting mode is invalid and was not sent."));
        return false;
    }

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

    const bool sent = _sendCamTargetingParameters(payload);
    if (sent) {
        _rememberCamTargeting(payload);
    }
    return sent;
}

bool DigiviewManager::_sendCamTargetingParameters(const mavlink_cam_targeting_parameters_t& payload)
{
    mavlink_message_t msg;
    _encodeMessage(msg, payload, mavlink_msg_cam_targeting_parameters_encode);
    return _sendMessage(msg);
}

void DigiviewManager::_rememberCamTargeting(const mavlink_cam_targeting_parameters_t& payload)
{
    if ((payload.cam_id >= _activeTargets.size())
        || ((payload.targeting_mode != View::COORDINAL)
            && (payload.targeting_mode != View::DETECTION))) {
        return;
    }

    ActiveTarget& activeTarget = _activeTargets[payload.cam_id];
    activeTarget.type = ActiveTarget::Type::CamTargeting;
    activeTarget.camTargeting = payload;
}

void DigiviewManager::sendCamOpticsAndControlParameters(
    QString stream_name, uint8_t cam_id, int8_t zoom, float fov)
{
    mavlink_message_t msg;
    mavlink_cam_optics_and_control_parameters_t payload {};

    copyStringToCharBuf(stream_name, payload.stream_name, 16);
    payload.cam_id = cam_id;
    payload.zoom = zoom;
    payload.fov = fov;

    _encodeMessage(msg, payload, mavlink_msg_cam_optics_and_control_parameters_encode);
    _sendMessage(msg);
}

bool DigiviewManager::sendCamOffsetParameters(
    QString stream_name, uint8_t cam_id,
    float x, float y,
    float yaw_global, float pitch_global, float yaw_rel, float pitch_rel)
{
    Q_UNUSED(stream_name);
    Q_UNUSED(cam_id);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(yaw_global);
    Q_UNUSED(pitch_global);
    Q_UNUSED(yaw_rel);
    Q_UNUSED(pitch_rel);
    return _rejectUnsupportedSet(tr("CAM_OFFSET"));
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

bool DigiviewManager::sendCamDepthEstimationParameters(
    QString stream_name, uint8_t cam_id, uint8_t depth_estimation_mode, float depth)
{
    Q_UNUSED(stream_name);
    Q_UNUSED(cam_id);
    Q_UNUSED(depth_estimation_mode);
    Q_UNUSED(depth);
    return _rejectUnsupportedSet(tr("DEPTH"));
}

bool DigiviewManager::sendSingleTargetTrackingParameters(
    uint8_t command, QString stream_name, uint8_t cam_id,
    float x_offset, float y_offset,
    uint8_t detection_id, uint16_t zoom_level, float confidence,
    float yaw_global, float pitch_global,
    uint8_t rel_frame_of_reference, float yaw_rel, float pitch_rel,
    quint64 publish_timestamp_us, uint8_t status, uint8_t lock_target)
{
    if ((command > CMD_NONE)
        || (status > static_cast<uint8_t>(single_target_tracking_status::DROPPED))) {
        emit commandRejected(tr("The requested DigiView tracking state is invalid and was not sent."));
        return false;
    }

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

    const bool sent = _sendSingleTargetTrackingParameters(payload);
    if (sent) {
        _rememberSingleTargetTracking(payload);
    }
    return sent;
}

bool DigiviewManager::_sendSingleTargetTrackingParameters(const mavlink_single_target_tracking_parameters_t& payload)
{
    mavlink_message_t msg;
    _encodeMessage(msg, payload, mavlink_msg_single_target_tracking_parameters_encode);
    return _sendMessage(msg);
}

void DigiviewManager::_rememberSingleTargetTracking(const mavlink_single_target_tracking_parameters_t& payload)
{
    if (payload.cam_id >= _activeTargets.size()) {
        return;
    }

    ActiveTarget& activeTarget = _activeTargets[payload.cam_id];
    if (payload.command == CMD_OFF) {
        if ((activeTarget.type == ActiveTarget::Type::SingleTargetTracking)
            || (activeTarget.type == ActiveTarget::Type::Detection)) {
            activeTarget = {};
        }
        return;
    }

    activeTarget.type = ActiveTarget::Type::SingleTargetTracking;
    activeTarget.singleTargetTracking = payload;
}

void DigiviewManager::_rememberInboundCamTargeting(const mavlink_cam_targeting_parameters_t& payload)
{
    if (payload.cam_id >= _activeTargets.size()) {
        return;
    }

    ActiveTarget& activeTarget = _activeTargets[payload.cam_id];
    if (payload.targeting_mode == View::DIRECTIONAL) {
        activeTarget = {};
        return;
    }

    if (payload.targeting_mode == View::COORDINAL) {
        activeTarget.type = ActiveTarget::Type::CamTargeting;
        activeTarget.camTargeting = payload;
        return;
    }

    if (payload.targeting_mode != View::DETECTION) {
        return;
    }

    if (((activeTarget.type == ActiveTarget::Type::SingleTargetTracking)
         || (activeTarget.type == ActiveTarget::Type::Detection))
        && (activeTarget.singleTargetTracking.command == CMD_SET_TARGET_VECTOR)
        && (payload.view_id >= 0)
        && (payload.view_id <= std::numeric_limits<uint8_t>::max())
        && (static_cast<uint8_t>(payload.view_id) == activeTarget.singleTargetTracking.detection_id)) {
        activeTarget.type = ActiveTarget::Type::Detection;
        activeTarget.camTargeting = payload;
        return;
    }

    activeTarget = {};
    activeTarget.type = ActiveTarget::Type::PendingDetection;
    activeTarget.camTargeting = payload;
}

void DigiviewManager::_rememberInboundSingleTargetTracking(
    const mavlink_single_target_tracking_parameters_t& payload)
{
    if (payload.cam_id >= _activeTargets.size()) {
        return;
    }

    ActiveTarget& activeTarget = _activeTargets[payload.cam_id];
    if ((payload.command == CMD_OFF)
        || (payload.status == static_cast<uint8_t>(single_target_tracking_status::OFF))
        || (payload.status == static_cast<uint8_t>(single_target_tracking_status::DROPPED))) {
        if (activeTarget.type != ActiveTarget::Type::CamTargeting
            || (activeTarget.camTargeting.targeting_mode != View::COORDINAL)) {
            activeTarget = {};
        }
        return;
    }

    if ((activeTarget.type == ActiveTarget::Type::Detection)
        && (activeTarget.camTargeting.view_id >= 0)
        && (activeTarget.camTargeting.view_id <= std::numeric_limits<uint8_t>::max())
        && (payload.command == CMD_SET_TARGET_VECTOR)
        && (static_cast<uint8_t>(activeTarget.camTargeting.view_id) == payload.detection_id)) {
        activeTarget.singleTargetTracking = payload;
        return;
    }

    if ((activeTarget.type == ActiveTarget::Type::PendingDetection)
        && (payload.command == CMD_SET_TARGET_VECTOR)
        && (activeTarget.camTargeting.view_id >= 0)
        && (activeTarget.camTargeting.view_id <= std::numeric_limits<uint8_t>::max())
        && (static_cast<uint8_t>(activeTarget.camTargeting.view_id) == payload.detection_id)) {
        activeTarget.type = ActiveTarget::Type::Detection;
        activeTarget.singleTargetTracking = payload;
        return;
    }

    activeTarget.type = ActiveTarget::Type::SingleTargetTracking;
    activeTarget.singleTargetTracking = payload;
}

bool DigiviewManager::setSingleTargetTrackingTarget(int camId, float xOffset, float yOffset)
{
    if ((camId < 0) || (camId > std::numeric_limits<uint8_t>::max())
        || !std::isfinite(xOffset) || !std::isfinite(yOffset)
        || (xOffset < -1.0f) || (xOffset > 1.0f)
        || (yOffset < -1.0f) || (yOffset > 1.0f)) {
        return false;
    }

    return sendSingleTargetTrackingParameters(
        CMD_SET_TARGET_VECTOR,
        _streamName,
        static_cast<uint8_t>(camId),
        xOffset,
        yOffset,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        0,
        0.0f,
        0.0f,
        0,
        0,
        0);
}

bool DigiviewManager::setCameraCursorTarget(int camId, float xOffset, float yOffset)
{
    if ((camId < 0) || (camId > std::numeric_limits<uint8_t>::max())
        || !std::isfinite(xOffset) || !std::isfinite(yOffset)
        || (xOffset < -1.0f) || (xOffset > 1.0f)
        || (yOffset < -1.0f) || (yOffset > 1.0f)) {
        return false;
    }

    return sendCamTargetingParameters(
        _streamName,
        static_cast<uint8_t>(camId),
        View::COORDINAL,
        0,
        0.0f,
        0.0f,
        0.0f,
        kCamTargetingLockFlagsUnchanged,
        xOffset,
        yOffset,
        0.0f,
        0.0f,
        0.0f,
        0,
        -1,
        0);
}

bool DigiviewManager::setCameraManualTarget(int camId, float latitude, float longitude, float altitude)
{
    if ((camId < 0) || (camId > std::numeric_limits<uint8_t>::max())
        || !std::isfinite(latitude) || !std::isfinite(longitude) || !std::isfinite(altitude)
        || (latitude < -90.0f) || (latitude > 90.0f)
        || (longitude < -180.0f) || (longitude > 180.0f)) {
        return false;
    }

    return sendCamTargetingParameters(
        _streamName,
        static_cast<uint8_t>(camId),
        View::COORDINAL,
        0,
        0.0f,
        0.0f,
        0.0f,
        kCamTargetingLockFlagsUnchanged,
        0.0f,
        0.0f,
        latitude,
        longitude,
        altitude,
        0,
        -1,
        0);
}

bool DigiviewManager::stopSingleTargetTracking(int camId)
{
    if ((camId < 0) || (camId > std::numeric_limits<uint8_t>::max())) {
        return false;
    }

    return sendSingleTargetTrackingParameters(
        CMD_OFF,
        _streamName,
        static_cast<uint8_t>(camId),
        0.0f,
        0.0f,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        0,
        0.0f,
        0.0f,
        0,
        0,
        0);
}

bool DigiviewManager::lockCurrentTarget(int cameraSlot)
{
    if ((cameraSlot < 0) || (static_cast<size_t>(cameraSlot) >= _activeTargets.size())) {
        return false;
    }

    const ActiveTarget activeTarget = _activeTargets[cameraSlot];
    switch (activeTarget.type) {
    case ActiveTarget::Type::CamTargeting: {
        auto payload = activeTarget.camTargeting;
        payload.lock_target = 1;
        return _sendCamTargetingParameters(payload);
    }
    case ActiveTarget::Type::PendingDetection: {
        auto payload = activeTarget.camTargeting;
        payload.lock_target = 1;
        return _sendCamTargetingParameters(payload);
    }
    case ActiveTarget::Type::SingleTargetTracking: {
        auto payload = activeTarget.singleTargetTracking;
        payload.lock_target = 1;
        return _sendSingleTargetTrackingParameters(payload);
    }
    case ActiveTarget::Type::Detection: {
        auto camTargeting = activeTarget.camTargeting;
        auto singleTargetTracking = activeTarget.singleTargetTracking;
        camTargeting.lock_target = 1;
        singleTargetTracking.lock_target = 1;
        return _sendCamTargetingParameters(camTargeting)
            && _sendSingleTargetTrackingParameters(singleTargetTracking);
    }
    case ActiveTarget::Type::None:
        return false;
    }

    return false;
}

bool DigiviewManager::clearCurrentTarget(int cameraSlot)
{
    if ((cameraSlot < 0) || (static_cast<size_t>(cameraSlot) >= _activeTargets.size())) {
        return false;
    }

    const ActiveTarget activeTarget = _activeTargets[cameraSlot];
    switch (activeTarget.type) {
    case ActiveTarget::Type::SingleTargetTracking:
        return stopSingleTargetTracking(cameraSlot);
    case ActiveTarget::Type::Detection:
        return clearDetectionTracking(cameraSlot);
    case ActiveTarget::Type::PendingDetection:
        return clearDetectionTracking(cameraSlot);
    case ActiveTarget::Type::CamTargeting: {
        if ((activeTarget.type == ActiveTarget::Type::CamTargeting)
            && (activeTarget.camTargeting.targeting_mode == View::DETECTION)) {
            return clearDetectionTracking(cameraSlot);
        }

        auto payload = activeTarget.camTargeting;
        // Canonical targeting mode 0 is directional and clears the active target.
        payload.targeting_mode = View::DIRECTIONAL;
        payload.euler_delta = 1;
        payload.yaw = 0.0f;
        payload.pitch = 0.0f;
        payload.roll = 0.0f;
        payload.lock_flags = kCamTargetingLockFlagsAll;
        payload.x_offset = 0.0f;
        payload.y_offset = 0.0f;
        payload.target_latitude = 0.0f;
        payload.target_longitude = 0.0f;
        payload.target_altitude = 0.0f;
        payload.track_id = 0;
        payload.view_id = -1;
        payload.lock_target = 0;

        if (!_sendCamTargetingParameters(payload)) {
            return false;
        }

        _activeTargets[cameraSlot] = {};
        return true;
    }
    case ActiveTarget::Type::None:
        return false;
    }

    return false;
}

bool DigiviewManager::sendCalibrationParameters(int cameraId, int calibrationCommand)
{
    if ((cameraId < 0) || (cameraId > std::numeric_limits<uint8_t>::max())
        || (calibrationCommand < CALIBRATION_CMD_NONE) || (calibrationCommand >= NUM_CALIBRATION_CMDS)) {
        emit commandRejected(tr("The requested DigiView calibration command is invalid and was not sent."));
        return false;
    }

    mavlink_message_t msg;
    mavlink_calibration_parameters_t payload {};

    payload.cam_id = static_cast<uint8_t>(cameraId);
    payload.calib_command = static_cast<uint8_t>(calibrationCommand);
    payload.calib_status = CALIBRATION_STATUS_NOT_STARTED;

    _encodeMessage(msg, payload, mavlink_msg_calibration_parameters_encode);
    return _sendMessage(msg);
}

bool DigiviewManager::sendNavigationParameters(
    float altitude, float visual_lat, float visual_lon,
    float next_waypoint_target_yaw, float next_waypoint_target_pitch, float next_waypoint_target_roll,
    float visual_vel_x, float visual_vel_y, float visual_vel_z)
{
    Q_UNUSED(altitude);
    Q_UNUSED(visual_lat);
    Q_UNUSED(visual_lon);
    Q_UNUSED(next_waypoint_target_yaw);
    Q_UNUSED(next_waypoint_target_pitch);
    Q_UNUSED(next_waypoint_target_roll);
    Q_UNUSED(visual_vel_x);
    Q_UNUSED(visual_vel_y);
    Q_UNUSED(visual_vel_z);
    return _rejectUnsupportedSet(tr("NAVIGATION"));
}

void DigiviewManager::changeEuler(int camId, float yaw, float pitch)
{
    sendCamTargetingParameters(
        _streamName,
        camId,
        View::DIRECTIONAL,
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
        0
    );
}

void DigiviewManager::startRecording() 
{
    (void) sendCaptureParameters(
        _streamName,
        0,
        1,
        0,
        0
    );
}

void DigiviewManager::stopRecording() 
{
    (void) sendCaptureParameters(
        _streamName,
        0,
        0,
        0,
        0
    );
}

bool DigiviewManager::takePhoto()
{
    emit commandRejected(tr("Still-image capture is not supported by DigiView and was not sent."));
    return false;
}

void DigiviewManager::_handleMessage(const mavlink_message_t& message)
{
    if (!_logicalSessionActive || !_connection->connected()) {
        return;
    }

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
        if ((message.sysid != kDigiviewSystemId) || (message.compid != kDigiviewComponentId)) {
            if (!_unexpectedHeartbeatWarningTimer.isValid()
                || _unexpectedHeartbeatWarningTimer.hasExpired(5000)) {
                _unexpectedHeartbeatWarningTimer.start();
                qCWarning(DigiviewManagerLog) << "Ignoring Digiview HEARTBEAT from unexpected MAVLink identity"
                                              << "senderSystem" << message.sysid
                                              << "senderComponent" << message.compid
                                              << "expectedSystem" << kDigiviewSystemId
                                              << "expectedComponent" << kDigiviewComponentId;
            }
        } else {
            _establishRemoteSession(kDigiviewSystemId, kDigiviewComponentId);
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

        const bool expectedSender = _remoteIdentityValid
            && (message.sysid == _remoteSystemId) && (message.compid == _remoteComponentId);
        const bool expectedTarget = ((ack.target_system == 0U) || (ack.target_system == _senderSystemId))
            && ((ack.target_component == 0U) || (ack.target_component == _senderComponentId));
        if (expectedSender && (ack.command == MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS)
            && expectedTarget && _videoOutputTransaction && (ack.result != MAV_RESULT_IN_PROGRESS)) {
            if (ack.result == MAV_RESULT_ACCEPTED) {
                // COMMAND_ACK has no transaction generation. An old accepted ACK may request a GET, but only
                // matching authoritative state can complete the current transaction.
                auto& transaction = *_videoOutputTransaction;
                transaction.awaitingAuthoritativeState = true;
                if (!transaction.stateGetIssued) {
                    transaction.stateGetIssued = true;
                    (void) _requestParameters(MAVLINK_MSG_ID_VIDEO_OUTPUT_PARAMETERS);
                }
            } else if (ack.result == MAV_RESULT_DENIED) {
                emit commandRejected(
                    tr("DigiView rejected the video-output update because the selected pipeline is locked."));
                _videoOutputTransactionTimer.stop();
                _videoOutputTransaction.reset();
            } else {
                emit commandRejected(tr("DigiView rejected the video-output update: %1.")
                                         .arg(QGCMAVLink::mavResultToString(ack.result)));
                _videoOutputTransactionTimer.stop();
                _videoOutputTransaction.reset();
            }
        }
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
        emit aiParametersReceived(payload.run_ai, stringFromCharBuf(payload.scan_model_name, 16));
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

        const QString streamName = stringFromCharBuf(payload.stream_name, 16);
        if (streamName != _streamName) {
            qCDebug(DigiviewManagerLog) << "Ignoring VIDEO_OUTPUT_PARAMETERS for unexpected stream"
                                        << "stream" << streamName
                                        << "expected" << _streamName;
            break;
        }

        const uint8_t expectedUserViewCount = userViewCountForLayout(payload.layout_mode);
        if ((expectedUserViewCount == 0U) || (payload.num_user_views != expectedUserViewCount)
            || (payload.detection_overlay_mode > Layout::DET_OVERLAY_MAX)) {
            qCWarning(DigiviewManagerLog) << "Ignoring invalid VIDEO_OUTPUT_PARAMETERS"
                                          << "layoutMode" << payload.layout_mode
                                          << "detectionOverlayMode" << payload.detection_overlay_mode
                                          << "numUserViews" << payload.num_user_views;
            break;
        }

        if ((message.sysid != kDigiviewSystemId) || (message.compid != kDigiviewComponentId)) {
            qCWarning(DigiviewManagerLog) << "Ignoring VIDEO_OUTPUT_PARAMETERS from unexpected DigiView identity"
                                          << "senderSystem" << message.sysid
                                          << "senderComponent" << message.compid;
            break;
        }
        if (!_remoteIdentityValid) {
            _establishRemoteSession(kDigiviewSystemId, kDigiviewComponentId);
        } else if ((message.sysid != _remoteSystemId) || (message.compid != _remoteComponentId)) {
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
        const bool completesVideoOutputTransaction = _videoOutputTransaction
            && _videoOutputTransaction->awaitingAuthoritativeState
            && (_videoOutputTransaction->requested
                == VideoOutputLayoutSnapshot {payload.layout_mode, payload.detection_overlay_mode, payload.num_user_views});

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
        _videoOutputParameters = payload;
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
        if (completesVideoOutputTransaction) {
            _videoOutputTransactionTimer.stop();
            _videoOutputTransaction.reset();
        }
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
        if (!_remoteIdentityValid || (message.sysid != _remoteSystemId) || (message.compid != _remoteComponentId)) {
            qCDebug(DigiviewManagerLog) << "Ignoring DETECTION_PARAMETERS from an unexpected MAVLink identity"
                                        << "senderSystem" << message.sysid
                                        << "senderComponent" << message.compid;
            break;
        }

        mavlink_detection_parameters_t payload;
        mavlink_msg_detection_parameters_decode(&message, &payload);

        const bool hasDetectionParametersChangedValue = !_hasDetectionParameters;
        const bool detectionModeChangedValue = _detectionMode != payload.mode;
        const bool detectionSortingModeChangedValue = _detectionSortingMode != payload.sorting_mode;
        const bool detectionTrackConfidenceThresholdChangedValue =
            !qFuzzyCompare(_detectionTrackConfidenceThreshold, payload.track_confidence_threshold);
        const bool detectionScanConfidenceThresholdChangedValue =
            !qFuzzyCompare(_detectionScanConfidenceThreshold, payload.scan_confidence_threshold);
        const bool detectionTrackBoxOverlapChangedValue =
            !qFuzzyCompare(_detectionTrackBoxOverlap, payload.track_box_overlap);
        const bool detectionScanBoxOverlapChangedValue =
            !qFuzzyCompare(_detectionScanBoxOverlap, payload.scan_box_overlap);
        const bool detectionCreationScoreScaleChangedValue = _detectionCreationScoreScale != payload.creation_score_scale;
        const bool detectionBonusDetectionScaleChangedValue = _detectionBonusDetectionScale != payload.bonus_detection_scale;
        const bool detectionBonusRedetectionScaleChangedValue =
            _detectionBonusRedetectionScale != payload.bonus_redetection_scale;
        const bool detectionMissedDetectionPenaltyChangedValue =
            _detectionMissedDetectionPenalty != payload.missed_detection_penalty;
        const bool detectionMissedRedetectionPenaltyChangedValue =
            _detectionMissedRedetectionPenalty != payload.missed_redetection_penalty;

        _hasDetectionParameters = true;
        _detectionMode = payload.mode;
        _detectionSortingMode = payload.sorting_mode;
        _detectionTrackConfidenceThreshold = payload.track_confidence_threshold;
        _detectionScanConfidenceThreshold = payload.scan_confidence_threshold;
        _detectionTrackBoxOverlap = payload.track_box_overlap;
        _detectionScanBoxOverlap = payload.scan_box_overlap;
        _detectionCreationScoreScale = payload.creation_score_scale;
        _detectionBonusDetectionScale = payload.bonus_detection_scale;
        _detectionBonusRedetectionScale = payload.bonus_redetection_scale;
        _detectionMissedDetectionPenalty = payload.missed_detection_penalty;
        _detectionMissedRedetectionPenalty = payload.missed_redetection_penalty;

        if (hasDetectionParametersChangedValue) {
            emit hasDetectionParametersChanged();
        }
        if (detectionModeChangedValue) {
            emit detectionModeChanged();
        }
        if (detectionSortingModeChangedValue) {
            emit detectionSortingModeChanged();
        }
        if (detectionTrackConfidenceThresholdChangedValue) {
            emit detectionTrackConfidenceThresholdChanged();
        }
        if (detectionScanConfidenceThresholdChangedValue) {
            emit detectionScanConfidenceThresholdChanged();
        }
        if (detectionTrackBoxOverlapChangedValue) {
            emit detectionTrackBoxOverlapChanged();
        }
        if (detectionScanBoxOverlapChangedValue) {
            emit detectionScanBoxOverlapChanged();
        }
        if (detectionCreationScoreScaleChangedValue) {
            emit detectionCreationScoreScaleChanged();
        }
        if (detectionBonusDetectionScaleChangedValue) {
            emit detectionBonusDetectionScaleChanged();
        }
        if (detectionBonusRedetectionScaleChangedValue) {
            emit detectionBonusRedetectionScaleChanged();
        }
        if (detectionMissedDetectionPenaltyChangedValue) {
            emit detectionMissedDetectionPenaltyChanged();
        }
        if (detectionMissedRedetectionPenaltyChangedValue) {
            emit detectionMissedRedetectionPenaltyChanged();
        }
        if (hasDetectionParametersChangedValue || detectionModeChangedValue || detectionSortingModeChangedValue
            || detectionTrackConfidenceThresholdChangedValue || detectionScanConfidenceThresholdChangedValue
            || detectionTrackBoxOverlapChangedValue || detectionScanBoxOverlapChangedValue
            || detectionCreationScoreScaleChangedValue || detectionBonusDetectionScaleChangedValue
            || detectionBonusRedetectionScaleChangedValue || detectionMissedDetectionPenaltyChangedValue
            || detectionMissedRedetectionPenaltyChangedValue) {
            emit detectionParametersChanged();
        }

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
        if (!_remoteIdentityValid) {
            qCDebug(DigiviewManagerLog) << "Ignoring CAM_TARGETING_PARAMETERS before target HEARTBEAT"
                                        << "senderSystem" << message.sysid
                                        << "senderComponent" << message.compid;
            break;
        }

        if ((message.sysid != _remoteSystemId) || (message.compid != _remoteComponentId)) {
            qCWarning(DigiviewManagerLog) << "Ignoring CAM_TARGETING_PARAMETERS from different MAVLink identity"
                                          << "senderSystem" << message.sysid
                                          << "senderComponent" << message.compid;
            break;
        }

        mavlink_cam_targeting_parameters_t payload;
        mavlink_msg_cam_targeting_parameters_decode(&message, &payload);
        const QString streamName = stringFromCharBuf(payload.stream_name, 16);

        if (streamName != _streamName) {
            qCDebug(DigiviewManagerLog) << "Ignoring CAM_TARGETING_PARAMETERS for unexpected stream"
                                        << "stream" << streamName
                                        << "expected" << _streamName;
            break;
        }

        emit camTargetingParametersReceived(
            streamName,
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

        _rememberInboundCamTargeting(payload);

        if (payload.cam_id < kMaxCameras) {
            auto& state = _cameraStates[payload.cam_id];
            state.targetingMode = payload.targeting_mode;
            state.trackId = payload.track_id;
            state.viewId = payload.view_id;
            state.hasActiveTarget = _activeTargets[payload.cam_id].type != ActiveTarget::Type::None;
            state.hasTargetState = true;

            emit cameraStatesChanged();
        }
        break;
    }
    case MAVLINK_MSG_ID_CAM_OPTICS_AND_CONTROL_PARAMETERS: {
        mavlink_cam_optics_and_control_parameters_t payload;
        mavlink_msg_cam_optics_and_control_parameters_decode(&message, &payload);
        emit camOpticsAndControlParametersReceived(
            stringFromCharBuf(payload.stream_name, 16),
            payload.cam_id,
            payload.zoom,
            payload.fov);
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
        if (!_remoteIdentityValid || (message.sysid != _remoteSystemId) || (message.compid != _remoteComponentId)) {
            qCDebug(DigiviewManagerLog) << "Ignoring SENSOR_PARAMETERS from an unexpected MAVLink identity"
                                        << "senderSystem" << message.sysid
                                        << "senderComponent" << message.compid;
            break;
        }

        mavlink_sensor_parameters_t payload;
        mavlink_msg_sensor_parameters_decode(&message, &payload);

        const bool hasSensorParametersChangedValue = !_hasSensorParameters;
        const bool sensorMinExposureChangedValue = _sensorMinExposure != payload.min_exposure;
        const bool sensorMaxExposureChangedValue = _sensorMaxExposure != payload.max_exposure;
        const bool sensorMinGainChangedValue = _sensorMinGain != payload.min_gain;
        const bool sensorMaxGainChangedValue = _sensorMaxGain != payload.max_gain;
        const bool sensorTargetBrightnessChangedValue =
            !qFuzzyCompare(_sensorTargetBrightness, payload.target_brightness);

        _hasSensorParameters = true;
        _sensorMinExposure = payload.min_exposure;
        _sensorMaxExposure = payload.max_exposure;
        _sensorMinGain = payload.min_gain;
        _sensorMaxGain = payload.max_gain;
        _sensorTargetBrightness = payload.target_brightness;

        if (hasSensorParametersChangedValue) {
            emit hasSensorParametersChanged();
        }
        if (sensorMinExposureChangedValue) {
            emit sensorMinExposureChanged();
        }
        if (sensorMaxExposureChangedValue) {
            emit sensorMaxExposureChanged();
        }
        if (sensorMinGainChangedValue) {
            emit sensorMinGainChanged();
        }
        if (sensorMaxGainChangedValue) {
            emit sensorMaxGainChanged();
        }
        if (sensorTargetBrightnessChangedValue) {
            emit sensorTargetBrightnessChanged();
        }
        if (hasSensorParametersChangedValue || sensorMinExposureChangedValue || sensorMaxExposureChangedValue
            || sensorMinGainChangedValue || sensorMaxGainChangedValue || sensorTargetBrightnessChangedValue) {
            emit sensorParametersChanged();
        }

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
        if (!_remoteIdentityValid) {
            qCDebug(DigiviewManagerLog) << "Ignoring SINGLE_TARGET_TRACKING_PARAMETERS before target HEARTBEAT"
                                        << "senderSystem" << message.sysid
                                        << "senderComponent" << message.compid;
            break;
        }

        if ((message.sysid != _remoteSystemId) || (message.compid != _remoteComponentId)) {
            qCWarning(DigiviewManagerLog) << "Ignoring SINGLE_TARGET_TRACKING_PARAMETERS from different MAVLink identity"
                                          << "senderSystem" << message.sysid
                                          << "senderComponent" << message.compid;
            break;
        }

        mavlink_single_target_tracking_parameters_t payload;
        mavlink_msg_single_target_tracking_parameters_decode(&message, &payload);
        const QString streamName = stringFromCharBuf(payload.stream_name, 16);
        const bool globalStatusResponse = streamName.isEmpty();

        if (globalStatusResponse) {
            bool validGlobalStatusCommand = false;
            switch (static_cast<single_target_tracker_command>(payload.command)) {
            case CMD_OFF:
            case CMD_SET_TARGET_VECTOR:
            case CMD_NONE:
                validGlobalStatusCommand = true;
                break;
            default:
                break;
            }

            if (!validGlobalStatusCommand) {
                qCWarning(DigiviewManagerLog)
                    << "Ignoring empty-stream SINGLE_TARGET_TRACKING_PARAMETERS with invalid command"
                    << "command" << payload.command;
                break;
            }
        } else if (streamName != _streamName) {
            qCDebug(DigiviewManagerLog) << "Ignoring SINGLE_TARGET_TRACKING_PARAMETERS for unexpected stream"
                                         << "stream" << streamName
                                         << "expected" << _streamName;
            break;
        }

        qCDebug(DigiviewManagerLog)
            << "command =" << payload.command
            << "stream =" << streamName
            << "cam =" << payload.cam_id
            << "x =" << payload.x_offset
            << "y =" << payload.y_offset
            << "det =" << payload.detection_id
            << "zoom =" << payload.zoom_level
            << "conf =" << payload.confidence
            << "yaw =" << payload.yaw_global
            << "pitch =" << payload.pitch_global
            << "status =" << payload.status
            << "lock =" << payload.lock_target;

        emit singleTargetTrackingParametersReceived(
            payload.command,
            streamName,
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
            payload.publish_timestamp_us,
            payload.status,
            payload.lock_target);

        if (!globalStatusResponse) {
            _rememberInboundSingleTargetTracking(payload);

            if (payload.cam_id < kMaxCameras) {
                auto& state = _cameraStates[payload.cam_id];
                state.sttStatus = payload.status;
                state.confidence = payload.confidence;
                state.lockTarget = (payload.lock_target != 0);
                state.hasActiveTarget = _activeTargets[payload.cam_id].type != ActiveTarget::Type::None;
                state.hasTargetState = true;

                emit cameraStatesChanged();
            }
        }

        if (!_hasSttParameters) {
            _hasSttParameters = true;
            emit hasSttParametersChanged();
        }

        if (_sttStatus != payload.status) {
            _sttStatus = payload.status;
            emit sttStatusChanged();
        }

        if (!globalStatusResponse) {
            if (_sttCamId != payload.cam_id) {
                _sttCamId = payload.cam_id;
                emit sttCamIdChanged();
            }

            if (!qFuzzyCompare(_sttConfidence, payload.confidence)) {
                _sttConfidence = payload.confidence;
                emit sttConfidenceChanged();
            }

            if (_sttLockTarget != payload.lock_target) {
                _sttLockTarget = payload.lock_target;
                emit sttLockTargetChanged();
            }
        }
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

bool DigiviewManager::_rejectUnsupportedSet(const QString& parameterName)
{
    emit commandRejected(tr("DigiView does not support setting %1 parameters; nothing was sent.").arg(parameterName));
    return false;
}

void DigiviewManager::_videoOutputTransactionTimedOut()
{
    if (!_videoOutputTransaction
        || (_videoOutputTransaction->generation != _videoOutputTransactionTimerGeneration)) {
        return;
    }

    if (!_videoOutputTransaction->deadline.hasExpired()) {
        _videoOutputTransactionTimer.start(static_cast<int>(_videoOutputTransaction->deadline.remainingTime()));
        return;
    }

    qCWarning(DigiviewManagerLog) << "Timed out waiting for authoritative VIDEO_OUTPUT_PARAMETERS state"
                                  << "generation" << _videoOutputTransaction->generation;
    _videoOutputTransaction.reset();
}

void DigiviewManager::_establishRemoteSession(uint8_t systemId, uint8_t componentId)
{
    if (!_logicalSessionActive || !_connection->connected()) {
        return;
    }
    if ((systemId != kDigiviewSystemId) || (componentId != kDigiviewComponentId)) {
        return;
    }

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
    _sendMessage(msg);

    command.param1 = static_cast<float>(MAVLINK_MSG_ID_CAM_TARGETING_PARAMETERS);

    _encodeMessage(msg, command, mavlink_msg_command_long_encode);
    qCDebug(DigiviewManagerLog) << "Subscribing to CAM_TARGETING_PARAMETERS at" << command.param2 << "us";
    _sendMessage(msg);

    command.param1 = static_cast<float>(MAVLINK_MSG_ID_SINGLE_TARGET_TRACKING_PARAMETERS);
    command.param2 = kVideoOutputParametersSubscriptionIntervalUs;

    _encodeMessage(msg, command, mavlink_msg_command_long_encode);

    qCDebug(DigiviewManagerLog)
        << "Subscribing to SINGLE_TARGET_TRACKING_PARAMETERS at"
        << command.param2 << "us";

    _sendMessage(msg);

    command.param1 = static_cast<float>(MAVLINK_MSG_ID_SENSOR_PARAMETERS);
    _encodeMessage(msg, command, mavlink_msg_command_long_encode);
    qCDebug(DigiviewManagerLog) << "Subscribing to SENSOR_PARAMETERS at" << command.param2 << "us";
    _sendMessage(msg);

    command.param1 = static_cast<float>(MAVLINK_MSG_ID_DETECTION_PARAMETERS);
    _encodeMessage(msg, command, mavlink_msg_command_long_encode);
    qCDebug(DigiviewManagerLog) << "Subscribing to DETECTION_PARAMETERS at" << command.param2 << "us";
    _sendMessage(msg);

    if (_pendingVideoOutputParametersRequest) {
        requestVideoOutputParameters();
    }
    if (_pendingSensorParametersRequest) {
        requestSensorParameters();
    }
    if (_pendingDetectionParametersRequest) {
        requestDetectionParameters();
    }
    if (_pendingSingleTargetTrackingParametersRequest) {
        requestSingleTargetTrackingParameters();
    }
}

void DigiviewManager::_resetRemoteSession()
{
    _remoteSystemId = 0;
    _remoteComponentId = 0;
    _remoteIdentityValid = false;
    _pendingVideoOutputParametersRequest = false;
    _pendingSensorParametersRequest = true;
    _pendingDetectionParametersRequest = true;
    _pendingSingleTargetTrackingParametersRequest = true;
    _videoOutputTransactionTimer.stop();
    _videoOutputTransaction.reset();

    _cameraStates.fill(CameraTrackingState{});
    _activeTargets.fill(ActiveTarget{});
    _hasSttParameters = false;
    _sttStatus = static_cast<uint8_t>(single_target_tracking_status::OFF);
    _sttCamId = 0;
    _sttConfidence = 0.0f;
    _sttLockTarget = 0;

    emit hasSttParametersChanged();
    emit sttStatusChanged();
    emit sttCamIdChanged();
    emit sttConfidenceChanged();
    emit sttLockTargetChanged();
    emit cameraStatesChanged();

    const bool hasVideoOutputParametersChangedValue = _hasVideoOutputParameters;
    const bool videoOutputStreamNameChangedValue = !_videoOutputStreamName.isEmpty();
    const bool videoOutputWidthChangedValue = _videoOutputWidth != 0;
    const bool videoOutputHeightChangedValue = _videoOutputHeight != 0;
    const bool videoOutputFpsChangedValue = _videoOutputFps != 0;
    const bool videoOutputLayoutModeChangedValue = _videoOutputLayoutMode != Layout::LAYOUT_1;
    const bool videoOutputDetectionOverlayModeChangedValue =
        _videoOutputDetectionOverlayMode != Layout::DET_OVERLAY_NONE;
    const bool videoOutputNumUserViewsChangedValue = _videoOutputNumUserViews != 0;
    const bool videoOutputViewsChangedValue = !_videoOutputViews.isEmpty();
    const bool videoOutputDetectionOverlayRectChangedValue = !_videoOutputDetectionOverlayRect.isEmpty();
    const bool videoOutputSingleDetectionSizeChangedValue = _videoOutputSingleDetectionSize != 0;

    _hasVideoOutputParameters = false;
    _videoOutputStreamName.clear();
    _videoOutputWidth = 0;
    _videoOutputHeight = 0;
    _videoOutputFps = 0;
    _videoOutputLayoutMode = Layout::LAYOUT_1;
    _videoOutputDetectionOverlayMode = Layout::DET_OVERLAY_NONE;
    _videoOutputNumUserViews = 0;
    _videoOutputViews.clear();
    _videoOutputDetectionOverlayRect.clear();
    _videoOutputSingleDetectionSize = 0;
    _videoOutputParameters = {};

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

    const bool hasSensorParametersChangedValue = _hasSensorParameters;
    const bool sensorMinExposureChangedValue = _sensorMinExposure != 0;
    const bool sensorMaxExposureChangedValue = _sensorMaxExposure != 0;
    const bool sensorMinGainChangedValue = _sensorMinGain != 0;
    const bool sensorMaxGainChangedValue = _sensorMaxGain != 0;
    const bool sensorTargetBrightnessChangedValue = !qFuzzyIsNull(_sensorTargetBrightness);

    _hasSensorParameters = false;
    _sensorMinExposure = 0;
    _sensorMaxExposure = 0;
    _sensorMinGain = 0;
    _sensorMaxGain = 0;
    _sensorTargetBrightness = 0.0f;

    if (hasSensorParametersChangedValue) {
        emit hasSensorParametersChanged();
    }
    if (sensorMinExposureChangedValue) {
        emit sensorMinExposureChanged();
    }
    if (sensorMaxExposureChangedValue) {
        emit sensorMaxExposureChanged();
    }
    if (sensorMinGainChangedValue) {
        emit sensorMinGainChanged();
    }
    if (sensorMaxGainChangedValue) {
        emit sensorMaxGainChanged();
    }
    if (sensorTargetBrightnessChangedValue) {
        emit sensorTargetBrightnessChanged();
    }
    if (hasSensorParametersChangedValue || sensorMinExposureChangedValue || sensorMaxExposureChangedValue
        || sensorMinGainChangedValue || sensorMaxGainChangedValue || sensorTargetBrightnessChangedValue) {
        emit sensorParametersChanged();
    }

    const bool hasDetectionParametersChangedValue = _hasDetectionParameters;
    const bool detectionModeChangedValue = _detectionMode != 0;
    const bool detectionSortingModeChangedValue = _detectionSortingMode != 0;
    const bool detectionTrackConfidenceThresholdChangedValue = !qFuzzyIsNull(_detectionTrackConfidenceThreshold);
    const bool detectionScanConfidenceThresholdChangedValue = !qFuzzyIsNull(_detectionScanConfidenceThreshold);
    const bool detectionTrackBoxOverlapChangedValue = !qFuzzyIsNull(_detectionTrackBoxOverlap);
    const bool detectionScanBoxOverlapChangedValue = !qFuzzyIsNull(_detectionScanBoxOverlap);
    const bool detectionCreationScoreScaleChangedValue = _detectionCreationScoreScale != 0;
    const bool detectionBonusDetectionScaleChangedValue = _detectionBonusDetectionScale != 0;
    const bool detectionBonusRedetectionScaleChangedValue = _detectionBonusRedetectionScale != 0;
    const bool detectionMissedDetectionPenaltyChangedValue = _detectionMissedDetectionPenalty != 0;
    const bool detectionMissedRedetectionPenaltyChangedValue = _detectionMissedRedetectionPenalty != 0;

    _hasDetectionParameters = false;
    _detectionMode = 0;
    _detectionSortingMode = 0;
    _detectionTrackConfidenceThreshold = 0.0f;
    _detectionScanConfidenceThreshold = 0.0f;
    _detectionTrackBoxOverlap = 0.0f;
    _detectionScanBoxOverlap = 0.0f;
    _detectionCreationScoreScale = 0;
    _detectionBonusDetectionScale = 0;
    _detectionBonusRedetectionScale = 0;
    _detectionMissedDetectionPenalty = 0;
    _detectionMissedRedetectionPenalty = 0;

    if (hasDetectionParametersChangedValue) {
        emit hasDetectionParametersChanged();
    }
    if (detectionModeChangedValue) {
        emit detectionModeChanged();
    }
    if (detectionSortingModeChangedValue) {
        emit detectionSortingModeChanged();
    }
    if (detectionTrackConfidenceThresholdChangedValue) {
        emit detectionTrackConfidenceThresholdChanged();
    }
    if (detectionScanConfidenceThresholdChangedValue) {
        emit detectionScanConfidenceThresholdChanged();
    }
    if (detectionTrackBoxOverlapChangedValue) {
        emit detectionTrackBoxOverlapChanged();
    }
    if (detectionScanBoxOverlapChangedValue) {
        emit detectionScanBoxOverlapChanged();
    }
    if (detectionCreationScoreScaleChangedValue) {
        emit detectionCreationScoreScaleChanged();
    }
    if (detectionBonusDetectionScaleChangedValue) {
        emit detectionBonusDetectionScaleChanged();
    }
    if (detectionBonusRedetectionScaleChangedValue) {
        emit detectionBonusRedetectionScaleChanged();
    }
    if (detectionMissedDetectionPenaltyChangedValue) {
        emit detectionMissedDetectionPenaltyChanged();
    }
    if (detectionMissedRedetectionPenaltyChangedValue) {
        emit detectionMissedRedetectionPenaltyChanged();
    }
    if (hasDetectionParametersChangedValue || detectionModeChangedValue || detectionSortingModeChangedValue
        || detectionTrackConfidenceThresholdChangedValue || detectionScanConfidenceThresholdChangedValue
        || detectionTrackBoxOverlapChangedValue || detectionScanBoxOverlapChangedValue
        || detectionCreationScoreScaleChangedValue || detectionBonusDetectionScaleChangedValue
        || detectionBonusRedetectionScaleChangedValue || detectionMissedDetectionPenaltyChangedValue
        || detectionMissedRedetectionPenaltyChangedValue) {
        emit detectionParametersChanged();
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

QVariantList DigiviewManager::cameraStates() const
{
    QVariantList list;
    list.reserve(static_cast<qsizetype>(_cameraStates.size()));

    for (const auto& cam : _cameraStates) {
        QVariantMap map;
        map.insert(QStringLiteral("sttStatus"), cam.sttStatus);
        map.insert(QStringLiteral("confidence"), cam.confidence);
        map.insert(QStringLiteral("trackId"), cam.trackId);
        map.insert(QStringLiteral("viewId"), cam.viewId);
        map.insert(QStringLiteral("lockTarget"), cam.lockTarget);
        map.insert(QStringLiteral("targetingMode"), cam.targetingMode);
        map.insert(QStringLiteral("hasActiveTarget"), cam.hasActiveTarget);
        map.insert(QStringLiteral("hasTargetState"), cam.hasTargetState);
        list.append(map);
    }
    return list;
}
