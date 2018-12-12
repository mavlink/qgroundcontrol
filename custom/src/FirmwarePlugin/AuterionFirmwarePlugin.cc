/*!
 * @file
 *   @brief Auterion Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "AuterionFirmwarePlugin.h"
#include "AuterionAutoPilotPlugin.h"
#include "AuterionCameraManager.h"
#include "AuterionCameraControl.h"

//-----------------------------------------------------------------------------
AuterionFirmwarePlugin::AuterionFirmwarePlugin()
{
}

//-----------------------------------------------------------------------------
AutoPilotPlugin* AuterionFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new AuterionAutoPilotPlugin(vehicle, vehicle);
}

//-----------------------------------------------------------------------------
QGCCameraManager*
AuterionFirmwarePlugin::createCameraManager(Vehicle *vehicle)
{
    return new AuterionCameraManager(vehicle);
}

//-----------------------------------------------------------------------------
QGCCameraControl*
AuterionFirmwarePlugin::createCameraControl(const mavlink_camera_information_t* info, Vehicle *vehicle, int compID, QObject* parent)
{
    return new AuterionCameraControl(info, vehicle, compID, parent);
}
