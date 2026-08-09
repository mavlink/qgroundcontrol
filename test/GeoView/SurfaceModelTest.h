#pragma once

#include "UnitTest.h"

class SurfaceModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _noViewportNoPatches();
    void _coarseWhenFar();
    void _refinesWhenNear();
    void _patchCountBounded();
    void _budgetExhaustedKeepsCoverage();
    void _heightsArrive();
    void _diffOnMove();
    void _noChurnOnIdenticalUpdate();
    void _cullsInvisibleRegion();
    void _failedHeightsDegradeToFlat();
};
