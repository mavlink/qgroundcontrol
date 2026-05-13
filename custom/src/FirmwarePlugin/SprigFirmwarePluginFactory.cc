#include "SprigFirmwarePluginFactory.h"
#include "SprigFirmwarePlugin.h"

SprigFirmwarePluginFactory SprigFirmwarePluginFactoryImp;

SprigFirmwarePluginFactory::SprigFirmwarePluginFactory()
    : _pluginInstance(nullptr)
{

}

QList<QGCMAVLink::FirmwareClass_t> SprigFirmwarePluginFactory::supportedFirmwareClasses() const
{
    QList<QGCMAVLink::FirmwareClass_t> firmwareClasses;
    firmwareClasses.append(QGCMAVLink::FirmwareClassPX4);
    return firmwareClasses;
}

QList<QGCMAVLink::VehicleClass_t> SprigFirmwarePluginFactory::supportedVehicleClasses() const
{
    QList<QGCMAVLink::VehicleClass_t> vehicleClasses;
    vehicleClasses.append(QGCMAVLink::VehicleClassMultiRotor);
    return vehicleClasses;
}

FirmwarePlugin *SprigFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE /*vehicleType*/)
{
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        if (!_pluginInstance) {
            _pluginInstance = new SprigFirmwarePlugin;
        }
        return _pluginInstance;
    }

    return nullptr;
}
