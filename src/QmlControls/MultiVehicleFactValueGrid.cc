/****************************************************************************
 *
 * (c) 2009-2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MultiVehicleFactValueGrid.h"

const QString MultiVehicleFactValueGrid::vehicleCardUserSettingsGroup    ("VehicleCardUserSettings");
const QString MultiVehicleFactValueGrid::vehicleCardDefaultSettingsGroup ("VehicleCardDefaultSettings");

MultiVehicleFactValueGrid::MultiVehicleFactValueGrid(QQuickItem* parent)
    : FactValueGrid(parent)
{

}

MultiVehicleFactValueGrid::MultiVehicleFactValueGrid(const QString& defaultSettingsGroup)
    : FactValueGrid(defaultSettingsGroup)
{

}
