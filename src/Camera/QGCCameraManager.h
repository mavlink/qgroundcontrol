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

    //-- Return a list of cameras provided by this vehicle
    virtual QmlObjectListModel* cameras             () { return &_cameras; }

signals:
    void    camerasChanged          ();

protected slots:
    void    _vehicleReady           (bool ready);
    void    _mavlinkMessageReceived (const mavlink_message_t& message);

protected:
    QGCCameraControl* _findCamera   (int id);
    void    _requestCameraInfo      (int compID);
    void    _handleHeartbeat        (const mavlink_message_t& message);
    void    _handleCameraInfo       (const mavlink_message_t& message);
    void    _handleStorageInfo      (const mavlink_message_t& message);
    void    _handleCameraSettings   (const mavlink_message_t& message);
    void    _handleParamAck         (const mavlink_message_t& message);
    void    _handleParamValue       (const mavlink_message_t& message);
    void    _handleCaptureStatus    (const mavlink_message_t& message);

protected:
    Vehicle*            _vehicle;
    bool                _vehicleReadyState;
    int                 _currentTask;
    QmlObjectListModel  _cameras;
    QMap<int, bool>     _cameraInfoRequested;
};
