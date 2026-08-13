#pragma once

#include "UnitTest.h"

class TileMathTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _geoWorldOrigin();
    void _geoWorldRoundTrip();
    void _latitudeClamp();
    void _metersPerPixel();
    void _tileZoom0();
    void _tileZoom1Quadrants();
    void _tileWorldRoundTrip();
    void _tileEdgeClamp();
    void _zoomForMetersPerPixel();
    void _mercatorScale();
    void _isValidKey();
};
