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

#include "MultiVehicleManager.h"
#include "AutoPilotPlugin.h"
#include "MAVLinkProtocol.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "FollowMe.h"

#ifdef __mobile__
#include "MobileScreenMgr.h"
#endif

#include <QQmlEngine>

QGC_LOGGING_CATEGORY(MultiVehicleManagerLog, "MultiVehicleManagerLog")

const char* MultiVehicleManager::_gcsHeartbeatEnabledKey = "gcsHeartbeatEnabled";

MultiVehicleManager::MultiVehicleManager(QGCApplication* app)
    : QGCTool(app)
    , _activeVehicleAvailable(false)
    , _parameterReadyVehicleAvailable(false)
    , _activeVehicle(NULL)
    , _disconnectedVehicle(NULL)
    , _firmwarePluginManager(NULL)
    , _autopilotPluginManager(NULL)
    , _joystickManager(NULL)
    , _mavlinkProtocol(NULL)
    , _gcsHeartbeatEnabled(true)
{
    QSettings settings;

    _gcsHeartbeatEnabled = settings.value(_gcsHeartbeatEnabledKey, true).toBool();

    _gcsHeartbeatTimer.setInterval(_gcsHeartbeatRateMSecs);
    _gcsHeartbeatTimer.setSingleShot(false);
    connect(&_gcsHeartbeatTimer, &QTimer::timeout, this, &MultiVehicleManager::_sendGCSHeartbeat);
    if (_gcsHeartbeatEnabled) {
        _gcsHeartbeatTimer.start();
    }

    _disconnectedVehicle = new Vehicle(this);
}

void MultiVehicleManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

   _firmwarePluginManager =     _toolbox->firmwarePluginManager();
   _autopilotPluginManager =    _toolbox->autopilotPluginManager();
   _joystickManager =           _toolbox->joystickManager();
   _mavlinkProtocol =           _toolbox->mavlinkProtocol();

   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<MultiVehicleManager>("QGroundControl.MultiVehicleManager", 1, 0, "MultiVehicleManager", "Reference only");

   connect(_mavlinkProtocol, &MAVLinkProtocol::vehicleHeartbeatInfo, this, &MultiVehicleManager::_vehicleHeartbeatInfo);
}

void MultiVehicleManager::_vehicleHeartbeatInfo(LinkInterface* link, int vehicleId, int vehicleMavlinkVersion, int vehicleFirmwareType, int vehicleType)
{
    if (_ignoreVehicleIds.contains(vehicleId) || getVehicleById(vehicleId)
            || vehicleId == 0) {
        return;
    }

    qCDebug(MultiVehicleManagerLog()) << "Adding new vehicle link:vehicleId:vehicleMavlinkVersion:vehicleFirmwareType:vehicleType "
                                      << link->getName()
                                      << vehicleId
                                      << vehicleMavlinkVersion
                                      << vehicleFirmwareType
                                      << vehicleType;

    if (vehicleId == _mavlinkProtocol->getSystemId()) {
        _app->showMessage(QString("Warning: A vehicle is using the same system id as QGroundControl: %1").arg(vehicleId));
    }

    QSettings settings;
    bool mavlinkVersionCheck = settings.value("VERSION_CHECK_ENABLED", true).toBool();
    if (mavlinkVersionCheck && vehicleMavlinkVersion != MAVLINK_VERSION) {
        _ignoreVehicleIds += vehicleId;
        _app->showMessage(QString("The MAVLink protocol version on vehicle #%1 and QGroundControl differ! "
                                  "It is unsafe to use different MAVLink versions. "
                                  "QGroundControl therefore refuses to connect to vehicle #%1, which sends MAVLink version %2 (QGroundControl uses version %3).").arg(vehicleId).arg(vehicleMavlinkVersion).arg(MAVLINK_VERSION));
        return;
    }

    Vehicle* vehicle = new Vehicle(link, vehicleId, (MAV_AUTOPILOT)vehicleFirmwareType, (MAV_TYPE)vehicleType, _firmwarePluginManager, _autopilotPluginManager, _joystickManager);
    connect(vehicle, &Vehicle::allLinksInactive, this, &MultiVehicleManager::_deleteVehiclePhase1);
    connect(vehicle->autopilotPlugin(), &AutoPilotPlugin::parametersReadyChanged, this, &MultiVehicleManager::_autopilotParametersReadyChanged);

    _vehicles.append(vehicle);

    emit vehicleAdded(vehicle);

    setActiveVehicle(vehicle);

    // Mark link as active
    link->setActive(true);

#ifdef __mobile__
    if(_vehicles.count() == 1) {
        //-- Once a vehicle is connected, keep screen from going off
        qCDebug(MultiVehicleManagerLog) << "QAndroidJniObject::keepScreenOn";
        MobileScreenMgr::setKeepScreenOn(true);
    }
#endif

}

/// This slot is connected to the Vehicle::allLinksDestroyed signal such that the Vehicle is deleted
/// and all other right things happen when the Vehicle goes away.
void MultiVehicleManager::_deleteVehiclePhase1(Vehicle* vehicle)
{
    qCDebug(MultiVehicleManagerLog) << "_deleteVehiclePhase1" << vehicle;

    _vehiclesBeingDeleted << vehicle;

    // Remove from map
    bool found = false;
    for (int i=0; i<_vehicles.count(); i++) {
        if (_vehicles[i] == vehicle) {
            _vehicles.removeAt(i);
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning() << "Vehicle not found in map!";
    }

    vehicle->setActive(false);
    vehicle->uas()->shutdownVehicle();

    // First we must signal that a vehicle is no longer available.
    _activeVehicleAvailable = false;
    _parameterReadyVehicleAvailable = false;
    emit activeVehicleAvailableChanged(false);
    emit parameterReadyVehicleAvailableChanged(false);
    emit vehicleRemoved(vehicle);

#ifdef __mobile__
    if(_vehicles.count() == 0) {
        //-- Once no vehicles are connected, we no longer need to keep screen from going off
        qCDebug(MultiVehicleManagerLog) << "QAndroidJniObject::restoreScreenOn";
        MobileScreenMgr::setKeepScreenOn(false);
    }
#endif

    // We must let the above signals flow through the system as well as get back to the main loop event queue
    // before we can actually delete the Vehicle. The reason is that Qml may be holding on the references to it.
    // Even though the above signals should unload any Qml which has references, that Qml will not be destroyed
    // until we get back to the main loop. So we set a short timer which will then fire after Qt has finished
    // doing all of its internal nastiness to clean up the Qml. This works for both the normal running case
    // as well as the unit testing case whichof course has a different signal flow!
    QTimer::singleShot(20, this, &MultiVehicleManager::_deleteVehiclePhase2);
}

void MultiVehicleManager::_deleteVehiclePhase2(void)
{
    qCDebug(MultiVehicleManagerLog) << "_deleteVehiclePhase2" << _vehiclesBeingDeleted[0];

    /// Qml has been notified of vehicle about to go away and should be disconnected from it by now.
    /// This means we can now clear the active vehicle property and delete the Vehicle for real.

    Vehicle* newActiveVehicle = NULL;
    if (_vehicles.count()) {
        newActiveVehicle = qobject_cast<Vehicle*>(_vehicles[0]);
    }

    _activeVehicle = newActiveVehicle;
    emit activeVehicleChanged(newActiveVehicle);

    if (_activeVehicle) {
        _activeVehicle->setActive(true);
        emit activeVehicleAvailableChanged(true);
        if (_activeVehicle->autopilotPlugin()->parametersReady()) {
            emit parameterReadyVehicleAvailableChanged(true);
        }
    }

    delete _vehiclesBeingDeleted[0];
    _vehiclesBeingDeleted.removeAt(0);
}

void MultiVehicleManager::setActiveVehicle(Vehicle* vehicle)
{
    qCDebug(MultiVehicleManagerLog) << "setActiveVehicle" << vehicle;

    if (vehicle != _activeVehicle) {
        if (_activeVehicle) {
            _activeVehicle->setActive(false);

            // The sequence of signals is very important in order to not leave Qml elements connected
            // to a non-existent vehicle.

            // First we must signal that there is no active vehicle available. This will disconnect
            // any existing ui from the currently active vehicle.
            _activeVehicleAvailable = false;
            _parameterReadyVehicleAvailable = false;
            emit activeVehicleAvailableChanged(false);
            emit parameterReadyVehicleAvailableChanged(false);
        }

        // See explanation in _deleteVehiclePhase1
        _vehicleBeingSetActive = vehicle;
        QTimer::singleShot(20, this, &MultiVehicleManager::_setActiveVehiclePhase2);
    }
}

void MultiVehicleManager::_setActiveVehiclePhase2(void)
{
    qCDebug(MultiVehicleManagerLog) << "_setActiveVehiclePhase2 _vehicleBeingSetActive" << _vehicleBeingSetActive;

    // Now we signal the new active vehicle
    _activeVehicle = _vehicleBeingSetActive;
    emit activeVehicleChanged(_activeVehicle);

    // And finally vehicle availability
    if (_activeVehicle) {
        _activeVehicle->setActive(true);
        _activeVehicleAvailable = true;
        emit activeVehicleAvailableChanged(true);

        if (_activeVehicle->autopilotPlugin()->parametersReady()) {
            _parameterReadyVehicleAvailable = true;
            emit parameterReadyVehicleAvailableChanged(true);
        }
    }
}

void MultiVehicleManager::_autopilotParametersReadyChanged(bool parametersReady)
{
    AutoPilotPlugin* autopilot = dynamic_cast<AutoPilotPlugin*>(sender());

    if (!autopilot) {
        qWarning() << "Dynamic cast failed!";
        return;
    }

    if (autopilot->vehicle() == _activeVehicle) {
        _parameterReadyVehicleAvailable = parametersReady;
        emit parameterReadyVehicleAvailableChanged(parametersReady);
    }
}

void MultiVehicleManager::saveSetting(const QString &name, const QString& value)
{
    QSettings settings;
    settings.setValue(name, value);
}

QString MultiVehicleManager::loadSetting(const QString &name, const QString& defaultValue)
{
    QSettings settings;
    return settings.value(name, defaultValue).toString();
}

Vehicle* MultiVehicleManager::getVehicleById(int vehicleId)
{
    for (int i=0; i< _vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(_vehicles[i]);
        if (vehicle->id() == vehicleId) {
            return vehicle;
        }
    }

    return NULL;
}

void MultiVehicleManager::setGcsHeartbeatEnabled(bool gcsHeartBeatEnabled)
{
    if (gcsHeartBeatEnabled != _gcsHeartbeatEnabled) {
        _gcsHeartbeatEnabled = gcsHeartBeatEnabled;
        emit gcsHeartBeatEnabledChanged(gcsHeartBeatEnabled);

        QSettings settings;
        settings.setValue(_gcsHeartbeatEnabledKey, gcsHeartBeatEnabled);

        if (gcsHeartBeatEnabled) {
            _gcsHeartbeatTimer.start();
        } else {
            _gcsHeartbeatTimer.stop();
        }
    }
}

void MultiVehicleManager::_sendGCSHeartbeat(void)
{
    for (int i=0; i< _vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(_vehicles[i]);

        mavlink_message_t message;
        mavlink_msg_heartbeat_pack(_mavlinkProtocol->getSystemId(),
                                   _mavlinkProtocol->getComponentId(),
                                   &message,
                                   MAV_TYPE_GCS,                // MAV_TYPE
                                   MAV_AUTOPILOT_INVALID,   // MAV_AUTOPILOT
                                   MAV_MODE_MANUAL_ARMED,   // MAV_MODE
                                   0,                       // custom mode
                                   MAV_STATE_ACTIVE);       // MAV_STATE
        vehicle->sendMessage(message);
    }
}

bool MultiVehicleManager::linkInUse(LinkInterface* link, Vehicle* skipVehicle)
{
    for (int i=0; i< _vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(_vehicles[i]);

        if (vehicle != skipVehicle) {
            if (vehicle->containsLink(link)) {
                return true;
            }
        }
    }

    return false;
}
