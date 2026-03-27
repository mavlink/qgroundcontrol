#pragma once
#include "UnitTest.h"

class GPSSatelliteInfoSourceTest : public UnitTest
{
    Q_OBJECT
private slots:
    void testInitialState();
    void testStartStop();
    void testUpdateEmitsSignals();
    void testSvidMapping();
};
