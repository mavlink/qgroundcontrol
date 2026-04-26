#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include <functional>

class MavlinkCameraControlInterface;

Q_DECLARE_LOGGING_CATEGORY(CameraControlBridgeLog)

/// Isolates the MAVLink camera-control protocol from VideoManager. Routes
/// camera-originated `local*Requested` signals up as its own signals, and
/// pushes the facade's aggregate video state (hasVideo/decoding/recording)
/// down to the camera via `setVideoState()`. Stream stop/resume on vehicle
/// swap also funnels through here so VideoManager never talks to the
/// camera-control interface directly.
///
/// Lifetime: owned by VideoManager. `setCamera()` rebinds on active-vehicle
/// changes; `setCamera(nullptr)` clears and stops the outgoing camera's
/// stream.
class CameraControlBridge : public QObject
{
    Q_OBJECT

public:
    struct State
    {
        bool hasVideo = false;
        bool decoding = false;
        bool recording = false;
    };

    /// Snapshots the current aggregate state each time `pushState()` runs.
    /// Must stay valid for the bridge's lifetime (captures outlive calls).
    using StateProvider = std::function<State()>;

    explicit CameraControlBridge(StateProvider provider, QObject* parent = nullptr);
    ~CameraControlBridge() override;

    /// Rebinds to `camera`. Stops the outgoing camera's stream, resumes the
    /// incoming camera's stream, and pushes current state to it. Pass
    /// nullptr to clear (no active vehicle).
    void setCamera(MavlinkCameraControlInterface* camera);

    /// Snapshots state via the provider and forwards to the bound camera.
    /// No-op when nothing is bound.
    void pushState();

signals:
    /// Mirrors of the camera's local-* signals. Facade wires these to its
    /// record/grab verbs.
    void localRecordingRequested();
    void localRecordingStopRequested();
    void localImageCaptureRequested(const QString& path);

private:
    void _connectSignals(MavlinkCameraControlInterface* camera);
    void _disconnectSignals();

    QPointer<MavlinkCameraControlInterface> _camera;
    QList<QMetaObject::Connection> _conns;
    StateProvider _stateProvider;
};
