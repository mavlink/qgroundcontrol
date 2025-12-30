/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "ArduCopterFirmwarePlugin.h"
class AutoPilotPlugin;
class Vehicle;

class FoxFourFirmwarePlugin : public ArduCopterFirmwarePlugin
{
    Q_OBJECT
public:
    // FirmwarePlugin overrides

    AutoPilotPlugin *autopilotPlugin(Vehicle *vehicle) const final;
    MavlinkCameraControl *createCameraControl(const mavlink_camera_information_t *info, Vehicle *vehicle, int compID, QObject *parent) const;

};
