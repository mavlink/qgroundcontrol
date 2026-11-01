#pragma once

#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/qtvideo.h>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoScreenshot.h"

class QImageCapture;
class QMediaCaptureSession;
class QMediaDevices;
class QMediaRecorder;
class QQuickItem;
class QTimer;
class QVideoFrame;
class QVideoSink;
class UVCFrameRelay;
class UVCStreamMonitor;

/// The USB (UVC) camera VideoManager selected: owns its QCamera and capture session, measures its frames, takes its
/// photos and records it. The user hears of camera errors and of the camera being unplugged.
///
/// The camera is tracked by its device id, so unplugging it is noticed even while another camera of the same name stays
/// attached; the controller then runs that one. After an error the camera is recreated up to kMaxRetries times.
///
/// The camera runs while one is selected and a QML VideoOutput is registered: registerVideoOutput() once it is created,
/// unregisterVideoOutput() when it is destroyed (see UVCVideoOutput.qml).
class UVCCameraController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("UVCStreamMonitor.h")

    Q_PROPERTY(UVCStreamMonitor* monitor READ monitor CONSTANT)
    Q_PROPERTY(QStringList formats READ formats NOTIFY formatsChanged)
    Q_PROPERTY(QStringList recordingCodecs READ recordingCodecs CONSTANT)
    Q_PROPERTY(bool whiteBalanceAdjustable READ whiteBalanceAdjustable NOTIFY controlsChanged)
    Q_PROPERTY(qreal minimumZoom READ minimumZoom NOTIFY controlsChanged)
    Q_PROPERTY(qreal maximumZoom READ maximumZoom NOTIFY controlsChanged)
    Q_PROPERTY(bool exposureAdjustable READ exposureAdjustable NOTIFY controlsChanged)
    Q_PROPERTY(bool focusAdjustable READ focusAdjustable NOTIFY controlsChanged)

    friend class UVCCameraControllerTest;

public:
    static constexpr int kMaxRetries = 3;
    static constexpr int kRetryIntervalMs = 2000;

    explicit UVCCameraController(QObject* parent = nullptr);
    ~UVCCameraController() override;

    UVCStreamMonitor* monitor() const { return _monitor; }

    /// The selected camera's description; empty while none is.
    QString device() const { return _deviceName; }

    /// Selects the attached camera of description @p description, or none for an empty one.
    void setDevice(const QString& description);
    /// Emits camerasChanged() from now on when a camera is plugged in or unplugged.
    void watchCameras();

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

    /// Displays the camera in @p videoOutput, a QML VideoOutput, replacing any other.
    Q_INVOKABLE void registerVideoOutput(QQuickItem* videoOutput);
    /// Stops displaying in @p videoOutput, if it is the registered output.
    Q_INVOKABLE void unregisterVideoOutput(QQuickItem* videoOutput);

    /// Turns the camera's picture clockwise by @p rotation and then mirrors it when @p mirrored, on display, in
    /// screenshots and in photos; recordings keep the camera's own picture. The frames themselves are turned, not the
    /// VideoOutput, so what Qt draws over them (the telemetry overlay) stays upright.
    void setTransform(QtVideo::Rotation rotation, bool mirrored);

    /// True while the camera runs into a registered VideoOutput.
    bool canTakeScreenshot() const;
    /// Saves the frame on display to @p file, as displayed, with @p exif for a JPEG. False, with a warning logged,
    /// without a VideoOutput.
    bool takeScreenshot(const QString& file, const VideoScreenshot::Exif& exif = {}) const;

    /// Takes a photo into @p file, turned and mirrored as setTransform() says and saved off the GUI thread: a still
    /// capture where Qt takes one (Android, Apple), else the frame on display. Saves the frame on display too while the
    /// camera can't capture, or when a capture fails. A JPEG carries @p exif.
    void capturePhoto(const QString& file, const VideoScreenshot::Exif& exif = {});

    /// True while the camera runs into a registered VideoOutput, so startRecording() can record it.
    bool canRecord() const { return canTakeScreenshot(); }

    bool recording() const { return _recording; }

    /// Records the camera's video to "<file without suffix>.<suffix of the format used>", trying recordingFormats() in
    /// turn until one encodes. recordingStarted() follows once the first frame is encoded, or recordingFinished() with
    /// an error; a failed attempt leaves no file behind. Recreating the camera (another device or format, an unplug, a
    /// retry) ends the recording; Qt finishes writing the file.
    void startRecording(const QString& file, QMediaFormat::FileFormat container,
                        QMediaFormat::VideoCodec codec = QMediaFormat::VideoCodec::Unspecified);
    void stopRecording();
    /// Stops recording and waits up to @p timeoutMs for the file to be finished: Qt writes it on its own thread, which
    /// the application's exit would cut short.
    void stopRecordingAndWait(int timeoutMs);

    /// The formats startRecording() tries for @p container, best first: @p codec where Qt can encode it, then H.264
    /// and the other codecs Qt can encode here, down to software MPEG-2. In @p container where Qt can write it, else in
    /// another container. Qt counts a hardware encoder as encodable even where the hardware turns it down, hence the
    /// fallbacks.
    static QList<QMediaFormat> recordingFormats(QMediaFormat::FileFormat container,
                                                QMediaFormat::VideoCodec codec = QMediaFormat::VideoCodec::Unspecified);

    /// The codecs Qt can encode a recording in here, in any of the recording containers, as the keys of
    /// QMediaFormat::VideoCodec (such as "H264"): the values of VideoSettings::uvcRecordingCodec. recordingFormats()'s
    /// fallbacks come first, in its order.
    static QStringList recordingCodecs();
    /// The codec of key @p key, as recordingCodecs() names it; Unspecified for an empty or unknown key.
    static QMediaFormat::VideoCodec recordingCodec(const QString& key);
    /// The name of the codec of key @p key, such as "H.264", for display.
    Q_INVOKABLE static QString recordingCodecName(const QString& key);

signals:
    /// A camera was plugged in or unplugged.
    void camerasChanged();
    void formatsChanged();
    /// The camera's adjustable controls, or its zoom range, changed.
    void controlsChanged();
    void recordingChanged(bool recording);
    void recordingStarted(const QString& file);
    /// @p error is empty for a recording that finished normally.
    void recordingFinished(const QString& file, const QString& error);

private:
    void _createCamera();
    void _releaseCamera();
    /// Connects the camera, the registered VideoOutput and the monitor, and runs the camera while both exist.
    void _connectVideoOutput();
    /// Replaces the camera with a new one for the selected device, if one is attached.
    void _recreateCamera();
    void _onCameraError(const QString& errorString);
    void _onVideoInputsChanged();
    void _onImageAvailable(int id, const QVideoFrame& frame);
    void _onImageCaptureError(int id, const QString& errorString);
    void _applyControls();
    /// Saves the frame on display to @p file off the GUI thread; logs a warning without a VideoOutput.
    void _savePhotoOnDisplay(const QString& file, const VideoScreenshot::Exif& exif) const;
    /// Records with the next of _pendingFormats.
    void _recordNextFormat();
    void _onRecorderStateChanged(QMediaRecorder::RecorderState state);
    void _onRecorderError(const QString& errorString);
    /// The recorder stopped before it encoded a frame: tries the next format after a failure, else reports it.
    void _onRecordingNotStarted();
    /// Whether the camera in use is attached; without one, whether a camera of the selected name is.
    bool _cameraAttached() const;
    QVideoSink* _videoSink() const;

    UVCStreamMonitor* const _monitor;
    /// Between the capture session and the registered VideoOutput.
    UVCFrameRelay* const _frameRelay;
    QMediaDevices* const _mediaDevices;
    QTimer* const _retryTimer;
    /// Outlives the capture sessions, like _recorder.
    QImageCapture* const _imageCapture;
    QCamera* _camera = nullptr;
    Controls _controls;
    QMediaCaptureSession* _captureSession = nullptr;
    /// Outlives the capture sessions: a session going away mid-recording makes it finish the file and report it.
    QMediaRecorder* _recorder = nullptr;
    QPointer<QQuickItem> _videoOutput;
    QString _deviceName;
    QString _formatName;
    QStringList _formats;
    QByteArray _cameraId;
    int _retries = 0;

    struct PendingPhoto
    {
        QString file;
        VideoScreenshot::Exif exif;
    };

    /// The photos requested and not taken yet, by request id.
    QHash<int, PendingPhoto> _photoFiles;
    QList<QMediaFormat> _pendingFormats;
    /// The file being recorded, without its suffix.
    QString _recordingBase;
    QString _recordingFile;
    QString _recordingError;
    /// From the first encoded frame until the recorder stops.
    bool _recording = false;
    bool _stopRequested = false;
};
