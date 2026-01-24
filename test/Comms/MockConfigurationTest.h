#pragma once

#include "TestFixtures.h"

/// Unit tests for MockConfiguration.
/// Tests configuration properties, failure modes, settings persistence, and signals.
class MockConfigurationTest : public OfflineTest
{
    Q_OBJECT

public:
    MockConfigurationTest() = default;

private slots:
    // Construction tests
    void _testDefaultConstruction();
    void _testNamedConstruction();
    void _testCopyConstruction();

    // Type and URL tests
    void _testLinkType();
    void _testSettingsURL();
    void _testSettingsTitle();

    // Default values tests
    void _testDefaultFirmwareType();
    void _testDefaultVehicleType();
    void _testDefaultSendStatusText();
    void _testDefaultIncrementVehicleId();
    void _testDefaultFailureMode();
    void _testDefaultBoardIds();

    // Firmware property tests
    void _testSetFirmwareType();
    void _testSetFirmwareInt();
    void _testFirmwareChangedSignal();

    // Vehicle type property tests
    void _testSetVehicleType();
    void _testSetVehicleInt();
    void _testVehicleChangedSignal();

    // SendStatusText property tests
    void _testSetSendStatusText();
    void _testSendStatusChangedSignal();

    // IncrementVehicleId property tests
    void _testSetIncrementVehicleId();
    void _testIncrementVehicleIdChangedSignal();

    // Board vendor/product IDs tests
    void _testSetBoardVendorProduct();

    // FailureMode_t enum tests
    void _testFailureModeEnumValues();
    void _testSetFailureMode();

    // CopyFrom tests
    void _testCopyFrom();
    void _testCopyFromPreservesAllProperties();

    // Settings persistence tests
    void _testSaveSettings();
    void _testLoadSettings();
    void _testSettingsRoundTrip();
};
