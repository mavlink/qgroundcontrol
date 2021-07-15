/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FMTFirmwarePluginFactory.h"
#include "FMT/FMTFirmwarePlugin.h"
FMTFirmwarePluginFactory FMTFirmwarePluginFactory;

FMTFirmwarePluginFactory::FMTFirmwarePluginFactory(void)
    : _pluginInstance(NULL)
{

}

QList<MAV_AUTOPILOT> FMTFirmwarePluginFactory::supportedFirmwareTypes(void) const
{
    QList<MAV_AUTOPILOT> list;

    list.append(MAV_AUTOPILOT_FMT);
    return list;
}

FirmwarePlugin* FMTFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType);

    if (autopilotType == MAV_AUTOPILOT_FMT) {
        if (!_pluginInstance) {
            _pluginInstance = new FMTFirmwarePlugin;
        }
        return _pluginInstance;
    }

    return NULL;
}
