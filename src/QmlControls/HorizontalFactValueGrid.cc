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
const QString HorizontalFactValueGrid::telemetryBarSettingsGroup("TelemetryBarUserSettings");

// for multi-vehicle list telem tables
const QString HorizontalFactValueGrid::vehicleCardSettingsGroup("VehicleCardUserSettings");

HorizontalFactValueGrid::HorizontalFactValueGrid(QQuickItem* parent)
    : FactValueGrid(parent)
{

}
