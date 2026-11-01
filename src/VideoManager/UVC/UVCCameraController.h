#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

class QCamera;
class QMediaCaptureSession;
class QMediaDevices;
class QQuickItem;
class QTimer;
class QVideoSink;
class UVCStreamMonitor;

/// The USB (UVC) camera VideoManager selected: owns its QCamera and capture session, measures its frames and takes its
/// screenshots. The user hears of camera errors and of the camera being unplugged.
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

    /// Displays the camera in @p videoOutput, a QML VideoOutput, replacing any other.
    Q_INVOKABLE void registerVideoOutput(QQuickItem* videoOutput);
    /// Stops displaying in @p videoOutput, if it is the registered output.
    Q_INVOKABLE void unregisterVideoOutput(QQuickItem* videoOutput);

    /// True while the camera runs into a registered VideoOutput.
    bool canTakeScreenshot() const;
    /// Saves the frame on display to @p file. False, with a warning logged, without one.
    bool takeScreenshot(const QString& file) const;

signals:
    /// A camera was plugged in or unplugged.
    void camerasChanged();
    void formatsChanged();

private:
    void _createCamera();
    void _releaseCamera();
    /// Connects the camera, the registered VideoOutput and the monitor, and runs the camera while both exist.
    void _connectVideoOutput();
    /// Replaces the camera with a new one for the selected device, if one is attached.
    void _recreateCamera();
    void _onCameraError(const QString& errorString);
    void _onVideoInputsChanged();
    /// Whether the camera in use is attached; without one, whether a camera of the selected name is.
    bool _cameraAttached() const;
    QVideoSink* _videoSink() const;

    UVCStreamMonitor* const _monitor;
    QMediaDevices* const _mediaDevices;
    QTimer* const _retryTimer;
    QCamera* _camera = nullptr;
    QMediaCaptureSession* _captureSession = nullptr;
    QPointer<QQuickItem> _videoOutput;
    QString _deviceName;
    QString _formatName;
    QStringList _formats;
    QByteArray _cameraId;
    int _retries = 0;
};
