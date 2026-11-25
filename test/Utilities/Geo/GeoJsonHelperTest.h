#pragma once

#include "UnitTest.h"

class GeoJsonHelperTest : public UnitTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_determineShapeType_test);
    UT_PARAMETERIZED_TEST(_loadShape_test);
    UT_PARAMETERIZED_TEST(_loadFailure_test);
    void _loadSaveGeoJsonCoordinate_test();
    void _loadSaveGeoJsonCoordinateWithAltitude_test();
};
