#pragma once

#include "UnitTest.h"

class GeoJsonHelperTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _loadShapes_test_data();
    void _loadShapes_test();
    void _loadUnusableShapes_test_data();
    void _loadUnusableShapes_test();
    void _loadUnreadableFile_test_data();
    void _loadUnreadableFile_test();
    void _loadSaveGeoJsonCoordinate_test();
    void _loadSaveGeoJsonCoordinateWithAltitude_test();
};
