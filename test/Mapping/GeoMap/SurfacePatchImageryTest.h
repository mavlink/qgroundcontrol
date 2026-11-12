#pragma once

#include "UnitTest.h"

class SurfacePatchImageryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _imagesArriveForAllPatches();
    void _emptyMapTypeDisablesImagery();
    void _mapTypeSwitchRefetches();
    void _sourceChangeRefetches();
    void _fallbackCoversLodChanges();
    void _retiredImagesFollowMemoryCacheSetting_data();
    void _retiredImagesFollowMemoryCacheSetting();
    void _imagesRecoverAfterGestureChurn();
    void _imagesRecoverAfterTransientOutage();
    void _missingImagesNotRequestedAgain();
};
