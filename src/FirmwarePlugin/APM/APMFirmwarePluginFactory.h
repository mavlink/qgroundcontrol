/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FirmwarePluginFactory.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>

class ArduCopterFirmwarePlugin;
class ArduPlaneFirmwarePlugin;
class ArduRoverFirmwarePlugin;
class ArduSubFirmwarePlugin;

Q_DECLARE_LOGGING_CATEGORY(FirmwarePluginFactoryLog)

class APMFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    explicit APMFirmwarePluginFactory(QObject *parent = nullptr);
    ~APMFirmwarePluginFactory();

    QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses() const final;
    FirmwarePlugin *firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;

private:
    ArduCopterFirmwarePlugin *_arduCopterPluginInstance = nullptr;
    ArduPlaneFirmwarePlugin *_arduPlanePluginInstance = nullptr;
    ArduRoverFirmwarePlugin *_arduRoverPluginInstance = nullptr;
    ArduSubFirmwarePlugin *_arduSubPluginInstance = nullptr;
};
