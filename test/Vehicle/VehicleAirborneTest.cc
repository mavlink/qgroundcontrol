#include "VehicleAirborneTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(VehicleAirborneTest, TestLabel::Integration, TestLabel::Vehicle)

void VehicleAirborneTest::_airborneOnlyForAircraft_data()
{
    QTest::addColumn<int>("firmware");
    QTest::addColumn<int>("vehicleType");
    QTest::addColumn<bool>("expectAirborne");

    QTest::addRow("PX4 multirotor") << int(MAV_AUTOPILOT_PX4) << int(MAV_TYPE_QUADROTOR) << true;
    QTest::addRow("PX4 rover") << int(MAV_AUTOPILOT_PX4) << int(MAV_TYPE_GROUND_ROVER) << false;
    QTest::addRow("ArduPilot rover") << int(MAV_AUTOPILOT_ARDUPILOTMEGA) << int(MAV_TYPE_GROUND_ROVER) << false;
    QTest::addRow("ArduPilot sub") << int(MAV_AUTOPILOT_ARDUPILOTMEGA) << int(MAV_TYPE_SUBMARINE) << false;
}

void VehicleAirborneTest::_airborneOnlyForAircraft()
{
    QFETCH(int, firmware);
    QFETCH(int, vehicleType);
    QFETCH(bool, expectAirborne);

    if ((firmware == MAV_AUTOPILOT_ARDUPILOTMEGA) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(spyVehicle.isValid());

    auto* const mockConfig = new MockConfiguration(QStringLiteral("VehicleAirborneTest MockLink"));
    mockConfig->setFirmwareType(static_cast<MAV_AUTOPILOT>(firmware));
    mockConfig->setVehicleType(static_cast<MAV_TYPE>(vehicleType));
    _mockLink = MockLink::startMockLink(mockConfig);
    QVERIFY(_mockLink);

    QVERIFY(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")));
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    QVERIFY(waitForInitialConnect());

    // The airborne transition creates QGCPressure, which warns on hosts without a pressure backend
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));

    QSignalSpy spyAirborne(_vehicle, &Vehicle::airborneChanged);
    QVERIFY(spyAirborne.isValid());

    // MockLink arms and moves the vehicle away from home, which every firmware/vehicle type reports as underway
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, 0.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
    QVERIFY_TRUE_WAIT(_vehicle->underway(), TestTimeout::longMs());

    QCOMPARE(_vehicle->airborne(), expectAirborne);
    QCOMPARE(spyAirborne.count(), expectAirborne ? 1 : 0);
}
