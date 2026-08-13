#pragma once

#include "UnitTest.h"

class SurfaceModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _patchRendersEstimateImmediately();
    void _patchesAreViewsOfField();
    void _reMeshOnDataArrival();
    void _reMeshScopeGate();
    void _requestsTileCoverage();
    void _noViewportNoPatches();
    void _unpositionedCameraNoPatches();
    void _coarseWhenFar();
    void _refinesWhenNear();
    void _patchCountBounded();
    void _budgetExhaustedKeepsCoverage();
    void _diffOnMove();
    void _noChurnOnIdenticalUpdate();
    void _cullsInvisibleRegion();
    void _coverageMaintainedDuringLodChurn();
    void _coverageSweepAcrossPoses();
    void _coverageAfterInteractiveGesture();
    void _renderedEdgeContractAcrossLodBoundaries();
    void _tallTerrainRecullsOnDataArrival();
    void _cameraGroundTileResidentOverFlatTerrain();
    void _edgeLodDeltasMatchResidentNeighbors();
    void _edgeDeltasNotifiedOnNeighborChurn();
    void _pinsPatchBackingTiles();
};
