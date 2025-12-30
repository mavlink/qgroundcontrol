/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FoxFourFirmwarePlugin.h"
#include "FoxFourAutoPilotPlugin.h"
#include "Vehicle.h"
#include "Camera/FoxFourCameraControl.h"
AutoPilotPlugin* FoxFourFirmwarePlugin::autopilotPlugin(Vehicle *vehicle) const
{
    return new FoxFourAutoPilotPlugin(vehicle, vehicle);
}

MavlinkCameraControl *FoxFourFirmwarePlugin::createCameraControl(const mavlink_camera_information_t *info, Vehicle *vehicle, int compID, QObject *parent) const
{
    return new FoxFourCameraControl(info,vehicle,compID,parent);
}
