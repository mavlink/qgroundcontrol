#pragma once

#include "UnitTest.h"

class GeoJsonHelperTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _determineShapeType_test_data();
    void _determineShapeType_test();
    void _loadShape_test_data();
    void _loadShape_test();
    void _loadFailure_test_data();
    void _loadFailure_test();
    void _loadSaveGeoJsonCoordinate_test();
    void _loadSaveGeoJsonCoordinateWithAltitude_test();
};
