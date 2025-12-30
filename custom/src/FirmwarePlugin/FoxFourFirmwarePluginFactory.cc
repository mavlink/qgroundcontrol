/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FoxFourFirmwarePluginFactory.h"
#include "FoxFourFirmwarePlugin.h"
#include "PX4FirmwarePlugin.h"
FoxFourFirmwarePluginFactory FoxFourFirmwarePluginFactoryImp;

FoxFourFirmwarePluginFactory::FoxFourFirmwarePluginFactory(){

}

QList<QGCMAVLink::FirmwareClass_t> FoxFourFirmwarePluginFactory::supportedFirmwareClasses() const
{
    QList<QGCMAVLink::FirmwareClass_t> firmwareClasses;
    firmwareClasses.append(QGCMAVLink::FirmwareClassPX4);
    firmwareClasses.append(QGCMAVLink::FirmwareClassArduPilot);
    return firmwareClasses;
}

QList<QGCMAVLink::VehicleClass_t> FoxFourFirmwarePluginFactory::supportedVehicleClasses() const
{
    QList<QGCMAVLink::VehicleClass_t> vehicleClasses;
    // vehicleClasses.append(QGCMAVLink::VehicleClassMultiRotor);
    vehicleClasses=FirmwarePluginFactory::supportedVehicleClasses();
    return vehicleClasses;
}

FirmwarePlugin *FoxFourFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    // For now F4 only supported ArduPilot

    if (autopilotType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        if (!_ardupilotPluginInstance) {
            _ardupilotPluginInstance = new FoxFourFirmwarePlugin;
        }
        return _ardupilotPluginInstance;
    } else if ( autopilotType == MAV_AUTOPILOT_PX4 ){
        if(!_px4PluginInstance) {
            _px4PluginInstance = new PX4FirmwarePlugin;
        }
        return _px4PluginInstance;
    }
    return nullptr;
}
