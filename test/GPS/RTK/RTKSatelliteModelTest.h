#pragma once

#include "UnitTest.h"

class RTKSatelliteModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testEmptyModel();
    void testUpdatePopulatesModel();
    void testClear();
    void testRoleNames();
    void testUsedCount();
    void testGrowModel();
    void testShrinkModel();
    void testSameSizeUpdate();
    void testConstellationSummaryContent();
};
