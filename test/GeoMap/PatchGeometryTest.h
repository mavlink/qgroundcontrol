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
};
