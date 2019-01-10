/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>
#include "QmlObjectListModel.h"
#include "QGCCameraControl.h"

#include <QObject>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(CameraManagerLog)

class Joystick;

//-----------------------------------------------------------------------------
class QGCCameraManager : public QObject
{
    Q_OBJECT
public:
    QGCCameraManager(Vehicle* vehicle);
    virtual ~QGCCameraManager();

    Q_PROPERTY(QmlObjectListModel*  cameras                 READ cameras            NOTIFY camerasChanged)
    Q_PROPERTY(QStringList          cameraLabels            READ cameraLabels       NOTIFY cameraLabelsChanged)
    Q_PROPERTY(int                  currentCamera           READ currentCamera      WRITE  setCurrentCamera     NOTIFY currentCameraChanged)
    Q_PROPERTY(QGCCameraControl*    currentCameraInstance   READ currentCameraInstance                          NOTIFY currentCameraChanged)

    //-- Return a list of cameras provided by this vehicle
    virtual QmlObjectListModel* cameras             () { return &_cameras; }
    //-- Camera names to show the user (for selection)
    virtual QStringList          cameraLabels       () { return _cameraLabels; }
    //-- Current selected camera
    virtual int                 currentCamera       () { return _currentCamera; }
    virtual QGCCameraControl*   currentCameraInstance();
    //-- Set current camera
    virtual void                setCurrentCamera    (int sel);
    //-- Current stream
    virtual QGCVideoStreamInfo* currentStreamInstance();

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
    virtual void    _stepCamera             (int direction);
    virtual void    _stepStream             (int direction);

protected:
    virtual QGCCameraControl* _findCamera   (int id);
    virtual void    _requestCameraInfo      (int compID);
    virtual void    _handleHeartbeat        (const mavlink_message_t& message);
    virtual void    _handleCameraInfo       (const mavlink_message_t& message);
    virtual void    _handleStorageInfo      (const mavlink_message_t& message);
    virtual void    _handleCameraSettings   (const mavlink_message_t& message);
    virtual void    _handleParamAck         (const mavlink_message_t& message);
    virtual void    _handleParamValue       (const mavlink_message_t& message);
    virtual void    _handleCaptureStatus    (const mavlink_message_t& message);
    virtual void    _handleVideoStreamInfo  (const mavlink_message_t& message);
    virtual void    _handleVideoStreamStatus(const mavlink_message_t& message);

protected:
    Vehicle*            _vehicle            = nullptr;
    Joystick*           _activeJoystick     = nullptr;
    bool                _vehicleReadyState  = false;
    int                 _currentTask        = 0;
    QmlObjectListModel  _cameras;
    QStringList         _cameraLabels;
    QMap<int, bool>     _cameraInfoRequested;
    int                 _currentCamera      = 0;
    QTime               _lastZoomChange;
    QTime               _lastCameraChange;
};
