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

#include "PX4FirmwarePlugin.h"

class AutoPilotPlugin;
class CustomCameraManager;
class Vehicle;

class CustomFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT
public:
    CustomFirmwarePlugin();

    // FirmwarePlugin overrides
    AutoPilotPlugin*    autopilotPlugin (Vehicle* vehicle) const final;
    const QVariantList& toolIndicators  (const Vehicle* vehicle) final;
    bool                hasGimbal       (Vehicle* vehicle, bool& rollSupported, bool& pitchSupported, bool& yawSupported) const final;
    void                updateAvailableFlightModes      (FlightModeList &modeList) override;

private:
    QVariantList _toolIndicatorList;
};
