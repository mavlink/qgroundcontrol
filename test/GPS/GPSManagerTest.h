#pragma once

#include "UnitTest.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testMultiDeviceTracking();
    void testDisconnectAll();
    void testDuplicateConnect();
    void testPrimaryDevice();
};
