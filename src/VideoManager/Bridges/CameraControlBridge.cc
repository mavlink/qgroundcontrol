#include "CameraControlBridge.h"

#include "MavlinkCameraControlInterface.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CameraControlBridgeLog, "Video.CameraControlBridge")

CameraControlBridge::CameraControlBridge(StateProvider provider, QObject* parent)
    : QObject(parent),
      _stateProvider(std::move(provider))
{
}

CameraControlBridge::~CameraControlBridge() = default;

void CameraControlBridge::setCamera(MavlinkCameraControlInterface* camera)
{
    if (_camera.data() == camera)
        return;

    // Quiesce the outgoing camera first so its decoder tears down before
    // signals are dropped.
    if (_camera)
        _camera->stopStream();

    _disconnectSignals();
    _camera = camera;

    if (_camera) {
        _camera->resumeStream();
        _connectSignals(_camera);
        pushState();
    }
}

void CameraControlBridge::pushState()
{
    if (!_camera || !_stateProvider)
        return;

    const State s = _stateProvider();
    _camera->setVideoState(s.hasVideo, s.decoding, s.recording);
}

void CameraControlBridge::_connectSignals(MavlinkCameraControlInterface* camera)
{
    _conns << connect(camera, &MavlinkCameraControlInterface::localRecordingRequested,
                      this, &CameraControlBridge::localRecordingRequested);
    _conns << connect(camera, &MavlinkCameraControlInterface::localRecordingStopRequested,
                      this, &CameraControlBridge::localRecordingStopRequested);
    _conns << connect(camera, &MavlinkCameraControlInterface::localImageCaptureRequested,
                      this, &CameraControlBridge::localImageCaptureRequested);
}

void CameraControlBridge::_disconnectSignals()
{
    for (const auto& conn : std::as_const(_conns))
        QObject::disconnect(conn);
    _conns.clear();
}
