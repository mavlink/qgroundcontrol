#pragma once

#include "TempDirectoryTest.h"

class GeoJsonHelperTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _determineShapeTypePolygon_test();
    void _determineShapeTypePolyline_test();
    void _determineShapeTypeNoShapes_test();
    void _determineShapeTypeMissingFile_test();
    void _determineShapeTypeUnsupportedGeometry_test();
    void _determineShapeTypeInvalidJson_test();
    void _loadPolygonFromFile_test();
    void _loadPolylineFromFile_test();
    void _loadPolygonFromNestedGeometryCollection_test();
    void _loadPolylineFromNestedGeometryCollection_test();
    void _loadPolygonFromPolylineFails_test();
    void _loadPolylineFromPolygonFails_test();
    void _loadSaveGeoJsonCoordinate_test();
    void _loadSaveGeoJsonCoordinateWithAltitude_test();
};
