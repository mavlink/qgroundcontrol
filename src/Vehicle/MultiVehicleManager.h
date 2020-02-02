/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef MultiVehicleManager_H
#define MultiVehicleManager_H

#include "Vehicle.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"
#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"

class FirmwarePluginManager;
class FollowMe;
class JoystickManager;
class QGCApplication;
class MAVLinkProtocol;

Q_DECLARE_LOGGING_CATEGORY(MultiVehicleManagerLog)

class MultiVehicleManager : public QGCTool
{
    Q_OBJECT

public:
    MultiVehicleManager(QGCApplication* app, QGCToolbox* toolbox);

    Q_INVOKABLE void        saveSetting (const QString &key, const QString& value);
    Q_INVOKABLE QString     loadSetting (const QString &key, const QString& defaultValue);

    Q_PROPERTY(bool                 activeVehicleAvailable          READ activeVehicleAvailable                                         NOTIFY activeVehicleAvailableChanged)
    Q_PROPERTY(bool                 parameterReadyVehicleAvailable  READ parameterReadyVehicleAvailable                                 NOTIFY parameterReadyVehicleAvailableChanged)
    /// The current, active vehicle
    Q_PROPERTY(Vehicle*             activeVehicle                   READ activeVehicle                  WRITE setActiveVehicle          NOTIFY activeVehicleChanged)
    /// The list of all connected vehicles
    Q_PROPERTY(QmlObjectListModel*  vehicles                        READ vehicles                                                       CONSTANT)
    /// Enable sending heartbeats to the vehicle (defaults to true)
    Q_PROPERTY(bool                 gcsHeartBeatEnabled             READ gcsHeartbeatEnabled            WRITE setGcsHeartbeatEnabled    NOTIFY gcsHeartBeatEnabledChanged)
    /// A disconnected vehicle used for offline editing. It will match the vehicle type specified in Settings.
    Q_PROPERTY(Vehicle*             offlineEditingVehicle           READ offlineEditingVehicle                                          CONSTANT)
    /// The current vehicle's last known location
    Q_PROPERTY(QGeoCoordinate       lastKnownLocation               READ lastKnownLocation                                              NOTIFY lastKnownLocationChanged)

    // Methods

    Q_INVOKABLE Vehicle* getVehicleById(int vehicleId);

    UAS* activeUas(void) { return _activeVehicle ? _activeVehicle->uas() : nullptr; }

    // Property accessors

    bool activeVehicleAvailable(void) { return _activeVehicleAvailable; }

    bool parameterReadyVehicleAvailable(void) { return _parameterReadyVehicleAvailable; }

    Vehicle* activeVehicle(void) { return _activeVehicle; }
    void setActiveVehicle(Vehicle* vehicle);

    QmlObjectListModel* vehicles(void) { return &_vehicles; }

    bool gcsHeartbeatEnabled(void) const { return _gcsHeartbeatEnabled; }
    void setGcsHeartbeatEnabled(bool gcsHeartBeatEnabled);

    Vehicle* offlineEditingVehicle(void) { return _offlineEditingVehicle; }

    /// Determines if the link is in use by a Vehicle
    ///     @param link Link to test against
    ///     @param skipVehicle Don't consider this Vehicle as part of the test
    /// @return true: link is in use by one or more Vehicles
    bool linkInUse(LinkInterface* link, Vehicle* skipVehicle);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    QGeoCoordinate lastKnownLocation    () { return _lastKnownLocation; }

signals:
    void vehicleAdded                   (Vehicle* vehicle);
    void vehicleRemoved                 (Vehicle* vehicle);
    void activeVehicleAvailableChanged  (bool activeVehicleAvailable);
    void parameterReadyVehicleAvailableChanged(bool parameterReadyVehicleAvailable);
    void activeVehicleChanged           (Vehicle* activeVehicle);
    void gcsHeartBeatEnabledChanged     (bool gcsHeartBeatEnabled);
    void lastKnownLocationChanged       ();
#ifndef DOXYGEN_SKIP
    void _deleteVehiclePhase2Signal     (void);
#endif

private slots:
    void _deleteVehiclePhase1           (Vehicle* vehicle);
    void _deleteVehiclePhase2           (void);
    void _setActiveVehiclePhase2        (void);
    void _vehicleParametersReadyChanged (bool parametersReady);
    void _sendGCSHeartbeat              (void);
    void _vehicleHeartbeatInfo          (LinkInterface* link, int vehicleId, int componentId, int vehicleFirmwareType, int vehicleType);
    void _requestProtocolVersion        (unsigned version);
    void _coordinateChanged             (QGeoCoordinate coordinate);

private:
    bool _vehicleExists(int vehicleId);

    bool        _activeVehicleAvailable;            ///< true: An active vehicle is available
    bool        _parameterReadyVehicleAvailable;    ///< true: An active vehicle with ready parameters is available
    Vehicle*    _activeVehicle;                     ///< Currently active vehicle from a ui perspective
    Vehicle*    _offlineEditingVehicle;             ///< Disconnected vechicle used for offline editing

    QList<Vehicle*> _vehiclesBeingDeleted;          ///< List of Vehicles being deleted in queued phases
    Vehicle*        _vehicleBeingSetActive;         ///< Vehicle being set active in queued phases

    QList<int>  _ignoreVehicleIds;          ///< List of vehicle id for which we ignore further communication

    QmlObjectListModel  _vehicles;

    FirmwarePluginManager*      _firmwarePluginManager;
    JoystickManager*            _joystickManager;
    MAVLinkProtocol*            _mavlinkProtocol;
    QGeoCoordinate              _lastKnownLocation;

    QTimer              _gcsHeartbeatTimer;             ///< Timer to emit heartbeats
    bool                _gcsHeartbeatEnabled;           ///< Enabled/disable heartbeat emission
    static const int    _gcsHeartbeatRateMSecs = 1000;  ///< Heartbeat rate
    static const char*  _gcsHeartbeatEnabledKey;
};

#endif
