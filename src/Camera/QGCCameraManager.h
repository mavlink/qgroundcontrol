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

//-----------------------------------------------------------------------------
class QGCCameraManager : public QObject
{
    Q_OBJECT
public:
    QGCCameraManager(Vehicle* vehicle);
    virtual ~QGCCameraManager();

    Q_PROPERTY(QmlObjectListModel*  cameras             READ cameras            NOTIFY camerasChanged)
    Q_PROPERTY(QStringList          cameraLabels        READ cameraLabels       NOTIFY cameraLabelsChanged)
    Q_PROPERTY(int                  currentCamera       READ currentCamera      WRITE  setCurrentCamera     NOTIFY currentCameraChanged)

    //-- Return a list of cameras provided by this vehicle
    virtual QmlObjectListModel* cameras             () { return &_cameras; }
    //-- Camera names to show the user (for selection)
    virtual QStringList          cameraLabels       () { return _cameraLabels; }
    //-- Current selected camera
    virtual int                 currentCamera       () { return _currentCamera; }
    //-- Set current camera
    virtual void                setCurrentCamera    (int sel);

signals:
    void    camerasChanged          ();
    void    cameraLabelsChanged     ();
    void    currentCameraChanged    ();

protected slots:
    virtual void    _vehicleReady           (bool ready);
    virtual void    _mavlinkMessageReceived (const mavlink_message_t& message);

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

protected:
    Vehicle*            _vehicle;
    bool                _vehicleReadyState;
    int                 _currentTask;
    QmlObjectListModel  _cameras;
    QStringList         _cameraLabels;
    QMap<int, bool>     _cameraInfoRequested;
    int                 _currentCamera;
};
