#pragma once

#include "UnitTest.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

protected slots:
    void cleanup() override;

private slots:
    void testInitialState();
    void testMultiDeviceTracking();
    void testDisconnectAll();
    void testDuplicateConnect();
    void testPrimaryDevice();
    void testConnectGPSRegistersDevice();
    void testDisconnectGPSRemovesDevice();
    void testDisconnectAllMultiple();
    void testGpsRtkFactGroupReturnsStaticDefault();
    void testLogEvent();
};
