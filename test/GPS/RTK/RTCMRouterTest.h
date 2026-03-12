#pragma once

#include "UnitTest.h"

class RTCMRouterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testRouteToVehicles();
    void testRouteToBaseStation();
    void testRouteAll();
    void testNullSafety();
};
