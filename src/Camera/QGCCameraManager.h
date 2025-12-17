/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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

public:
    explicit QGCCameraManager(Vehicle* vehicle);
    ~QGCCameraManager();

    struct CameraStruct {
        CameraStruct(QGCCameraManager* manager_, uint8_t compID_, Vehicle* vehicle_);
        ~CameraStruct();

        bool infoReceived = false;
        uint8_t compID = 0;
        int retryCount = 0;
        QElapsedTimer lastHeartbeat;
        QTimer backoffTimer;
        QPointer<Vehicle> vehicle;
        QPointer<QGCCameraManager> manager;

    private:
        Q_DISABLE_COPY_MOVE(CameraStruct)
    };

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

    CameraStruct* findCameraStruct(uint8_t compId) const { return _cameraInfoRequest.value(QString::number(compId), nullptr); }

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

private:
    MavlinkCameraControl* _findCamera(int id);
    void _requestCameraInfo(CameraStruct* cameraInfo);
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
    QMap<QString, CameraStruct*> _cameraInfoRequest;
    static QVariantList _cameraList;

    QHash<int, double> _aspectByCompId;
};
