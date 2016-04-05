/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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
class AutoPilotPluginManager;
class FollowMe;
class JoystickManager;
class QGCApplication;
class MAVLinkProtocol;

Q_DECLARE_LOGGING_CATEGORY(MultiVehicleManagerLog)

class MultiVehicleManager : public QGCTool
{
    Q_OBJECT

public:
    MultiVehicleManager(QGCApplication* app);

    Q_INVOKABLE void        saveSetting (const QString &key, const QString& value);
    Q_INVOKABLE QString     loadSetting (const QString &key, const QString& defaultValue);

    Q_PROPERTY(bool                 activeVehicleAvailable          READ activeVehicleAvailable                                         NOTIFY activeVehicleAvailableChanged)
    Q_PROPERTY(bool                 parameterReadyVehicleAvailable  READ parameterReadyVehicleAvailable                                 NOTIFY parameterReadyVehicleAvailableChanged)
    Q_PROPERTY(Vehicle*             activeVehicle                   READ activeVehicle                  WRITE setActiveVehicle          NOTIFY activeVehicleChanged)
    Q_PROPERTY(QmlObjectListModel*  vehicles                        READ vehicles                                                       CONSTANT)
    Q_PROPERTY(bool                 gcsHeartBeatEnabled             READ gcsHeartbeatEnabled            WRITE setGcsHeartbeatEnabled    NOTIFY gcsHeartBeatEnabledChanged)

    /// A disconnected vehicle is used to access FactGroup information for the Vehicle object when no active vehicle is available
    Q_PROPERTY(Vehicle*             disconnectedVehicle             MEMBER _disconnectedVehicle                                         CONSTANT)

    // Methods

    Q_INVOKABLE Vehicle* getVehicleById(int vehicleId);

    UAS* activeUas(void) { return _activeVehicle ? _activeVehicle->uas() : NULL; }

    // Property accessors

    bool activeVehicleAvailable(void) { return _activeVehicleAvailable; }

    bool parameterReadyVehicleAvailable(void) { return _parameterReadyVehicleAvailable; }

    Vehicle* activeVehicle(void) { return _activeVehicle; }
    void setActiveVehicle(Vehicle* vehicle);

    QmlObjectListModel* vehicles(void) { return &_vehicles; }

    bool gcsHeartbeatEnabled(void) const { return _gcsHeartbeatEnabled; }
    void setGcsHeartbeatEnabled(bool gcsHeartBeatEnabled);

    /// Determines if the link is in use by a Vehicle
    ///     @param link Link to test against
    ///     @param skipVehicle Don't consider this Vehicle as part of the test
    /// @return true: link is in use by one or more Vehicles
    bool linkInUse(LinkInterface* link, Vehicle* skipVehicle);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

signals:
    void vehicleAdded(Vehicle* vehicle);
    void vehicleRemoved(Vehicle* vehicle);
    void activeVehicleAvailableChanged(bool activeVehicleAvailable);
    void parameterReadyVehicleAvailableChanged(bool parameterReadyVehicleAvailable);
    void activeVehicleChanged(Vehicle* activeVehicle);
    void gcsHeartBeatEnabledChanged(bool gcsHeartBeatEnabled);

    void _deleteVehiclePhase2Signal(void);

private slots:
    void _deleteVehiclePhase1(Vehicle* vehicle);
    void _deleteVehiclePhase2(void);
    void _setActiveVehiclePhase2(void);
    void _autopilotParametersReadyChanged(bool parametersReady);
    void _sendGCSHeartbeat(void);
    void _vehicleHeartbeatInfo(LinkInterface* link, int vehicleId, int vehicleMavlinkVersion, int vehicleFirmwareType, int vehicleType);

private:
    bool _vehicleExists(int vehicleId);

    bool        _activeVehicleAvailable;            ///< true: An active vehicle is available
    bool        _parameterReadyVehicleAvailable;    ///< true: An active vehicle with ready parameters is available
    Vehicle*    _activeVehicle;                     ///< Currently active vehicle from a ui perspective
    Vehicle*    _disconnectedVehicle;               ///< Disconnected vechicle for FactGroup access

    QList<Vehicle*> _vehiclesBeingDeleted;          ///< List of Vehicles being deleted in queued phases
    Vehicle*        _vehicleBeingSetActive;         ///< Vehicle being set active in queued phases

    QList<int>  _ignoreVehicleIds;          ///< List of vehicle id for which we ignore further communication

    QmlObjectListModel  _vehicles;

    FirmwarePluginManager*      _firmwarePluginManager;
    AutoPilotPluginManager*     _autopilotPluginManager;
    JoystickManager*            _joystickManager;
    MAVLinkProtocol*            _mavlinkProtocol;

    QTimer              _gcsHeartbeatTimer;             ///< Timer to emit heartbeats
    bool                _gcsHeartbeatEnabled;           ///< Enabled/disable heartbeat emission
    static const int    _gcsHeartbeatRateMSecs = 1000;  ///< Heartbeat rate
    static const char*  _gcsHeartbeatEnabledKey;
};

#endif
