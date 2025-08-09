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
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkLib.h"
#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(CameraManagerLog)

class CameraMetaData;
class Joystick;
class MavlinkCameraControl;
class QGCCameraManagerTest;
class QGCVideoStreamInfo;
class SimulatedCameraControl;
class Vehicle;

/// Camera Manager
class QGCCameraManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Joystick.h")
    Q_MOC_INCLUDE("MavlinkCameraControl.h")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(QmlObjectListModel*      cameras                 READ cameras                                        NOTIFY camerasChanged)
    Q_PROPERTY(QStringList              cameraLabels            READ cameraLabels                                   NOTIFY cameraLabelsChanged)
    Q_PROPERTY(MavlinkCameraControl*    currentCameraInstance   READ currentCameraInstance                          NOTIFY currentCameraChanged)
    Q_PROPERTY(int                      currentCamera           READ currentCamera          WRITE setCurrentCamera  NOTIFY currentCameraChanged)

    friend class QGCCameraManagerTest;
public:
    QGCCameraManager(Vehicle *vehicle);
    ~QGCCameraManager();

    struct CameraStruct {
        CameraStruct(QGCCameraManager *manager_, uint8_t compID_, Vehicle *vehicle);
        ~CameraStruct();

        bool infoReceived = false;
        uint8_t compID = 0;
        int retryCount = 0;
        QElapsedTimer lastHeartbeat;
        QTimer backoffTimer;
        Vehicle *vehicle = nullptr;
        QGCCameraManager *manager = nullptr;
    };

    void start();
    void stop();
    bool requestingEnabled() const { return _requestingEnabled; }
    QmlObjectListModel *cameras() { return &_cameras; }         ///< List of cameras provided by current vehicle
    QStringList cameraLabels() const { return _cameraLabels; }  ///< Camera names to show the user (for selection)
    int currentCamera() const { return _currentCameraIndex; }   ///< Current selected camera
    MavlinkCameraControl *currentCameraInstance();
    void setCurrentCamera(int sel);
    QGCVideoStreamInfo *currentStreamInstance();
    QGCVideoStreamInfo *thermalStreamInstance();

    /// Returns a list of CameraMetaData objects for available cameras on the vehicle.
    const QVariantList &cameraList();

    /// Helper method for static functions to access vehicle
    Vehicle *vehicle() const { return _vehicle; }

signals:
    void camerasChanged();
    void cameraLabelsChanged();
    void currentCameraChanged();
    void streamChanged();

protected slots:
    void _vehicleReady(bool ready);
    void _mavlinkMessageReceived(const mavlink_message_t &message);
    void _activeJoystickChanged(Joystick *joystick);
    void _stepZoom(int direction);
    void _startZoom(int direction);
    void _stopZoom();
    void _stepCamera(int direction);
    void _stepStream(int direction);
    /// Called to check for cameras which are no longer sending a heartbeat
    void _checkForLostCameras();
    void _triggerCamera();
    void _startVideoRecording();
    void _stopVideoRecording();
    void _toggleVideoRecording();

private:
    MavlinkCameraControl *_findCamera(int id);
    void _requestCameraInfo(CameraStruct *cameraInfo);
    void _handleHeartbeat(const mavlink_message_t &message);
    void _handleCameraInfo(const mavlink_message_t &message);
    void _handleStorageInfo(const mavlink_message_t &message);
    void _handleCameraSettings(const mavlink_message_t &message);
    void _handleParamAck(const mavlink_message_t &message);
    void _handleParamValue(const mavlink_message_t &message);
    void _handleCaptureStatus(const mavlink_message_t &message);
    void _handleVideoStreamInfo(const mavlink_message_t &message);
    void _handleVideoStreamStatus(const mavlink_message_t &message);
    void _handleBatteryStatus(const mavlink_message_t &message);
    void _handleTrackingImageStatus(const mavlink_message_t &message);
    void _addCameraControlToLists(MavlinkCameraControl *cameraControl);

    static QList<CameraMetaData*> _parseCameraMetaData(const QString &jsonFilePath);

    Vehicle *_vehicle = nullptr;
    SimulatedCameraControl *_simulatedCameraControl = nullptr;
    Joystick *_activeJoystick = nullptr;
    bool _vehicleReadyState = false;
    bool _requestingEnabled = false;
    int _currentTask = 0;
    QmlObjectListModel _cameras;
    QStringList _cameraLabels;
    int _currentCameraIndex = 0;
    QElapsedTimer _lastZoomChange;
    QElapsedTimer _lastCameraChange;
    QTimer _camerasLostHeartbeatTimer;
    QMap<QString, CameraStruct*> _cameraInfoRequest;
    static QVariantList _cameraList; ///< Standard QGC camera list
};
