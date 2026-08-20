#pragma once

#include "UnitTest.h"

class ElevationTilePyramidTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _emptyPyramidHasNoView();
    void _insertRejectsInvalidGrid();
    void _rejectsInvalidKey();
    void _selfTileWins();
    void _nearestAncestorWins();
    void _noDescendantFallback();
    void _subWindowMath();
    void _subWindowDeepZoom();
    void _insertReplacesTile();
    void _lruEvictionAtCap();
    void _pinnedTilesSurviveEviction();
    void _allPinnedGrowsPastCap();
    void _descendantTracking();
};
