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

#include "QGCSingleton.h"
#include "Vehicle.h"
#include "QGCMAVLink.h"
#include "UASWaypointManager.h"

class MultiVehicleManager : public QGCSingleton
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(MultiVehicleManager, MultiVehicleManager)

public:
    Q_INVOKABLE void        saveSetting (const QString &key, const QString& value);
    Q_INVOKABLE QString     loadSetting (const QString &key, const QString& defaultValue);
    
    Q_PROPERTY(bool activeVehicleAvailable          READ activeVehicleAvailable                         NOTIFY activeVehicleAvailableChanged)
    Q_PROPERTY(bool parameterReadyVehicleAvailable  READ parameterReadyVehicleAvailable                 NOTIFY parameterReadyVehicleAvailableChanged)
    Q_PROPERTY(Vehicle* activeVehicle               READ activeVehicle          WRITE setActiveVehicle  NOTIFY activeVehicleChanged)
    Q_PROPERTY(QVariantList vehicles                READ vehiclesAsVariants                                    CONSTANT)
    
    // Property accessors
    bool activeVehicleAvailable(void) { return _activeVehicleAvailable; }
    bool parameterReadyVehicleAvailable(void) { return _parameterReadyVehicleAvailable; }
    Vehicle* activeVehicle(void) { return _activeVehicle; }
    void setActiveVehicle(Vehicle* vehicle);
    
    /// Called to notify that a heartbeat was received with the specified information. MultiVehicleManager
    /// will create/update Vehicles as necessary.
    ///     @param link Heartbeat came through on this link
    ///     @param vehicleId Mavlink system id for vehicle
    ///     @param heartbeat Mavlink heartbeat message
    /// @return true: continue further processing of this message, false: disregard this message
    bool notifyHeartbeatInfo(LinkInterface* link, int vehicleId, mavlink_heartbeat_t& heartbeat);
    
    Vehicle* getVehicleById(int vehicleId) { return _vehicleMap[vehicleId]; }
    
    void setHomePositionForAllVehicles(double lat, double lon, double alt);
    
    UAS* activeUas(void) { return _activeVehicle ? _activeVehicle->uas() : NULL; }
    
    QList<Vehicle*> vehicles(void);
    QVariantList vehiclesAsVariants(void);
    
    UASWaypointManager* activeWaypointManager(void);

signals:
    void vehicleAdded(Vehicle* vehicle);
    void vehicleRemoved(Vehicle* vehicle);
    void activeVehicleAvailableChanged(bool activeVehicleAvailable);
    void parameterReadyVehicleAvailableChanged(bool parameterReadyVehicleAvailable);
    void activeVehicleChanged(Vehicle* activeVehicle);
    
    void _deleteVehiclePhase2Signal(void);
    
private slots:
    void _deleteVehiclePhase1(void);
    void _deleteVehiclePhase2(void);
    void _setActiveVehiclePhase2(void);
    void _autopilotPluginReadyChanged(bool pluginReady);
    
private:
    /// All access to singleton is through MultiVehicleManager::instance
    MultiVehicleManager(QObject* parent = NULL);
    ~MultiVehicleManager();
    
    bool _vehicleExists(int vehicleId);
    
    bool        _activeVehicleAvailable;            ///< true: An active vehicle is available
    bool        _parameterReadyVehicleAvailable;    ///< true: An active vehicle with ready parameters is available
    Vehicle*    _activeVehicle;                     ///< Currently active vehicle from a ui perspective
    
    Vehicle*    _vehicleBeingDeleted;               ///< Vehicle being deleted in queued phases
    Vehicle*    _vehicleBeingSetActive;             ///< Vehicle being set active in queued phases
    
    QList<int>  _ignoreVehicleIds;          ///< List of vehicle id for which we ignore further communication
    
    QMap<int, Vehicle*> _vehicleMap;        ///< Map of vehicles keyed by id
    
    UASWaypointManager* _offlineWaypointManager;
};

#endif
