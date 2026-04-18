#pragma once

#include <QtCore/QPermission>
#include <QtMultimedia/QCameraDevice>
#include <chrono>

#include "VideoReceiver.h"

class QCamera;
class QImageCapture;
class QMediaCaptureSession;
class QMediaDevices;
class QVideoSink;

/// VideoReceiver for local USB cameras via Qt's QMediaCaptureSession.
///
/// Manages camera lifecycle (open/close), delivers frames through the
/// standard VideoFrameDelivery::deliverFrame, and records via BridgeRecorder.
class UVCReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit UVCReceiver(QObject* parent = nullptr);
    ~UVCReceiver() override;

    [[nodiscard]] Capabilities capabilities() const override
    {
        Capabilities caps = CapLocalCamera | CapRecording;
        return caps;
    }

    [[nodiscard]] BackendKind kind() const override { return BackendKind::UVC; }

    [[nodiscard]] bool isStreaming() const override { return _streamingActive; }

    [[nodiscard]] bool isDecoding() const override { return _decoderActive; }

    /// QMediaCaptureSession delivers frames via videoFrameChanged — no clock/PTS latency path.
    [[nodiscard]] bool latencySupported() const override { return false; }

    static QCameraDevice findCameraDevice(const QString& cameraId);
    void checkPermission();
    static QString getSourceId();
    static bool deviceExists(const QString& device);
    static QStringList getDeviceNameList();

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding() override;
    void stopDecoding() override;

protected:
    void onSinkAboutToChange() override;
    void onSinkChanged(QVideoSink* newSink) override;

private:
    void _openCamera();
    void _closeCamera();
    void _rewireInternalSink();

    void _setStreamingActive(bool active);
    void _setDecoderActive(bool active);

    bool _streamingActive = false;
    bool _decoderActive = false;

    QCamera* _camera = nullptr;
    QImageCapture* _imageCapture = nullptr;
    QMediaCaptureSession* _captureSession = nullptr;
    QMediaDevices* _mediaDevices = nullptr;

    /// Internal sink wired to the capture session. Its videoFrameChanged is
    /// forwarded through VideoFrameDelivery::deliverFrame so frame stats and
    /// latency measurement work the same as for GStreamer/QtMultimedia.
    QVideoSink* _internalSink = nullptr;
    QMetaObject::Connection _internalFrameConn;

    /// Per-frame watchdog interval, stashed in start() and applied to the
    /// delivery watchdog in startDecoding(). The delivery endpoint owns the timer.
    std::chrono::milliseconds _watchdogInterval{0};

    // Cached permission status — updated by checkPermission(), queried in start().
    Qt::PermissionStatus _permissionStatus = Qt::PermissionStatus::Undetermined;
};
