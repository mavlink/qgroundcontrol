#pragma once

#include "UnitTest.h"

/// The base station of the corrections on the Fly view's two maps, loaded outside a map: its marker, the line to the
/// vehicle and the vehicle's RTK baseline on the line.
class GPSBaseStationMapTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _mapShowsBaseStation();
    void _geoMapShowsBaseStation();
    void _noLineWithoutVehicle_data();
    void _noLineWithoutVehicle();
};
