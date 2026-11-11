#pragma once

#include "UnitTest.h"

class GeoSceneTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _originAnchorsOnCameraSet();
    void _reanchorsEuclidean();
    void _cameraSwapAnchorsFresh();
    void _verticalScaleTracksOrigin();
    void _scenePositionFor();
    void _screenPositionFor();
    void _centerElevationInMeters();
    void _solveRoundTrip();
};
