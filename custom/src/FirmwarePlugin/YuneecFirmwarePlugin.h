/*!
 * @file
 *   @brief Yuneec Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "FirmwarePlugin.h"
#include "PX4FirmwarePlugin.h"

class YuneecFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT

public:
    YuneecFirmwarePlugin(void);

    // FirmwarePlugin overrides

    AutoPilotPlugin*    autopilotPlugin                     (Vehicle* vehicle) final;
    QString             vehicleImageOpaque                  (const Vehicle* vehicle) const final;
    QString             vehicleImageOutline                 (const Vehicle* vehicle) const final;
    QString             vehicleImageCompass                 (const Vehicle* vehicle) const final;
    const QVariantList& toolBarIndicators                   (const Vehicle* vehicle) final;
    const QVariantList& cameraList                          (const Vehicle* vehicle) final;
    bool                isGuidedMode                        (const Vehicle* vehicle) const final;
    bool                hasGimbal                           (Vehicle* vehicle, bool& rollSupported, bool& pitchSupported, bool& yawSupported) final;
    void                batteryConsumptionData              (Vehicle* vehicle, int& mAhBattery, double& hoverAmps, double& cruiseAmps) const final;
    bool                vehicleYawsToNextWaypointInMission  (const Vehicle* vehicle) const final;
    QString             internalParameterMetaDataFile       (Vehicle* vehicle) override { Q_UNUSED(vehicle); return QString(":/typhoonh/YuneecParameterFactMetaData.xml"); }

private:
    QVariantList        _toolBarIndicators;

    static QVariantList _cameraList;        ///< Yuneec camera list
};
