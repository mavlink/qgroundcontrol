#pragma once

#include "UnitTest.h"

class SurfacePatchModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _emptyWithoutCamera();
    void _populatesFromCamera();
    void _rolesValid();
    void _incrementalUpdatesOnMove();
    void _reanchorsOnLargeMove();
    void _cameraSwapAnchorsFresh();
    void _debugHillsSwitchResets();
    void _rowsAlwaysMeshedDuringLodChurn();
    void _edgeLodDeltasRoleStitchesLodRings();
    void _tileKeyAndHeightFieldExposedToDelegates();
    void _terrainHeightAt();
};
