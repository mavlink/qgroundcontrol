#pragma once

#include "UnitTest.h"

class VehicleGPSAggregateFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialValues();
    void testDualGpsMerge();
    void testSingleGps();
    void testAuthPriority();
    void testStaleTimeout();
    void testRebind();
};
