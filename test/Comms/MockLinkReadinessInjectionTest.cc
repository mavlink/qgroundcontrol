#include "MockLinkReadinessInjectionTest.h"

#include <QtTest/QSignalSpy>

#include "BatteryFactGroupListModel.h"
#include "FactGroup.h"
#include "Vehicle.h"

void MockLinkReadinessInjectionTest::_testSysStatusSensorBitsInjection()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    // Default MockLink SYS_STATUS: enabled = 0 makes (enabled & health) == enabled vacuously true
    QTRY_COMPARE_WITH_TIMEOUT(vehicle()->sensorsPresentBits(), static_cast<int>(MAV_SYS_STATUS_SENSOR_GPS), TestTimeout::mediumMs());
    QCOMPARE(vehicle()->sensorsUnhealthyBits(), 0);

    // Inject: GPS + RC present and enabled, only GPS healthy -> RC must surface as unhealthy
    const quint32 presentEnabled = MAV_SYS_STATUS_SENSOR_GPS | MAV_SYS_STATUS_SENSOR_RC_RECEIVER;
    mockLink()->setSysStatusSensorBits(presentEnabled, presentEnabled, MAV_SYS_STATUS_SENSOR_GPS);

    QTRY_COMPARE_WITH_TIMEOUT(vehicle()->sensorsPresentBits(), static_cast<int>(presentEnabled), TestTimeout::mediumMs());
    QCOMPARE(vehicle()->sensorsEnabledBits(), static_cast<int>(presentEnabled));
    QCOMPARE(vehicle()->sensorsUnhealthyBits(), static_cast<int>(MAV_SYS_STATUS_SENSOR_RC_RECEIVER));
    QVERIFY(!vehicle()->allSensorsHealthy());

    // Recover -> healthy again
    mockLink()->setSysStatusSensorBits(presentEnabled, presentEnabled, presentEnabled);
    QTRY_COMPARE_WITH_TIMEOUT(vehicle()->sensorsUnhealthyBits(), 0, TestTimeout::mediumMs());
    QVERIFY(vehicle()->allSensorsHealthy());
}

void MockLinkReadinessInjectionTest::_testGpsFixLostInjection()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    Fact* lockFact = vehicle()->gpsFactGroup()->getFact(QStringLiteral("lock"));
    QVERIFY(lockFact);

    QTRY_COMPARE_WITH_TIMEOUT(lockFact->rawValue().toInt(), static_cast<int>(GPS_FIX_TYPE_3D_FIX), TestTimeout::mediumMs());

    mockLink()->setGpsFixLost(true);
    QTRY_COMPARE_WITH_TIMEOUT(lockFact->rawValue().toInt(), static_cast<int>(GPS_FIX_TYPE_NO_FIX), TestTimeout::mediumMs());

    mockLink()->setGpsFixLost(false);
    QTRY_COMPARE_WITH_TIMEOUT(lockFact->rawValue().toInt(), static_cast<int>(GPS_FIX_TYPE_3D_FIX), TestTimeout::mediumMs());
}

void MockLinkReadinessInjectionTest::_testBatteryCriticalInjection()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    mockLink()->setBatteryCritical(true);

    // Battery 1 is the first entry in the list model (MockLink sends ids 1 then 2)
    QVERIFY(UnitTest::waitForCondition(
        [this]() {
            if (vehicle()->batteries()->count() < 1) {
                return false;
            }
            BatteryFactGroup* battery = vehicle()->batteries()->value<BatteryFactGroup*>(0);
            return battery
                && (battery->percentRemaining()->rawValue().toInt() == 5)
                && (battery->chargeState()->rawValue().toInt() == static_cast<int>(MAV_BATTERY_CHARGE_STATE_EMERGENCY));
        },
        TestTimeout::mediumMs(),
        QStringLiteral("battery 1 reports 5% remaining in EMERGENCY charge state")));
}

void MockLinkReadinessInjectionTest::_testPreArmDeniedInjection()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());
    QVERIFY(!vehicle()->armed());

    mockLink()->setPreArmDenied(true, QStringLiteral("Injected pre-arm failure"));

    QSignalSpy commandResultSpy(vehicle(), &Vehicle::mavCommandResult);
    QVERIFY(commandResultSpy.isValid());

    vehicle()->setArmed(true, false /* showError */);

    QVERIFY_SIGNAL_WAIT(commandResultSpy, TestTimeout::mediumMs());
    const QList<QVariant> arguments = commandResultSpy.takeFirst();
    QCOMPARE(arguments.at(2).toInt(), static_cast<int>(MAV_CMD_COMPONENT_ARM_DISARM));
    QCOMPARE(arguments.at(3).toInt(), static_cast<int>(MAV_RESULT_TEMPORARILY_REJECTED));
    QVERIFY(!mockLink()->armed());

    // Clearing the injection must allow a normal arm
    mockLink()->setPreArmDenied(false);
    vehicle()->setArmed(true, false /* showError */);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->armed(), TestTimeout::mediumMs());

    vehicle()->setArmed(false, false /* showError */);
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle()->armed(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(MockLinkReadinessInjectionTest, TestLabel::Integration, TestLabel::Comms, TestLabel::Vehicle)
