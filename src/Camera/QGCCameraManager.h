#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkLib.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(CameraManagerLog)

class CameraDiscoveryStateMachine;
class CameraMetaData;
class Joystick;
class MavlinkCameraControl;
class QGCCameraManagerTest;
class QGCVideoStreamInfo;
class SimulatedCameraControl;

/// Camera Manager
class QGCCameraManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Joystick.h")
    Q_MOC_INCLUDE("MavlinkCameraControl.h")

    Q_PROPERTY(QmlObjectListModel* cameras READ cameras NOTIFY camerasChanged)
    Q_PROPERTY(QStringList cameraLabels READ cameraLabels NOTIFY cameraLabelsChanged)
    Q_PROPERTY(MavlinkCameraControl* currentCameraInstance READ currentCameraInstance NOTIFY currentCameraChanged)
    Q_PROPERTY(int currentCamera READ currentCamera WRITE setCurrentCamera NOTIFY currentCameraChanged)
    Q_PROPERTY(int currentZoomLevel READ currentZoomLevel NOTIFY currentZoomLevelChanged)

    friend class QGCCameraManagerTest;
    friend class CameraDiscoveryStateMachine;

public:
    explicit QGCCameraManager(Vehicle* vehicle);
    ~QGCCameraManager();

    QmlObjectListModel* cameras() { return &_cameras; }
    const QmlObjectListModel* cameras() const { return &_cameras; }
    QStringList cameraLabels() const { return _cameraLabels; }
    int currentCamera() const { return _currentCameraIndex; }
    MavlinkCameraControl* currentCameraInstance();
    void setCurrentCamera(int sel);
    QGCVideoStreamInfo* currentStreamInstance();
    QGCVideoStreamInfo* thermalStreamInstance();

    const QVariantList& cameraList() const;

    Vehicle* vehicle() const { return _vehicle; }

    /// Find the discovery state machine for a camera component
    CameraDiscoveryStateMachine* findDiscoveryMachine(uint8_t compId) const;

    int currentZoomLevel() const;
    double aspectForComp(int compId) const;
    double currentCameraAspect();
    Q_INVOKABLE void requestCameraFovForComp(int compId);

private:
    int _zoomValueCurrent = 0;

signals:
    void camerasChanged();
    void cameraLabelsChanged();
    void currentCameraChanged();
    void streamChanged();

    void currentZoomLevelChanged();

protected slots:
    void _vehicleReady(bool ready);
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _activeJoystickChanged(Joystick* joystick);
    void _stepZoom(int direction);
    void _startZoom(int direction);
    void _stopZoom();
    void _stepCamera(int direction);
    void _stepStream(int direction);
    void _checkForLostCameras();
    void _triggerCamera();
    void _startVideoRecording();
    void _stopVideoRecording();
    void _toggleVideoRecording();

private slots:
    void _setCurrentZoomLevel(int level);

private slots:
    void _onDiscoveryComplete(uint8_t compId);
    void _onDiscoveryFailed(uint8_t compId);

private:
    MavlinkCameraControl* _findCamera(int id);
    void _handleHeartbeat(const mavlink_message_t& message);
    void _handleCameraInfo(const mavlink_message_t& message);
    void _handleStorageInfo(const mavlink_message_t& message);
    void _handleCameraSettings(const mavlink_message_t& message);
    void _handleParamAck(const mavlink_message_t& message);
    void _handleParamValue(const mavlink_message_t& message);
    void _handleCaptureStatus(const mavlink_message_t& message);
    void _handleVideoStreamInfo(const mavlink_message_t& message);
    void _handleVideoStreamStatus(const mavlink_message_t& message);
    void _handleBatteryStatus(const mavlink_message_t& message);
    void _handleTrackingImageStatus(const mavlink_message_t& message);
    void _addCameraControlToLists(MavlinkCameraControl* cameraControl);
    void _handleCameraFovStatus(const mavlink_message_t& message);

    QPointer<Vehicle> _vehicle;
    QPointer<SimulatedCameraControl> _simulatedCameraControl;
    QPointer<Joystick> _activeJoystick;
    bool _vehicleReadyState = false;
    int _currentTask = 0;
    QmlObjectListModel _cameras;
    QStringList _cameraLabels;
    int _currentCameraIndex = 0;
    QElapsedTimer _lastZoomChange;
    QElapsedTimer _lastCameraChange;
    QTimer _camerasLostHeartbeatTimer;
    QMap<uint8_t, CameraDiscoveryStateMachine*> _discoveryMachines;
    static QVariantList _cameraList;

    QHash<int, double> _aspectByCompId;
};
