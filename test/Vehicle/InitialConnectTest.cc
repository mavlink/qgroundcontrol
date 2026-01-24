#include "InitialConnectTest.h"
#include "TestHelpers.h"
#include "MultiVehicleManager.h"
#include "LinkManager.h"
#include "MockLink.h"
#include "Vehicle.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void InitialConnectTest::cleanup()
{
    _disconnectMockLink();
    LinkManager::instance()->disconnectAll();
    UnitTest::cleanup();
}

// ============================================================================
// Basic Connection Tests
// ============================================================================

void InitialConnectTest::_testPX4Connection()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    VERIFY_NOT_NULL(_vehicle);

    QVERIFY(_vehicle->px4Firmware());
    QVERIFY(!_vehicle->apmFirmware());
    QCOMPARE_EQ(_vehicle->firmwareType(), MAV_AUTOPILOT_PX4);
}

void InitialConnectTest::_testArduPilotConnection()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    VERIFY_NOT_NULL(_vehicle);

    QVERIFY(_vehicle->apmFirmware());
    QVERIFY(!_vehicle->px4Firmware());
    QCOMPARE_EQ(_vehicle->firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
}

// ============================================================================
// Property Verification Tests
// ============================================================================

void InitialConnectTest::_testVehicleId()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    VERIFY_NOT_NULL(_vehicle);

    QVERIFY(_vehicle->id() > 0);
}

void InitialConnectTest::_testBoardVendorProductId()
{
    auto *mvm = MultiVehicleManager::instance();
    TestHelpers::SignalCapture activeVehicleCapture(mvm, &MultiVehicleManager::activeVehicleChanged);

    auto mockConfig = std::make_shared<MockConfiguration>(QString{"MockLink"});
    const uint16_t mockVendor = 1234;
    const uint16_t mockProduct = 5678;
    mockConfig->setBoardVendorProduct(mockVendor, mockProduct);

    SharedLinkConfigurationPtr linkConfig = mockConfig;
    LinkManager::instance()->createConnectedLink(linkConfig);

    QVERIFY(activeVehicleCapture.wait());
    auto *vehicle = mvm->activeVehicle();
    VERIFY_NOT_NULL(vehicle);
    TestHelpers::SignalCapture initialConnectCapture(vehicle, &Vehicle::initialConnectComplete);

    QVERIFY(initialConnectCapture.wait() || vehicle->isInitialConnectComplete());

    QCOMPARE_EQ(vehicle->firmwareBoardVendorId(), mockVendor);
    QCOMPARE_EQ(vehicle->firmwareBoardProductId(), mockProduct);

    LinkManager::instance()->disconnectAll();
}

void InitialConnectTest::_testFirmwareTypeProperties()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    VERIFY_NOT_NULL(_vehicle);

    QVERIFY(!_vehicle->firmwareTypeString().isEmpty());

    const bool isPX4 = _vehicle->px4Firmware();
    const bool isAPM = _vehicle->apmFirmware();
    QVERIFY(isPX4 != isAPM);
}

void InitialConnectTest::_testVehicleTypeProperties()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    VERIFY_NOT_NULL(_vehicle);

    QVERIFY(!_vehicle->vehicleTypeString().isEmpty());

    const bool isMultiRotor = _vehicle->multiRotor();
    const bool isFixedWing = _vehicle->fixedWing();
    const bool isVTOL = _vehicle->vtol();
    const bool isRover = _vehicle->rover();
    const bool isSub = _vehicle->sub();
    const bool isAirship = _vehicle->airship();

    const int typeCount = (isMultiRotor ? 1 : 0) +
                          (isFixedWing ? 1 : 0) +
                          (isVTOL ? 1 : 0) +
                          (isRover ? 1 : 0) +
                          (isSub ? 1 : 0) +
                          (isAirship ? 1 : 0);
    QCOMPARE_GE(typeCount, 1);
}

// ============================================================================
// Signal Tests
// ============================================================================

void InitialConnectTest::_testInitialConnectCompleteSignal()
{
    auto *mvm = MultiVehicleManager::instance();
    QSignalSpy vehicleSpy(mvm, &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(vehicleSpy);

    _mockLink = MockLink::startPX4MockLink(false);
    QVERIFY(vehicleSpy.wait(TestHelpers::kLongTimeoutMs));

    _vehicle = mvm->activeVehicle();
    VERIFY_NOT_NULL(_vehicle);

    if (!_vehicle->isInitialConnectComplete()) {
        QSignalSpy connectSpy(_vehicle, &Vehicle::initialConnectComplete);
        QGC_VERIFY_SPY_VALID(connectSpy);
        QVERIFY(connectSpy.wait(TestHelpers::kLongTimeoutMs));
    }

    QVERIFY(_vehicle->isInitialConnectComplete());
}

void InitialConnectTest::_testCapabilitiesKnownSignal()
{
    auto *mvm = MultiVehicleManager::instance();
    QSignalSpy vehicleSpy(mvm, &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(vehicleSpy);

    _mockLink = MockLink::startPX4MockLink(false);
    QVERIFY(vehicleSpy.wait(TestHelpers::kLongTimeoutMs));

    _vehicle = mvm->activeVehicle();
    VERIFY_NOT_NULL(_vehicle);

    if (!_vehicle->capabilitiesKnown()) {
        QSignalSpy capSpy(_vehicle, &Vehicle::capabilitiesKnownChanged);
        QGC_VERIFY_SPY_VALID(capSpy);
        QVERIFY(capSpy.wait(TestHelpers::kLongTimeoutMs));
    }

    QVERIFY(_vehicle->capabilitiesKnown());
    QVERIFY(_vehicle->capabilityBits() != 0);
}

// ============================================================================
// Failure Mode Tests
// ============================================================================

void InitialConnectTest::_testFailNone()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone);
    VERIFY_NOT_NULL(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
}

void InitialConnectTest::_testFailAutopilotVersionFailure()
{
    _connectMockLink(MAV_AUTOPILOT_PX4,
                     MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure);
    VERIFY_NOT_NULL(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
}

void InitialConnectTest::_testFailAutopilotVersionLost()
{
    _connectMockLink(MAV_AUTOPILOT_PX4,
                     MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost);
    VERIFY_NOT_NULL(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
}

void InitialConnectTest::_testFailParamNoResponse()
{
    auto *mvm = MultiVehicleManager::instance();
    QSignalSpy vehicleSpy(mvm, &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(vehicleSpy);

    _mockLink = MockLink::startPX4MockLink(false, MockConfiguration::FailParamNoResponseToRequestList);

    QVERIFY(vehicleSpy.wait(TestHelpers::kLongTimeoutMs));
    _vehicle = mvm->activeVehicle();
    VERIFY_NOT_NULL(_vehicle);
}

void InitialConnectTest::_testFailMissingParamOnInitial()
{
    _connectMockLink(MAV_AUTOPILOT_PX4,
                     MockConfiguration::FailMissingParamOnInitialRequest);
    VERIFY_NOT_NULL(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
}

void InitialConnectTest::_testFailMissingParamOnAllRequests()
{
    auto *mvm = MultiVehicleManager::instance();
    QSignalSpy vehicleSpy(mvm, &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(vehicleSpy);

    _mockLink = MockLink::startPX4MockLink(false, MockConfiguration::FailMissingParamOnAllRequests);

    QVERIFY(vehicleSpy.wait(TestHelpers::kLongTimeoutMs));
    _vehicle = mvm->activeVehicle();
    VERIFY_NOT_NULL(_vehicle);
}
