/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QmlObjectListModel.h"
#include "MavlinkCameraControl.h"

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(CameraManagerLog)

class Joystick;
class SimulatedCameraControl;

//-----------------------------------------------------------------------------
/// Camera Manager
class QGCCameraManager : public QObject
{
    Q_OBJECT
public:
    QGCCameraManager(Vehicle* vehicle);
    virtual ~QGCCameraManager();

    Q_PROPERTY(QmlObjectListModel*      cameras                 READ cameras                                        NOTIFY camerasChanged)
    Q_PROPERTY(QStringList              cameraLabels            READ cameraLabels                                   NOTIFY cameraLabelsChanged)
    Q_PROPERTY(MavlinkCameraControl*    currentCameraInstance   READ currentCameraInstance                          NOTIFY currentCameraChanged)
    Q_PROPERTY(int                      currentCamera           READ currentCamera      WRITE  setCurrentCamera     NOTIFY currentCameraChanged)

    
    virtual QmlObjectListModel*     cameras             ()          { return &_cameras; }       ///< List of cameras provided by current vehicle
    virtual QStringList             cameraLabels        ()          { return _cameraLabels; }   ///< Camera names to show the user (for selection)
    virtual int                     currentCamera       ()          { return _currentCameraIndex; }  ///< Current selected camera
    virtual MavlinkCameraControl*   currentCameraInstance();
    virtual void                    setCurrentCamera    (int sel);
    virtual QGCVideoStreamInfo*     currentStreamInstance();
    virtual QGCVideoStreamInfo*     thermalStreamInstance();

signals:
    void    camerasChanged          ();
    void    cameraLabelsChanged     ();
    void    currentCameraChanged    ();
    void    streamChanged           ();

protected slots:
    virtual void    _vehicleReady           (bool ready);
    virtual void    _mavlinkMessageReceived (const mavlink_message_t& message);
    virtual void    _activeJoystickChanged  (Joystick* joystick);
    virtual void    _stepZoom               (int direction);
    virtual void    _startZoom              (int direction);
    virtual void    _stopZoom               ();
    virtual void    _stepCamera             (int direction);
    virtual void    _stepStream             (int direction);
    virtual void    _checkForLostCameras    ();
    virtual void    _triggerCamera          ();
    virtual void    _startVideoRecording    ();
    virtual void    _stopVideoRecording     ();
    virtual void    _toggleVideoRecording   ();

protected:
    virtual MavlinkCameraControl* _findCamera(int id);
    virtual void    _requestCameraInfo      (int compID, int tryCount);
    virtual void    _handleHeartbeat        (const mavlink_message_t& message);
    virtual void    _handleCameraInfo       (const mavlink_message_t& message);
    virtual void    _handleStorageInfo      (const mavlink_message_t& message);
    virtual void    _handleCameraSettings   (const mavlink_message_t& message);
    virtual void    _handleParamAck         (const mavlink_message_t& message);
    virtual void    _handleParamValue       (const mavlink_message_t& message);
    virtual void    _handleCaptureStatus    (const mavlink_message_t& message);
    virtual void    _handleVideoStreamInfo  (const mavlink_message_t& message);
    virtual void    _handleVideoStreamStatus(const mavlink_message_t& message);
    virtual void    _handleBatteryStatus    (const mavlink_message_t& message);
    virtual void    _handleTrackingImageStatus(const mavlink_message_t& message);
    virtual void    _addCameraControlToLists(MavlinkCameraControl* cameraControl);

protected:

    class CameraStruct : public QObject {
    public:
        CameraStruct(QObject* parent, uint8_t compID_);
        QElapsedTimer lastHeartbeat;
        bool    infoReceived = false;
        bool    gaveUp       = false;
        int     tryCount     = 0;
        uint8_t compID       = 0;
    };

    Vehicle*            _vehicle            = nullptr;
    Joystick*           _activeJoystick     = nullptr;
    bool                _vehicleReadyState  = false;
    int                 _currentTask        = 0;
    QmlObjectListModel  _cameras;
    QStringList         _cameraLabels;
    int                 _currentCameraIndex = 0;
    QElapsedTimer       _lastZoomChange;
    QElapsedTimer       _lastCameraChange;
    QTimer              _camerasLostHeartbeatTimer;
    QMap<QString, CameraStruct*> _cameraInfoRequest;
    SimulatedCameraControl* _simulatedCameraControl = nullptr;
};
