#pragma once

#include "UnitTest.h"

class Viewer3DTerrainGeometry;

class Viewer3DTerrainGeometryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testComputeFaceNormal();
    void _testComputeFaceNormalDegenerate();
    void _testComputeFaceNormalNegativeZ();
    void _testBuildTerrainZeroCounts();
    void _testBuildTerrainValidRegion();
    void _testBuildTerrainVertexCountScaling();
    void _testClearScene();
    void _testPropertySetters();
    void _testRoiSetters();
};
