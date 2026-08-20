#pragma once

#include "UnitTest.h"

class PatchGeometryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _flatMeshLayout();
    void _cornerPositionsAndUVs();
    void _heightDisplacement();
    void _mismatchedHeightsRenderFlat();
    void _skirtVerticesBelowSurface();
    void _flatNormalsPointUp();
    void _boundsIncludeSkirt();
    void _gridSizeClamped();
    void _sampleFromFieldDisplacesVertices();
    void _sampleFromFieldWithoutFieldWarns();
    void _adjacentPatchesShareEdgeHeights();
    void _resampleAfterFieldGainsData();
    void _stitchedEdgeLiesOnCoarseSegments();
    void _stitchedNorthEdgeLiesOnCoarseRenderedRow();
    void _stitchAppliesToAllFourEdges();
    void _stitchInvalidDeltaWarns();
    void _declarativeHeightsStitchLikeSampleFromField();
    void _gridSizeChangeResetsInvalidDeltas();
    void _edgeLodDeltasListProperty();
    void _replacingLiveFieldDisconnectsOld();
    void _fieldSetterIgnoresSameValue();
    void _skirtDepthScalesWithCoarsestNeighbor();
};
