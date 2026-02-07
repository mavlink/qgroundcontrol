#pragma once

#include "FirmwarePluginFactory.h"
#include "QGCMAVLink.h"


class ArduCopterFirmwarePlugin;
class ArduPlaneFirmwarePlugin;
class ArduRoverFirmwarePlugin;
class ArduSubFirmwarePlugin;


class APMFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    explicit APMFirmwarePluginFactory(QObject *parent = nullptr);
    ~APMFirmwarePluginFactory();

    QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses() const override;
    FirmwarePlugin *firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) override;

private:
    ArduCopterFirmwarePlugin *_arduCopterPluginInstance = nullptr;
    ArduPlaneFirmwarePlugin *_arduPlanePluginInstance = nullptr;
    ArduRoverFirmwarePlugin *_arduRoverPluginInstance = nullptr;
    ArduSubFirmwarePlugin *_arduSubPluginInstance = nullptr;
};
