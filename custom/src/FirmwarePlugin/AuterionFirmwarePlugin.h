/*!
 * @file
 *   @brief Auterion Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "FirmwarePlugin.h"
#include "PX4FirmwarePlugin.h"

class AuterionCameraManager;

class AuterionFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT
public:
    AuterionFirmwarePlugin();
    // FirmwarePlugin overrides
    AutoPilotPlugin*    autopilotPlugin                     (Vehicle* vehicle) final;
    QGCCameraManager*   createCameraManager                 (Vehicle *vehicle) override final;
    QGCCameraControl*   createCameraControl                 (const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr) override final;
    const QVariantList& toolBarIndicators                   (const Vehicle* vehicle) override final;
private:
    QVariantList _toolBarIndicatorList;
};
