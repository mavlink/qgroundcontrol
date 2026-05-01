#pragma once

#include "FirmwarePluginFactory.h"
#include "QGCMAVLink.h"

class CustomFirmwarePlugin;
class FirmwarePlugin;

/// This custom implementation of FirmwarePluginFactory creates a custom build which only supports
/// PX4 Pro firmware running on a multi-rotor vehicle. This is turn removes portions of the QGC UI
/// related to other firmware and vehicle types. This creating a more simplified UI for a specific
/// type of vehicle.
class CustomFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    CustomFirmwarePluginFactory();
    QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses() const final;
    QList<QGCMAVLink::VehicleClass_t> supportedVehicleClasses() const final;
    FirmwarePlugin *firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;

private:
    CustomFirmwarePlugin *_pluginInstance = nullptr;
};

extern CustomFirmwarePluginFactory CustomFirmwarePluginFactoryImp;
