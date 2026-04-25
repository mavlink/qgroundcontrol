#pragma once

#include <QtCore/QPermission>
#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QMediaPlayer>
#include <chrono>
#include <memory>

#include "VideoReceiver.h"

class QCamera;
class QImageCapture;
class QMediaCaptureSession;
class QMediaDevices;
class QtVideoSinkRouter;

class QtMultimediaReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit QtMultimediaReceiver(QObject* parent = nullptr);
    virtual ~QtMultimediaReceiver();

    [[nodiscard]] Capabilities capabilities() const override { return CapStreaming | CapRecording | CapLocalCamera; }

    [[nodiscard]] bool isStreaming() const override { return _streamingActive; }

    [[nodiscard]] bool isDecoding() const override { return _decoderActive; }

    /// Qt Multimedia (FFmpeg backend) provides no per-frame PTS-vs-clock measurement.
    [[nodiscard]] bool latencySupported() const override { return false; }

    void configureSource(const VideoSourceResolver::VideoSource& source) override;

    static QCameraDevice findLocalCameraDevice(const QString& cameraId);
    static QString defaultLocalCameraId();
    static bool localCameraAvailable();
    static bool localCameraDeviceExists(const QString& device);
    static QStringList localCameraDeviceNameList();

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding() override;
    void stopDecoding() override;
    void pause() override;
    void resume() override;

protected:
    void onSinkAboutToChange() override;
    [[nodiscard]] SinkChangeAction onSinkChanged(QVideoSink* newSink) override;

    /// Read the current codec/decoder info from the media player's metadata
    /// and update the base-class decoder fields. Called from hasVideoChanged
    /// and metaDataChanged to expose the decoder-name readout from Qt's
    /// decoder-name readout (HW decode status is not exposed by Qt's FFmpeg
    /// backend, so hwDecoding stays false).
    void _pullDecoderInfoFromMetadata();

    enum class SourceMode : quint8
    {
        Playback,
        LocalCamera,
    };

    QMediaPlayer* _mediaPlayer = nullptr;
    QCamera* _camera = nullptr;
    QImageCapture* _imageCapture = nullptr;
    QMediaCaptureSession* _captureSession = nullptr;
    QMediaDevices* _mediaDevices = nullptr;

    std::unique_ptr<QtVideoSinkRouter> _sinkRouter;

    /// Per-frame watchdog interval, stashed in start() and applied to the
    /// delivery watchdog in startDecoding(). The delivery endpoint owns the timer.
    std::chrono::milliseconds _watchdogInterval{0};

    /// Connections and timer used for async start-result signalling (receiverStarted / receiverError).
    /// The timer is a value member — it lives as long as this receiver and is
    /// rewired per start() call. Single-shot semantics are set in the ctor body.
    QMetaObject::Connection _startPlayingConn;
    QMetaObject::Connection _startBufferConn;
    QMetaObject::Connection _startTimeoutConn;
    QTimer _startTimeoutTimer;

    void _clearStartHandlers();
    bool _ensureCameraObjects();
    void _checkCameraPermission();
    void _startPlayback(uint32_t timeout);
    void _stopPlayback();
    void _startCamera(uint32_t timeout);
    void _stopCamera();
    void _openCamera();
    void _closeCamera();
    void _announceCameraFormat();
    void _setSinkTargetForMode(SourceMode mode);
    void _handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void _applyPlaybackTrackPolicy();

    void _setStreamingActive(bool active);
    void _setDecoderActive(bool active);

    SourceMode _sourceMode = SourceMode::Playback;
    SourceMode _activeMode = SourceMode::Playback;
    bool _streamingActive = false;
    bool _decoderActive = false;
    Qt::PermissionStatus _cameraPermissionStatus = Qt::PermissionStatus::Undetermined;
    QString _cameraId;
};
