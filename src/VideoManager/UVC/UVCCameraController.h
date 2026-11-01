#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QCamera;
class QMediaCaptureSession;
class QMediaDevices;
class QQuickItem;
class QVideoSink;
class UVCStreamMonitor;

/// The USB (UVC) camera VideoManager selected: owns its QCamera and capture session, measures its frames and takes its
/// screenshots. The user hears of camera errors and of the camera being unplugged.
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

    friend class UVCCameraControllerTest;

public:
    explicit UVCCameraController(QObject* parent = nullptr);
    ~UVCCameraController() override;

    UVCStreamMonitor* monitor() const { return _monitor; }

    /// The selected camera's description; empty while none is.
    QString device() const { return _deviceName; }

    /// Selects the attached camera of description @p description, or none for an empty one.
    void setDevice(const QString& description);
    /// Emits camerasChanged() from now on when a camera is plugged in or unplugged.
    void watchCameras();

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

private:
    void _createCamera();
    void _releaseCamera();
    /// Connects the camera, the registered VideoOutput and the monitor, and runs the camera while both exist.
    void _connectVideoOutput();
    void _onCameraError(const QString& errorString);
    void _onVideoInputsChanged();
    QVideoSink* _videoSink() const;

    UVCStreamMonitor* const _monitor;
    QMediaDevices* const _mediaDevices;
    QCamera* _camera = nullptr;
    QMediaCaptureSession* _captureSession = nullptr;
    QPointer<QQuickItem> _videoOutput;
    QString _deviceName;
};
