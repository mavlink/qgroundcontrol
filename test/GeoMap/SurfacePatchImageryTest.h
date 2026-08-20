#pragma once

#include "UnitTest.h"

class SurfacePatchImageryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _imagesArriveForAllPatches();
    void _emptyMapTypeDisablesImagery();
    void _mapTypeSwitchRefetches();
    void _fallbackCoversLodChanges();
    void _imagesRecoverAfterGestureChurn();
    void _imagesRecoverAfterTransientOutage();
};
