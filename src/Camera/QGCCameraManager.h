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

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>

Q_DECLARE_LOGGING_CATEGORY(CameraManagerLog)

class Joystick;
class SimulatedCameraControl;
class Vehicle;
class CameraMetaData;
class QGCCameraManagerTest;

//-----------------------------------------------------------------------------
/// Camera Manager
class QGCCameraManager : public QObject
{
    Q_OBJECT

    friend class QGCCameraManagerTest;
public:
    QGCCameraManager(Vehicle* vehicle);
    virtual ~QGCCameraManager();

    static void registerQmlTypes();

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

    /// Returns a list of CameraMetaData objects for available cameras on the vehicle.
    virtual const QVariantList &cameraList();

    // This is public to avoid some circular include problems caused by statics
    class CameraStruct : public QObject {
    public:
        CameraStruct(QObject* parent, uint8_t compID_, Vehicle* vehicle);
        QElapsedTimer lastHeartbeat;
        bool        infoReceived    = false;
        uint8_t     compID          = 0;
        Vehicle*    vehicle         = nullptr;
    };

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
    virtual void    _requestCameraInfo      (CameraStruct* cameraInfo);
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

    static QList<CameraMetaData*> _parseCameraMetaData(const QString &jsonFilePath);

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
    static QVariantList _cameraList; ///< Standard QGC camera list
};
