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
    AutoPilotPlugin*    autopilotPlugin                     (Vehicle* vehicle) override;
    QGCCameraManager*   createCameraManager                 (Vehicle *vehicle) override;
    QGCCameraControl*   createCameraControl                 (const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr) override;
    const QVariantList& toolBarIndicators                   (const Vehicle* vehicle) override;
private:
    QVariantList _toolBarIndicatorList;
};
