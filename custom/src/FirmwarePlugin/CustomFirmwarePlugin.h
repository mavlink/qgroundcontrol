/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom Firmware Plugin (PX4)
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include "ArduCopterFirmwarePlugin.h"

class AutoPilotPlugin;
class CustomCameraManager;
class Vehicle;

class CustomFirmwarePlugin : public ArduCopterFirmwarePlugin
{
    Q_OBJECT
public:
    CustomFirmwarePlugin();

    // FirmwarePlugin overrides
    AutoPilotPlugin*    autopilotPlugin (Vehicle* vehicle) const final;
    const QVariantList& toolIndicators  (const Vehicle* vehicle) final;
    bool                hasGimbal       (Vehicle* vehicle, bool& rollSupported, bool& pitchSupported, bool& yawSupported) const final;
    void                updateAvailableFlightModes      (FlightModeList &modeList) override;

    QList<MAV_CMD> supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass) const override;
    QString missionCommandOverrides(QGCMAVLink::VehicleClass_t vehicleClass) const override;

private:
    QVariantList _toolIndicatorList;
};
