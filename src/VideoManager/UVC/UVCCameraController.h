#pragma once

#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/qtvideo.h>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QtVideoRecorder.h"
#include "VideoReceiver.h"

class QImageCapture;
class QMediaCaptureSession;
class QMediaDevices;
class QQuickItem;
class QTimer;
class QVideoFrame;
class UVCStreamMonitor;

/// A USB (UVC) camera as a VideoReceiver: the camera the config's URI names by its description, which is its video
/// source name. Owns its QCamera and capture session, measures its frames, takes its photos and records it. The user
/// hears of camera errors and of the camera being unplugged.
///
/// The camera is tracked by its device id, so unplugging it is noticed even while another camera of the same name stays
/// attached; the controller then runs that one. After an error the camera is recreated up to kMaxRetries times.
///
/// The camera runs while the receiver is active and has a VideoOutput: a UVCVideoOutput.qml, which registers itself
/// once it is created and turns and mirrors the picture as rotation and mirrored say, with its orientation and mirrored
/// properties. Stopped until then, streaming from the camera's first frame, and reconnecting while it waits to retry
/// after an error.
class UVCCameraController : public VideoReceiver
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("UVCStreamMonitor.h")

    Q_PROPERTY(UVCStreamMonitor* monitor READ monitor CONSTANT FINAL)
    Q_PROPERTY(QStringList formats READ formats NOTIFY formatsChanged FINAL)
    Q_PROPERTY(QStringList recordingCodecs READ recordingCodecs CONSTANT FINAL)
    Q_PROPERTY(bool whiteBalanceAdjustable READ whiteBalanceAdjustable NOTIFY controlsChanged FINAL)
    Q_PROPERTY(qreal minimumZoom READ minimumZoom NOTIFY controlsChanged FINAL)
    Q_PROPERTY(qreal maximumZoom READ maximumZoom NOTIFY controlsChanged FINAL)
    Q_PROPERTY(bool exposureAdjustable READ exposureAdjustable NOTIFY controlsChanged FINAL)
    Q_PROPERTY(bool focusAdjustable READ focusAdjustable NOTIFY controlsChanged FINAL)
    Q_PROPERTY(int rotation READ rotation NOTIFY transformChanged FINAL)
    Q_PROPERTY(bool mirrored READ mirrored NOTIFY transformChanged FINAL)

    friend class UVCCameraControllerTest;
    friend class VideoManagerTest;

public:
    static constexpr int kMaxRetries = 3;
    static constexpr std::chrono::seconds kRetryInterval{2};

    explicit UVCCameraController(QObject* parent = nullptr);
    ~UVCCameraController() override;

    UVCStreamMonitor* monitor() const { return _monitor; }

    /// The selected camera's description, the config's URI; empty while none is.
    QString device() const { return config().uri; }

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

    /// For QML: setVideoOutput(), with @p videoOutput to display the camera in it, replacing any other, and with none
    /// to stop displaying in @p videoOutput if it is the output set. That one happens on the next event loop pass,
    /// unless another output registered by then: it takes the camera over without the camera stopping.
    Q_INVOKABLE void registerVideoOutput(QQuickItem* videoOutput);
    Q_INVOKABLE void unregisterVideoOutput(QQuickItem* videoOutput);

    /// Turns the camera's picture clockwise by @p rotation and then mirrors it when @p mirrored, on display and in
    /// photos; recordings keep the camera's own picture. On display the VideoOutput does it, on top of the frames' own
    /// rotation and mirroring: see rotation() and mirrored().
    void setTransform(QtVideo::Rotation rotation, bool mirrored);

    /// The clockwise rotation, in degrees, and the mirroring after it, that a VideoOutput showing the camera applies
    /// as its orientation and mirrored properties.
    int rotation() const { return static_cast<int>(_rotation); }

    bool mirrored() const { return _mirrored; }

    bool encodesRecordings() const override { return true; }

    /// Waits for a recording's file to be finished first: Qt writes it on its own thread, which the application's exit
    /// would cut short.
    void shutDown() override;

    /// For QML: QtVideoRecorder::recordingCodecs() and recordingCodecName().
    static QStringList recordingCodecs() { return QtVideoRecorder::recordingCodecs(); }

    Q_INVOKABLE static QString recordingCodecName(const QString& key)
    {
        return QtVideoRecorder::recordingCodecName(key);
    }

signals:
    /// A camera was plugged in or unplugged.
    void camerasChanged();
    void formatsChanged();
    /// The camera's adjustable controls, or its zoom range, changed.
    void controlsChanged();
    void transformChanged();

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onReconnectRequested() override;
    void onVideoOutputChanged() override;
    /// Records the camera's video as QtVideoRecorder::start() does, while it runs. Recreating the camera (another
    /// device or format, an unplug, a retry) ends the recording; Qt finishes writing the file.
    void onStartRecording(const QString& file, FileFormat format) override;
    void onStopRecording() override;
    /// Takes a photo, turned and mirrored as setTransform() says and saved off the GUI thread: a still capture where
    /// Qt takes one (Android, Apple), else the frame on display. Saves the frame on display too while the camera
    /// can't capture, or when a capture fails, which errorOccurred() reports if there is none by then.
    bool onTakeScreenshot(const QString& file, const VideoScreenshot::Exif& exif) override;

private:
    void _createCamera();
    void _releaseCamera();
    /// Connects the camera, the VideoOutput and the monitor, and runs the camera while both exist.
    void _connectVideoOutput();
    /// Replaces the camera with a new one for the selected device, if the receiver is active and one is attached.
    void _recreateCamera();
    /// _recreateCamera() with a new run of retries.
    void _startOver();
    void _updateState();
    void _onVideoFrame(const QVideoFrame& frame);
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

    UVCStreamMonitor* const _monitor;
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
