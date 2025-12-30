/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FirmwarePluginFactory.h"
#include "QGCMAVLink.h"


class FoxFourFirmwarePlugin;
class PX4FirmwarePlugin;
class FirmwarePlugin;

class FoxFourFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    FoxFourFirmwarePluginFactory();
    QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses() const final;
    QList<QGCMAVLink::VehicleClass_t> supportedVehicleClasses() const final;
    FirmwarePlugin* firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;

private:
    FoxFourFirmwarePlugin* _ardupilotPluginInstance = nullptr;
        PX4FirmwarePlugin* _px4PluginInstance = nullptr;
};

extern FoxFourFirmwarePluginFactory CustomFirmwarePluginFactoryImp;
