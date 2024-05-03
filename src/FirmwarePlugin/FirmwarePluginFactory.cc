/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwarePluginFactory.h"
#include "FirmwarePlugin.h"

FirmwarePluginFactory::FirmwarePluginFactory(void)
{
    FirmwarePluginFactoryRegister::instance()->registerPluginFactory(this);
}

QList<QGCMAVLink::VehicleClass_t> FirmwarePluginFactory::supportedVehicleClasses(void) const
{
    return QGCMAVLink::allVehicleClasses();
}

FirmwarePluginFactoryRegister* FirmwarePluginFactoryRegister::instance(void)
{
    static FirmwarePluginFactoryRegister* _instance = nullptr;

    if (!_instance) {
        _instance = new FirmwarePluginFactoryRegister;
    }

    return _instance;
}
