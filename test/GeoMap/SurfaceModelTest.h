#pragma once

#include "UnitTest.h"

class SurfaceModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _noViewportNoPatches();
    void _unpositionedCameraNoPatches();
    void _coarseWhenFar();
    void _refinesWhenNear();
    void _patchCountBounded();
    void _budgetExhaustedKeepsCoverage();
    void _heightsArrive();
    void _diffOnMove();
    void _noChurnOnIdenticalUpdate();
    void _cullsInvisibleRegion();
    void _failedHeightsDegradeToFlat();
    void _failedHeightsRetryAndRecover();
    void _keepsReadyPatchesDuringLodChurn();
    void _degradedPatchesKeepRetiringCover();
    void _degradedRetiringPatchesSwept();
    void _pendingPatchesCoveredDuringLodChurn();
    void _tallTerrainKeepsCameraTileResident();
    void _cameraGroundTileResidentOverFlatTerrain();
    void _slowHeightsChurnSettlesWithoutHoles();
};
