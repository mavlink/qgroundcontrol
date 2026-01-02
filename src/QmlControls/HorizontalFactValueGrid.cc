#include "HorizontalFactValueGrid.h"

// for activeVehicle telem table
const QString HorizontalFactValueGrid::telemetryBarSettingsGroup("TelemetryBarUserSettings");

// for multi-vehicle list telem tables
const QString HorizontalFactValueGrid::vehicleCardSettingsGroup("VehicleCardUserSettings");

HorizontalFactValueGrid::HorizontalFactValueGrid(QQuickItem* parent)
    : FactValueGrid(parent)
{

}
