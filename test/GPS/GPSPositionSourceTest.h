#pragma once

#include "UnitTest.h"

class GPSPositionSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testStartStop();
    void testUpdateFromSensorGps_noFix();
    void testUpdateFromSensorGps_2dFix();
    void testUpdateFromSensorGps_3dFix();
    void testUpdateFromSensorGps_attributes();
    void testUpdateWhileStopped();
    void testRequestUpdate();
};
