/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4FirmwarePluginFactory.h"
#include "PX4/PX4FirmwarePlugin.h"

PX4FirmwarePluginFactory PX4FirmwarePluginFactory;

PX4FirmwarePluginFactory::PX4FirmwarePluginFactory(void)
    : _pluginInstance(nullptr)
{

}

QList<QGCMAVLink::FirmwareClass_t> PX4FirmwarePluginFactory::supportedFirmwareClasses(void) const
{
    QList<QGCMAVLink::FirmwareClass_t> list;
    list.append(QGCMAVLink::FirmwareClassPX4);
    return list;
}

FirmwarePlugin* PX4FirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE /*vehicleType*/)
{
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        if (!_pluginInstance) {
            _pluginInstance = new PX4FirmwarePlugin();
        }
        return _pluginInstance;
    }
    return nullptr;
}
