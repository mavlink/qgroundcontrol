/*!
 * @file
 *   @brief Yuneec Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "FirmwarePlugin.h"
#include "PX4FirmwarePlugin.h"

class YuneecCameraManager;

class YuneecFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT

public:
    YuneecFirmwarePlugin();

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
    QGCCameraManager*   createCameraManager                 (Vehicle *vehicle) override final;
#if !defined (__planner__)
    QGCCameraControl*   createCameraControl                 (const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = NULL) override final;
#endif

private:
    QVariantList            _toolBarIndicators;

    static QVariantList _cameraList;        ///< Yuneec camera list
};
