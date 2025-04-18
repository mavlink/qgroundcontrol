/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "HorizontalFactValueGrid.h"

// for activeVehicle telem table
const QString HorizontalFactValueGrid::telemetryBarUserSettingsGroup    ("TelemetryBarUserSettings");
const QString HorizontalFactValueGrid::telemetryBarDefaultSettingsGroup ("TelemetryBarDefaultSettings");

// for multi-vehicle list telem tables
const QString HorizontalFactValueGrid::vehicleCardUserSettingsGroup    ("VehicleCardUserSettings");
const QString HorizontalFactValueGrid::vehicleCardDefaultSettingsGroup ("VehicleCardDefaultSettings");

HorizontalFactValueGrid::HorizontalFactValueGrid(QQuickItem* parent)
    : FactValueGrid(parent)
{

}

HorizontalFactValueGrid::HorizontalFactValueGrid(const QString& defaultSettingsGroup)
    : FactValueGrid(defaultSettingsGroup)
{

}
