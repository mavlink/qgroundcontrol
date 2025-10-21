/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleProfileSettingsArray.h"

#include "VehicleProfileSettings.h"

VehicleProfileSettingsArray::VehicleProfileSettingsArray(QObject* parent)
    : SettingsGroupArray(VehicleProfileSettings::name, VehicleProfileSettings::settingsGroup, parent)
{
    Q_UNUSED(parent);
}

SettingsGroup* VehicleProfileSettingsArray::_newGroupInstance(int indexKey)
{
    Q_UNUSED(indexKey);
    return new VehicleProfileSettings(this);
}
