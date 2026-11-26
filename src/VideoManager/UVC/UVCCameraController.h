#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/qtvideo.h>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoReceiver.h"
#include "VideoStreamStatsMeter.h"

class QImageCapture;
class QMediaCaptureSession;
class QMediaDevices;
class QTimer;
class QVideoFrame;
class QVideoSink;
class QtVideoRecorder;

/// A USB (UVC) camera as a VideoReceiver: the camera the config's URI names by its description, which is its video
/// source name. Owns its QCamera and capture session, measures its frames, takes its photos and records it. The user
/// hears of camera errors and of the camera being unplugged.
///
/// The camera is tracked by its device id, so unplugging it is noticed even while another camera of the same name stays
/// attached; the controller then runs that one. After an error the camera is recreated up to kMaxRetries times.
///
/// The camera runs while the receiver is active and has a VideoOutput, which the camera's VideoStreamController hands
/// it. Stopped until then, streaming from the camera's first frame, and reconnecting while it waits to retry after an
/// error.
///
/// Its stats come from the frames the capture session hands the VideoOutput, about once a second while they arrive:
/// valid, fps, cameraFps, codec (the camera's capture format, such as "MJPEG" or "YUYV"), gpuPath ("CPU" for frames
/// in memory, "GPU" for textures), uptimeMs, and recordingMs and recordingBytes while it records; framesDropped is
/// -1, and the network stream fields keep their defaults.
class UVCCameraController : public VideoReceiver
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool whiteBalanceAdjustable READ whiteBalanceAdjustable NOTIFY controlsChanged FINAL)
    Q_PROPERTY(qreal minimumZoom READ minimumZoom NOTIFY controlsChanged FINAL)
    Q_PROPERTY(qreal maximumZoom READ maximumZoom NOTIFY controlsChanged FINAL)
    Q_PROPERTY(bool exposureAdjustable READ exposureAdjustable NOTIFY controlsChanged FINAL)
    Q_PROPERTY(bool focusAdjustable READ focusAdjustable NOTIFY controlsChanged FINAL)

    friend class UVCCameraControllerTest;
    friend class VideoManagerTest;
    friend class VideoQmlTest;

public:
    static constexpr int kMaxRetries = 3;
    static constexpr std::chrono::seconds kRetryInterval{2};

    /// @p clock times the stats; the steady clock by default.
    explicit UVCCameraController(QObject* parent = nullptr, VideoStreamStatsMeter::Clock clock = {});
    ~UVCCameraController() override;

    /// The selected camera's description, the config's URI; empty while none is.
    QString device() const { return config().uri; }

    /// The formats the selected camera offers, as UVCDevices::formatNames(); empty without a camera.
    QStringList formats() const { return _formats; }

    /// The format the camera runs in, named as in formats(). Empty, or a format the camera doesn't offer, lets Qt
    /// choose.
    QString format() const { return _formatName; }

    /// Runs the camera in format @p name, recreating it: not every platform switches a running camera's format.
    void setFormat(const QString& name);

    /// The picture settings the camera takes on, from VideoSettings' uvcWhiteBalance, uvcZoom, uvcExposureCompensation
    /// and uvcFocusMode. Each applies where the camera supports it: as the *Adjustable() and *Zoom() properties say.
    struct Controls
    {
        QCamera::WhiteBalanceMode whiteBalance = QCamera::WhiteBalanceAuto;
        float zoom = 1.f;
        float exposureCompensation = 0.f;
        QCamera::FocusMode focusMode = QCamera::FocusModeAuto;

        friend bool operator==(const Controls&, const Controls&) = default;
    };

    /// Applies @p controls to the camera, and to each camera from now on as it starts.
    void setControls(const Controls& controls);

    const Controls& controls() const { return _controls; }

    /// Whether the camera has a white balance besides automatic.
    bool whiteBalanceAdjustable() const;
    /// The camera's zoom range; both 1 for a camera that doesn't zoom, or without a camera.
    qreal minimumZoom() const;
    qreal maximumZoom() const;
    /// Whether the camera's exposure compensation can be set. Never on Linux, where Qt's V4L2 camera reads the focus
    /// range into the exposure compensation range.
    bool exposureAdjustable() const;
    /// Whether the camera can focus at infinity, one of the fixed focus modes. Never on Linux, where Qt's V4L2 camera
    /// doesn't read the focus range, so it can't set a focus distance.
    bool focusAdjustable() const;

    /// Turns the camera's photos clockwise by @p rotation and then mirrors them when @p mirrored, as the output
    /// showing the camera does its picture (VideoStreamController::setTransform()). Recordings keep the camera's own
    /// picture.
    void setTransform(QtVideo::Rotation rotation, bool mirrored);

    bool encodesRecordings() const override { return true; }

    /// Waits for a recording's file to be finished first: Qt writes it on its own thread, which the application's exit
    /// would cut short.
    void shutDown() override;

signals:
    /// A camera was plugged in or unplugged.
    void camerasChanged();
    void formatsChanged();
    /// The camera's adjustable controls, or its zoom range, changed.
    void controlsChanged();

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onReconnectRequested() override;
    void onVideoOutputChanged() override;
    /// Records the camera's video as QtVideoRecorder::start() does, while it runs. Recreating the camera (another
    /// device or format, an unplug, a retry) ends the recording; Qt finishes writing the file. A start it refuses,
    /// as the camera isn't running or a recording is in progress, is reported as recordingFinished() for @p file
    /// with the reason.
    void onStartRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding) override;
    void onStopRecording() override;
    /// Takes a photo, turned and mirrored as setTransform() says and saved off the GUI thread: a still capture where
    /// Qt takes one (Android, Apple), else the frame on display. Saves the frame on display too while the camera
    /// can't capture, or when a capture fails, which errorOccurred() reports if there is none by then.
    bool onTakeScreenshot(const QString& file, const VideoScreenshot::Exif& exif) override;

private:
    void _createCamera();
    void _releaseCamera();
    /// Connects the camera and the VideoOutput, and runs the camera while both exist.
    void _connectVideoOutput();
    /// Replaces the camera with a new one for the selected device, if the receiver is active and one is attached.
    void _recreateCamera();
    /// _recreateCamera() with a new run of retries.
    void _startOver();
    void _updateState();
    void _onVideoFrame(const QVideoFrame& frame);
    /// Publishes the stats of the frames counted so far.
    void _sample();
    /// A new stream starts: the measurements so far are cleared.
    void _restartStats();
    void _onCameraError(const QString& errorString);
    void _onVideoInputsChanged();
    void _onImageAvailable(int id, const QVideoFrame& frame);
    void _onImageCaptureError(int id, const QString& errorString);
    void _applyControls();

    struct PendingPhoto
    {
        QString file;
        VideoScreenshot::Exif exif;
    };

    /// Saves the frame on display in place of @p photo, whose capture won't come.
    void _savePhotoOnDisplay(const PendingPhoto& photo);
    /// Saves @p frame, turned and mirrored as setTransform() says, to @p photo's file off the GUI thread.
    void _savePhoto(const QVideoFrame& frame, const PendingPhoto& photo);
    /// Whether the camera in use is attached; without one, whether a camera of the selected name is.
    bool _cameraAttached() const;
    /// The camera runs into a VideoOutput.
    bool _running() const;

    /// The frames shown since the stats last restarted, as the latest one described them.
    struct FrameCounters
    {
        void count(const QVideoFrame& frame);

        quint64 frames = 0;
        QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
        double streamFrameRate = 0.0;
        bool gpu = false;
    };

    const VideoStreamStatsMeter::Clock _clock;
    VideoStreamStatsMeter _meter;
    FrameCounters _frameCounters;
    std::optional<qint64> _streamingSinceMs;
    QTimer* const _sampleTimer;
    QMediaDevices* const _mediaDevices;
    QTimer* const _retryTimer;
    /// Outlives the capture sessions, like _recorder.
    QImageCapture* const _imageCapture;
    QtVideoRecorder* const _recorder;
    QCamera* _camera = nullptr;
    QtVideo::Rotation _rotation = QtVideo::Rotation::None;
    bool _mirrored = false;
    Controls _controls;
    QMediaCaptureSession* _captureSession = nullptr;
    QMetaObject::Connection _frameConnection;
    /// The output's sink that _frameConnection and the capture session were last given.
    QPointer<QVideoSink> _connectedSink;
    QString _formatName;
    QStringList _formats;
    QByteArray _cameraId;
    int _retries = 0;
    /// The camera in use has shown a frame.
    bool _framesArriving = false;
    /// The camera in use failed, with its retries used up.
    bool _gaveUp = false;

    /// The photos requested and not taken yet, by request id.
    QHash<int, PendingPhoto> _photoFiles;
};
