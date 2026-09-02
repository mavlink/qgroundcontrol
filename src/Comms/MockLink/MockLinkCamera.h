#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QMutex>
#include <QtCore/QVariant>
#include <QtCore/QVector>

class MockLink;

/// \brief Simulates MAVLink Camera Protocol v2 components for MockLink.
///
/// Two cameras are provided:
///   Camera 1 (MAV_COMP_ID_CAMERA)  – full-featured: video capture, photo capture,
///       mode switching, basic zoom, video streaming
///       (udp://127.0.0.1:5600, H.264 RTP, 1920×1080 @ 30 fps),
///       and image capture while in video mode.
///   Camera 2 (MAV_COMP_ID_CAMERA2) – photo-only: image capture only; video,
///       mode switching, zoom, focus, and streaming commands are denied.
///
/// Supported MAVLink commands:
///   MAV_CMD_REQUEST_MESSAGE (CAMERA_INFORMATION, CAMERA_SETTINGS,
///       STORAGE_INFORMATION, CAMERA_CAPTURE_STATUS,
///       VIDEO_STREAM_INFORMATION, VIDEO_STREAM_STATUS)
///   MAV_CMD_REQUEST_CAMERA_INFORMATION / SETTINGS / STORAGE / CAPTURE_STATUS
///   MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION / STATUS
///   MAV_CMD_SET_CAMERA_MODE
///   MAV_CMD_IMAGE_START_CAPTURE / IMAGE_STOP_CAPTURE
///   MAV_CMD_VIDEO_START_CAPTURE / VIDEO_STOP_CAPTURE
///   MAV_CMD_STORAGE_FORMAT
///   MAV_CMD_SET_CAMERA_ZOOM / SET_CAMERA_FOCUS
///   MAV_CMD_RESET_CAMERA_SETTINGS
///   MAV_CMD_CAMERA_TRACK_POINT / TRACK_RECTANGLE / STOP_TRACKING
///
/// Simulated storage: 16 GiB total, 8 GiB free, SD card.
///
/// Camera 1 additionally serves a small set of extended parameters (PARAM_EXT_REQUEST_LIST /
/// PARAM_EXT_REQUEST_READ / PARAM_EXT_SET). Camera 2 serves none, so tests can check that
/// components which don't implement the protocol are simply left out.
///
class MockLinkCamera
{
public:
    /// Image capture status values
    enum ImageCaptureStatus {
        ImageCaptureIdle            = 0,   ///< No capture in progress
        ImageCaptureInProgress      = 1,   ///< Single image capture in progress
        ImageCaptureInterval        = 2,   ///< Interval capture enabled
        ImageCaptureIntervalCapture = 3,   ///< Interval capture with capture in progress
    };

    /// Per-camera simulated state
    struct CameraState {
        uint8_t  compId              = MAV_COMP_ID_CAMERA;
        uint32_t capFlags            = 0;                    ///< CAMERA_CAP_FLAGS
        uint8_t  cameraMode          = CAMERA_MODE_IMAGE;    ///< CAMERA_MODE enum
        bool     recording           = false;
        int      imagesCaptured      = 0;
        float    zoomLevel           = 1.0f;
        float    focusLevel          = 0.0f;
        uint8_t  image_status        = ImageCaptureIdle;     ///< ImageCaptureStatus enum
        float    image_interval      = 0.0f;                 ///< Interval between image captures (seconds)
        qint64   singleShotStartMs   = 0;                    ///< Timestamp when single-shot capture started (0 = not active)

        // Tracking state
        uint8_t  trackingMode        = CAMERA_TRACKING_MODE_NONE; ///< CAMERA_TRACKING_MODE enum
        float    trackPointX         = 0.0f;
        float    trackPointY         = 0.0f;
        float    trackRadius         = 0.0f;
        float    trackRecTopX        = 0.0f;
        float    trackRecTopY        = 0.0f;
        float    trackRecBottomX     = 0.0f;
        float    trackRecBottomY     = 0.0f;
        qint64   trackingStatusIntervalUs = -1;               ///< Interval for CAMERA_TRACKING_IMAGE_STATUS (-1 = disabled)
        qint64   trackingStatusLastSentMs = 0;                ///< Timestamp of last tracking status message
        qint64   trackingStartMs     = 0;                     ///< Timestamp when tracking was started (for drift animation)
        float    trackAnchorX        = 0.0f;                  ///< Original center X of tracked target
        float    trackAnchorY        = 0.0f;                  ///< Original center Y of tracked target
    };

    explicit MockLinkCamera(MockLink *mockLink,
                            bool captureVideo = true,
                            bool captureImage = true,
                            bool hasModes = true,
                            bool hasVideoStream = true,
                            bool canCaptureImageInVideoMode = true,
                            bool canCaptureVideoInImageMode = false,
                            bool hasBasicZoom = true,
                            bool hasTrackingPoint = false,
                            bool hasTrackingRectangle = false);
    ~MockLinkCamera() = default;

    /// Send heartbeats for all simulated camera components (call from 1Hz tasks)
    void sendCameraHeartbeats();

    /// Update camera states (call from 10Hz tasks)
    void run10HzTasks();

    /// Handle all incoming MAVLink messages for camera.
    /// @return true if the message was handled by the camera
    bool handleMavlinkMessage(const mavlink_message_t &msg);

    /// Extended parameter served by camera 1
    struct ExtParam {
        QString  name;
        uint8_t  type;      ///< MAV_PARAM_EXT_TYPE
        QVariant value;
    };

    /// @return The extended parameters served by camera 1
    static QVector<ExtParam> defaultExtParams();

    enum ExtParamSetFailureMode_t {
        FailExtParamSetNone,        ///< Normal behavior
        FailExtParamSetNoAck,       ///< Do not send PARAM_EXT_ACK
        FailExtParamSetRejected,    ///< Reject with PARAM_ACK_VALUE_UNSUPPORTED, keep the stored value
        FailExtParamSetInProgress,  ///< Answer PARAM_ACK_IN_PROGRESS once, then accept on the next attempt
    };

    /// Sets a PARAM_EXT_SET failure mode for unit testing
    void setExtParamSetFailureMode(ExtParamSetFailureMode_t mode) {
        _extParamSetFailureMode = mode;
        _extParamSetInProgressPending = (mode == FailExtParamSetInProgress);
    }

    /// Test API: drops this index from the PARAM_EXT_REQUEST_LIST stream so the indexed
    /// re-request path can be exercised. Negative disables dropping. Only affects the
    /// list stream - an explicit PARAM_EXT_REQUEST_READ for the index is always answered.
    void setExtParamListDropIndex(int index) { _extParamListDropIndex = index; }

    /// @return Current value of an ext parameter, an invalid QVariant if unknown
    QVariant extParamValue(const QString &name) const;

private:
    bool _handleParamExtRequestList(const mavlink_message_t &msg);
    bool _handleParamExtRequestRead(const mavlink_message_t &msg);
    bool _handleParamExtSet(const mavlink_message_t &msg);
    void _sendParamExtValue(int index);

    /// Handle a COMMAND_LONG that targets a camera component.
    /// @return true if the command was handled (ack already sent)
    bool _handleCameraCommand(const mavlink_command_long_t &request, uint8_t targetCompId);

    /// Handle a MAV_CMD_REQUEST_MESSAGE for camera-related message IDs.
    /// @return true if the message ID was handled
    bool _handleRequestMessage(const mavlink_command_long_t &request, uint8_t targetCompId);

    void _sendCameraInformation(uint8_t compId);
    void _sendCameraSettings(uint8_t compId);
    void _sendStorageInformation(uint8_t compId);
    void _sendCameraCaptureStatus(uint8_t compId);
    void _sendCameraImageCaptured(uint8_t compId);
    void _sendVideoStreamInformation(uint8_t compId, uint8_t streamId);
    void _sendVideoStreamStatus(uint8_t compId, uint8_t streamId);
    void _sendCameraTrackingImageStatus(uint8_t compId);
    void _sendCommandAck(uint8_t compId, uint16_t command, uint8_t result, int requestedMsgId = -1);

    CameraState *_findCamera(uint8_t compId);
    static const char *_imageCaptureStatusToString(uint8_t status);

    static constexpr uint8_t  kNumCameras       = 2;
    static constexpr uint8_t  kNumStreams        = 2;    ///< Streams per camera
    static constexpr uint32_t kStorageTotalMiB   = 16384; ///< 16 GiB simulated SD card
    static constexpr uint32_t kStorageFreeMiB    = 8192;  ///< 8 GiB free

    static constexpr uint8_t kExtParamCompId = MAV_COMP_ID_CAMERA; ///< Only camera 1 serves ext params

    MockLink   *_mockLink = nullptr;
    QVector<ExtParam> _extParams;                ///< Ext parameters served by kExtParamCompId
    ExtParamSetFailureMode_t _extParamSetFailureMode = FailExtParamSetNone;
    bool _extParamSetInProgressPending = false;
    int _extParamListDropIndex = -1;
    CameraState _cameras[kNumCameras];           ///< Simulated cameras
    /// Protects _cameras array from race conditions between:
    ///   - Main thread: _handleCameraCommand() modifying camera state on MAVLink commands
    ///   - Worker thread: run10HzTasks() reading capture status and updating capture counts
    ///   Race example: Main sets singleShotStartMs while Worker checks it for completion
    QMutex      _camerasMutex;
};
