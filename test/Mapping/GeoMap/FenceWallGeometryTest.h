#pragma once

#include "UnitTest.h"

class FenceWallGeometryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _emptyAndDisabledStates();
    void _wallLayout();
    void _ringNormalization();
    void _subdivisionCap();
    void _deferredCoalescedRebuild();
};
