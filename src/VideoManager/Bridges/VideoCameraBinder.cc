#include "VideoCameraBinder.h"

#include "CameraControlBridge.h"
#include "QGCCameraManager.h"
#include "QGCLoggingCategory.h"
#include "VideoStreamOrchestrator.h"

#include <utility>

QGC_LOGGING_CATEGORY(VideoCameraBinderLog, "Video.VideoCameraBinder")

VideoCameraBinder::VideoCameraBinder(CameraManagerApplier applier, QObject* parent)
    : QObject(parent)
    , _applier(std::move(applier))
{
}

VideoCameraBinder::VideoCameraBinder(VideoStreamOrchestrator* orchestrator,
                                     CameraControlBridge* cameraBridge,
                                     QObject* parent)
    : VideoCameraBinder(
          [orchestrator, cameraBridge](QGCCameraManager* cameraManager) {
              if (orchestrator)
                  orchestrator->setCameraManager(cameraManager);
              if (cameraBridge)
                  cameraBridge->setCamera(cameraManager ? cameraManager->currentCameraInstance() : nullptr);
          },
          parent)
{
}

VideoCameraBinder::~VideoCameraBinder() = default;

void VideoCameraBinder::setCameraManager(QGCCameraManager* cameraManager)
{
    if (_recording) {
        qCInfo(VideoCameraBinderLog)
            << "Recording in progress; deferring camera/stream swap until stop";
        _deferredCameraManager = cameraManager;
        _hasDeferredCameraManager = true;
        return;
    }

    _applyCameraManager(cameraManager);
}

void VideoCameraBinder::setRecording(bool recording)
{
    if (_recording == recording)
        return;

    _recording = recording;
    if (_recording || !_hasDeferredCameraManager)
        return;

    QGCCameraManager* cameraManager = _deferredCameraManager.data();
    _deferredCameraManager.clear();
    _hasDeferredCameraManager = false;
    _applyCameraManager(cameraManager);
}

void VideoCameraBinder::_applyCameraManager(QGCCameraManager* cameraManager)
{
    if (_applier)
        _applier(cameraManager);
}
