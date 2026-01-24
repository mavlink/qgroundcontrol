#include "MockConfigurationTest.h"
#include "MockConfiguration.h"

#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Construction Tests
// ============================================================================

void MockConfigurationTest::_testDefaultConstruction()
{
    MockConfiguration config("TestConfig");
    QCOMPARE(config.name(), QStringLiteral("TestConfig"));
}

void MockConfigurationTest::_testNamedConstruction()
{
    MockConfiguration config("MyMockLink");
    QCOMPARE(config.name(), QStringLiteral("MyMockLink"));
    VERIFY_NOT_NULL(&config);
}

void MockConfigurationTest::_testCopyConstruction()
{
    MockConfiguration original("Original");
    original.setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    original.setVehicleType(MAV_TYPE_FIXED_WING);
    original.setSendStatusText(true);
    original.setIncrementVehicleId(false);
    original.setFailureMode(MockConfiguration::FailParamNoResponseToRequestList);
    original.setBoardVendorProduct(0x1234, 0x5678);

    MockConfiguration copy(&original);

    QCOMPARE(copy.firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
    QCOMPARE(copy.vehicleType(), MAV_TYPE_FIXED_WING);
    QVERIFY(copy.sendStatusText());
    QVERIFY(!copy.incrementVehicleId());
    QCOMPARE(copy.failureMode(), MockConfiguration::FailParamNoResponseToRequestList);
}

// ============================================================================
// Type and URL Tests
// ============================================================================

void MockConfigurationTest::_testLinkType()
{
    MockConfiguration config("TestConfig");
    QCOMPARE(config.type(), LinkConfiguration::TypeMock);
}

void MockConfigurationTest::_testSettingsURL()
{
    MockConfiguration config("TestConfig");
    QCOMPARE(config.settingsURL(), QStringLiteral("MockLinkSettings.qml"));
}

void MockConfigurationTest::_testSettingsTitle()
{
    MockConfiguration config("TestConfig");
    QCOMPARE(config.settingsTitle(), QStringLiteral("Mock Link Settings"));
}

// ============================================================================
// Default Values Tests
// ============================================================================

void MockConfigurationTest::_testDefaultFirmwareType()
{
    MockConfiguration config("TestConfig");
    QCOMPARE(config.firmwareType(), MAV_AUTOPILOT_PX4);
}

void MockConfigurationTest::_testDefaultVehicleType()
{
    MockConfiguration config("TestConfig");
    QCOMPARE(config.vehicleType(), MAV_TYPE_QUADROTOR);
}

void MockConfigurationTest::_testDefaultSendStatusText()
{
    MockConfiguration config("TestConfig");
    QVERIFY(!config.sendStatusText());
}

void MockConfigurationTest::_testDefaultIncrementVehicleId()
{
    MockConfiguration config("TestConfig");
    QVERIFY(config.incrementVehicleId());
}

void MockConfigurationTest::_testDefaultFailureMode()
{
    MockConfiguration config("TestConfig");
    QCOMPARE(config.failureMode(), MockConfiguration::FailNone);
}

void MockConfigurationTest::_testDefaultBoardIds()
{
    MockConfiguration config("TestConfig");
    QCOMPARE_EQ(config.boardVendorId(), static_cast<uint16_t>(0));
    QCOMPARE_EQ(config.boardProductId(), static_cast<uint16_t>(0));
}

// ============================================================================
// Firmware Property Tests
// ============================================================================

void MockConfigurationTest::_testSetFirmwareType()
{
    MockConfiguration config("TestConfig");

    config.setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QCOMPARE(config.firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);

    config.setFirmwareType(MAV_AUTOPILOT_GENERIC);
    QCOMPARE(config.firmwareType(), MAV_AUTOPILOT_GENERIC);

    config.setFirmwareType(MAV_AUTOPILOT_PX4);
    QCOMPARE(config.firmwareType(), MAV_AUTOPILOT_PX4);
}

void MockConfigurationTest::_testSetFirmwareInt()
{
    MockConfiguration config("TestConfig");

    config.setFirmware(static_cast<int>(MAV_AUTOPILOT_ARDUPILOTMEGA));
    QCOMPARE_EQ(config.firmware(), static_cast<int>(MAV_AUTOPILOT_ARDUPILOTMEGA));
}

void MockConfigurationTest::_testFirmwareChangedSignal()
{
    MockConfiguration config("TestConfig");
    QSignalSpy spy(&config, &MockConfiguration::firmwareChanged);
    QVERIFY(spy.isValid());

    config.setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QCOMPARE_EQ(spy.count(), 1);

    config.setFirmware(static_cast<int>(MAV_AUTOPILOT_GENERIC));
    QCOMPARE_EQ(spy.count(), 2);
}

// ============================================================================
// Vehicle Type Property Tests
// ============================================================================

void MockConfigurationTest::_testSetVehicleType()
{
    MockConfiguration config("TestConfig");

    config.setVehicleType(MAV_TYPE_FIXED_WING);
    QCOMPARE(config.vehicleType(), MAV_TYPE_FIXED_WING);

    config.setVehicleType(MAV_TYPE_GROUND_ROVER);
    QCOMPARE(config.vehicleType(), MAV_TYPE_GROUND_ROVER);

    config.setVehicleType(MAV_TYPE_SUBMARINE);
    QCOMPARE(config.vehicleType(), MAV_TYPE_SUBMARINE);

    config.setVehicleType(MAV_TYPE_HELICOPTER);
    QCOMPARE(config.vehicleType(), MAV_TYPE_HELICOPTER);
}

void MockConfigurationTest::_testSetVehicleInt()
{
    MockConfiguration config("TestConfig");

    config.setVehicle(static_cast<int>(MAV_TYPE_FIXED_WING));
    QCOMPARE_EQ(config.vehicle(), static_cast<int>(MAV_TYPE_FIXED_WING));
}

void MockConfigurationTest::_testVehicleChangedSignal()
{
    MockConfiguration config("TestConfig");
    QSignalSpy spy(&config, &MockConfiguration::vehicleChanged);
    QVERIFY(spy.isValid());

    config.setVehicleType(MAV_TYPE_FIXED_WING);
    QCOMPARE_EQ(spy.count(), 1);

    config.setVehicle(static_cast<int>(MAV_TYPE_GROUND_ROVER));
    QCOMPARE_EQ(spy.count(), 2);
}

// ============================================================================
// SendStatusText Property Tests
// ============================================================================

void MockConfigurationTest::_testSetSendStatusText()
{
    MockConfiguration config("TestConfig");

    QVERIFY(!config.sendStatusText());

    config.setSendStatusText(true);
    QVERIFY(config.sendStatusText());

    config.setSendStatusText(false);
    QVERIFY(!config.sendStatusText());
}

void MockConfigurationTest::_testSendStatusChangedSignal()
{
    MockConfiguration config("TestConfig");
    QSignalSpy spy(&config, &MockConfiguration::sendStatusChanged);
    QVERIFY(spy.isValid());

    config.setSendStatusText(true);
    QCOMPARE_EQ(spy.count(), 1);

    config.setSendStatusText(false);
    QCOMPARE_EQ(spy.count(), 2);
}

// ============================================================================
// IncrementVehicleId Property Tests
// ============================================================================

void MockConfigurationTest::_testSetIncrementVehicleId()
{
    MockConfiguration config("TestConfig");

    QVERIFY(config.incrementVehicleId());

    config.setIncrementVehicleId(false);
    QVERIFY(!config.incrementVehicleId());

    config.setIncrementVehicleId(true);
    QVERIFY(config.incrementVehicleId());
}

void MockConfigurationTest::_testIncrementVehicleIdChangedSignal()
{
    MockConfiguration config("TestConfig");
    QSignalSpy spy(&config, &MockConfiguration::incrementVehicleIdChanged);
    QVERIFY(spy.isValid());

    config.setIncrementVehicleId(false);
    QCOMPARE_EQ(spy.count(), 1);

    config.setIncrementVehicleId(true);
    QCOMPARE_EQ(spy.count(), 2);
}

// ============================================================================
// Board Vendor/Product IDs Tests
// ============================================================================

void MockConfigurationTest::_testSetBoardVendorProduct()
{
    MockConfiguration config("TestConfig");

    config.setBoardVendorProduct(0x1234, 0x5678);
    QCOMPARE_EQ(config.boardVendorId(), static_cast<uint16_t>(0x1234));
    QCOMPARE_EQ(config.boardProductId(), static_cast<uint16_t>(0x5678));

    config.setBoardVendorProduct(0xFFFF, 0x0000);
    QCOMPARE_EQ(config.boardVendorId(), static_cast<uint16_t>(0xFFFF));
    QCOMPARE_EQ(config.boardProductId(), static_cast<uint16_t>(0x0000));
}

// ============================================================================
// FailureMode_t Enum Tests
// ============================================================================

void MockConfigurationTest::_testFailureModeEnumValues()
{
    QCOMPARE_EQ(static_cast<int>(MockConfiguration::FailNone), 0);
    QCOMPARE_EQ(static_cast<int>(MockConfiguration::FailParamNoResponseToRequestList), 1);
    QCOMPARE_EQ(static_cast<int>(MockConfiguration::FailMissingParamOnInitialRequest), 2);
    QCOMPARE_EQ(static_cast<int>(MockConfiguration::FailMissingParamOnAllRequests), 3);
    QCOMPARE_EQ(static_cast<int>(MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure), 4);
    QCOMPARE_EQ(static_cast<int>(MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost), 5);
}

void MockConfigurationTest::_testSetFailureMode()
{
    MockConfiguration config("TestConfig");

    config.setFailureMode(MockConfiguration::FailNone);
    QCOMPARE(config.failureMode(), MockConfiguration::FailNone);

    config.setFailureMode(MockConfiguration::FailParamNoResponseToRequestList);
    QCOMPARE(config.failureMode(), MockConfiguration::FailParamNoResponseToRequestList);

    config.setFailureMode(MockConfiguration::FailMissingParamOnInitialRequest);
    QCOMPARE(config.failureMode(), MockConfiguration::FailMissingParamOnInitialRequest);

    config.setFailureMode(MockConfiguration::FailMissingParamOnAllRequests);
    QCOMPARE(config.failureMode(), MockConfiguration::FailMissingParamOnAllRequests);

    config.setFailureMode(MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure);
    QCOMPARE(config.failureMode(), MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure);

    config.setFailureMode(MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost);
    QCOMPARE(config.failureMode(), MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost);
}

// ============================================================================
// CopyFrom Tests
// ============================================================================

void MockConfigurationTest::_testCopyFrom()
{
    MockConfiguration source("Source");
    source.setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    source.setVehicleType(MAV_TYPE_FIXED_WING);

    MockConfiguration target("Target");
    target.copyFrom(&source);

    QCOMPARE(target.firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
    QCOMPARE(target.vehicleType(), MAV_TYPE_FIXED_WING);
}

void MockConfigurationTest::_testCopyFromPreservesAllProperties()
{
    MockConfiguration source("Source");
    source.setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    source.setVehicleType(MAV_TYPE_GROUND_ROVER);
    source.setSendStatusText(true);
    source.setIncrementVehicleId(false);
    source.setFailureMode(MockConfiguration::FailMissingParamOnAllRequests);

    MockConfiguration target("Target");
    target.copyFrom(&source);

    QCOMPARE(target.firmwareType(), source.firmwareType());
    QCOMPARE(target.vehicleType(), source.vehicleType());
    QCOMPARE(target.sendStatusText(), source.sendStatusText());
    QCOMPARE(target.incrementVehicleId(), source.incrementVehicleId());
    QCOMPARE(target.failureMode(), source.failureMode());
}

// ============================================================================
// Settings Persistence Tests
// ============================================================================

void MockConfigurationTest::_testSaveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "QGCTest", "MockConfigTest");
    settings.clear();

    MockConfiguration config("TestConfig");
    config.setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    config.setVehicleType(MAV_TYPE_FIXED_WING);
    config.setSendStatusText(true);
    config.setIncrementVehicleId(false);
    config.setFailureMode(MockConfiguration::FailParamNoResponseToRequestList);

    config.saveSettings(settings, "TestRoot");
    settings.sync();

    settings.beginGroup("TestRoot");
    QCOMPARE_EQ(settings.value("FirmwareType").toInt(), static_cast<int>(MAV_AUTOPILOT_ARDUPILOTMEGA));
    QCOMPARE_EQ(settings.value("VehicleType").toInt(), static_cast<int>(MAV_TYPE_FIXED_WING));
    QVERIFY(settings.value("SendStatusText").toBool());
    QVERIFY(!settings.value("IncrementVehicleId").toBool());
    QCOMPARE_EQ(settings.value("FailureMode").toInt(), static_cast<int>(MockConfiguration::FailParamNoResponseToRequestList));
    settings.endGroup();

    settings.clear();
}

void MockConfigurationTest::_testLoadSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "QGCTest", "MockConfigTest");
    settings.clear();

    settings.beginGroup("TestRoot");
    settings.setValue("FirmwareType", static_cast<int>(MAV_AUTOPILOT_GENERIC));
    settings.setValue("VehicleType", static_cast<int>(MAV_TYPE_SUBMARINE));
    settings.setValue("SendStatusText", true);
    settings.setValue("IncrementVehicleId", false);
    settings.setValue("FailureMode", static_cast<int>(MockConfiguration::FailMissingParamOnInitialRequest));
    settings.endGroup();
    settings.sync();

    MockConfiguration config("TestConfig");
    config.loadSettings(settings, "TestRoot");

    QCOMPARE(config.firmwareType(), MAV_AUTOPILOT_GENERIC);
    QCOMPARE(config.vehicleType(), MAV_TYPE_SUBMARINE);
    QVERIFY(config.sendStatusText());
    QVERIFY(!config.incrementVehicleId());
    QCOMPARE(config.failureMode(), MockConfiguration::FailMissingParamOnInitialRequest);

    settings.clear();
}

void MockConfigurationTest::_testSettingsRoundTrip()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "QGCTest", "MockConfigTest");
    settings.clear();

    MockConfiguration original("Original");
    original.setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    original.setVehicleType(MAV_TYPE_HELICOPTER);
    original.setSendStatusText(true);
    original.setIncrementVehicleId(false);
    original.setFailureMode(MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost);

    original.saveSettings(settings, "RoundTrip");
    settings.sync();

    MockConfiguration loaded("Loaded");
    loaded.loadSettings(settings, "RoundTrip");

    QCOMPARE(loaded.firmwareType(), original.firmwareType());
    QCOMPARE(loaded.vehicleType(), original.vehicleType());
    QCOMPARE(loaded.sendStatusText(), original.sendStatusText());
    QCOMPARE(loaded.incrementVehicleId(), original.incrementVehicleId());
    QCOMPARE(loaded.failureMode(), original.failureMode());

    settings.clear();
}
