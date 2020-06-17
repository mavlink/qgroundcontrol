/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef PX4FirmwarePluginFactory_H
#define PX4FirmwarePluginFactory_H

#include "FirmwarePlugin.h"

class PX4FirmwarePlugin;

class PX4FirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    PX4FirmwarePluginFactory(void);

    QList<MAV_AUTOPILOT>    supportedFirmwareTypes      (void) const final;
    FirmwarePlugin*         firmwarePluginForAutopilot  (MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;

private:
    PX4FirmwarePlugin*  _pluginInstance;
};

#endif
