#pragma once

#include "TerrainTest.h"

class TerrainHeightSourceTest : public TerrainTest
{
    Q_OBJECT

private slots:
    void _deliversFlatRegionHeights();
    void _deliversSlopeRegionHeights();
    void _interpolatesAcrossCellSteps();
    void _blendBandScalesHeights();
    void _coarseZoomDeliversFlatZeros();
    void _cancelPreventsDelivery();
    void _invalidGridSizeFails();
};
