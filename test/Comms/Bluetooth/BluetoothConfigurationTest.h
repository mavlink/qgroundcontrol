#pragma once

#include "UnitTest.h"

class BluetoothConfigurationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testConstruction();
    void _testCopyConstruction();
    void _testFactoryCreateAndDuplicate();
    void _testModeGetSet();
    void _testBleUuidGetSet();
    void _testBluetoothAvailabilityConsistency();
    void _testSettingsRoundtrip();
    void _testSelectAdapterInvalidAddress();
    void _testBleAddressRotationUpdatesSelectedDevice();
    void _testClassicDiscoveryWithZeroRssiIsListed();
    void _testBleDiscoveryWithZeroRssiIsListed();
    void _testBleDiscoveryWithUnknownCoreConfigIsListed();
    void _testAdapterDiscoverablePropertyMeta();
    void _testDestroyDuringAsyncRefresh();
};
