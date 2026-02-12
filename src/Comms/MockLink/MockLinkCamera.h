#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(MockLinkCameraLog)

class MockLink;

/// Simulates MAVLink Camera Protocol v2 components for MockLink.
///
/// Two cameras are provided:
///   Camera 1 (MAV_COMP_ID_CAMERA)  – full-featured: video capture, photo capture,
///       mode switching, basic zoom, basic focus, video streaming
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
    };

    explicit MockLinkCamera(MockLink *mockLink,
                            bool captureVideo = true,
                            bool captureImage = true,
                            bool hasModes = true,
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

    /// Handle a COMMAND_LONG that targets a camera component.
    /// @return true if the command was handled (ack already sent)
    bool handleCameraCommand(const mavlink_command_long_t &request, uint8_t targetCompId);

    /// Handle a MAV_CMD_REQUEST_MESSAGE for camera-related message IDs.
    /// @return true if the message ID was handled
    bool handleRequestMessage(const mavlink_command_long_t &request, uint8_t targetCompId);

private:
    void _sendCameraInformation(uint8_t compId);
    void _sendCameraSettings(uint8_t compId);
    void _sendStorageInformation(uint8_t compId);
    void _sendCameraCaptureStatus(uint8_t compId);
    void _sendCameraImageCaptured(uint8_t compId);
    void _sendVideoStreamInformation(uint8_t compId, uint8_t streamId);
    void _sendVideoStreamStatus(uint8_t compId, uint8_t streamId);
    void _sendCommandAck(uint8_t compId, uint16_t command, uint8_t result, int requestedMsgId = -1);

    CameraState *_findCamera(uint8_t compId);
    static const char *_imageCaptureStatusToString(uint8_t status);

    static constexpr uint8_t  kNumCameras       = 2;
    static constexpr uint8_t  kNumStreams        = 2;    ///< Streams per camera
    static constexpr uint32_t kStorageTotalMiB   = 16384; ///< 16 GiB simulated SD card
    static constexpr uint32_t kStorageFreeMiB    = 8192;  ///< 8 GiB free

    MockLink   *_mockLink = nullptr;
    CameraState _cameras[kNumCameras];           ///< Simulated cameras
};
