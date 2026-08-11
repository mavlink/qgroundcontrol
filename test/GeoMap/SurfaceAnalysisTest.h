#pragma once

#include "UnitTest.h"

class SurfaceAnalysisTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _noSeamsOnMatchingEdges();
    void _pendingFlatSeam();
    void _degradedFlatSeam();
    void _coveredPatchIgnored();
    void _belowThresholdIgnored();
    void _sameZoomMismatch();
    void _sameZoomSeamReportedOnce();
    void _lodTJunction();
    void _overlappingAncestorNotASeamNeighbor();
    void _noHolesWhenFullyCovered();
    void _holeNoPatch();
    void _holeSuppressedCovered();
    void _holeCheckSkippedWithoutSamples();
    void _holeElevatedTerrainCulled();
    void _elevatedTerrainRenderedNoHole();
    void _cameraBelowSurface();
    void _cameraAboveSurface();
    void _cameraClipsNearbyTerrain();
    void _noSurfaceUnderCamera();
    void _cameraCheckSkippedWithoutScale();
    void _cameraCheckSkippedWithoutHeight();
    void _nonFiniteHeights();
    void _reportText();
};
