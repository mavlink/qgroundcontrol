#pragma once

#include <functional>

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>

class CameraControlBridge;
class QGCCameraManager;
class VideoStreamOrchestrator;

Q_DECLARE_LOGGING_CATEGORY(VideoCameraBinderLog)

/// Owns the active camera-manager binding for the video stack. It applies
/// camera changes to the stream orchestrator and camera-control bridge, and
/// defers camera swaps while recording so an in-progress recording is not
/// restarted against a different vehicle stream.
class VideoCameraBinder : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoCameraBinder)

public:
    using CameraManagerApplier = std::function<void(QGCCameraManager*)>;

    explicit VideoCameraBinder(CameraManagerApplier applier, QObject* parent = nullptr);
    VideoCameraBinder(VideoStreamOrchestrator* orchestrator,
                      CameraControlBridge* cameraBridge,
                      QObject* parent = nullptr);
    ~VideoCameraBinder() override;

    void setCameraManager(QGCCameraManager* cameraManager);
    void setRecording(bool recording);

    [[nodiscard]] bool hasDeferredCameraManager() const { return _hasDeferredCameraManager; }

private:
    void _applyCameraManager(QGCCameraManager* cameraManager);

    CameraManagerApplier _applier;
    QPointer<QGCCameraManager> _deferredCameraManager;
    bool _hasDeferredCameraManager = false;
    bool _recording = false;
};
