#include "PX4TakeoffTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QtMath>

#include "MockLink.h"
#include "Vehicle.h"

namespace {
constexpr double kTakeoffAltitudeRel = 10.0;
}  // namespace

void PX4TakeoffTest::_prepareTakeoff()
{
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(!qIsNaN(vehicle()->altitudeAMSL()->rawValue().toDouble()), TestTimeout::shortMs());
    QVERIFY(!vehicle()->armed());

    // The accepted takeoff moves the vehicle. The airborne transition creates QGCPressure, which warns on hosts
    // without a pressure backend.
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));

    mockLink()->clearReceivedMavCommandCounts();
}

void PX4TakeoffTest::_waitForTakeoffResult()
{
    QVERIFY_TRUE_WAIT(!vehicle()->isMavCommandPending(vehicle()->defaultComponentId(), MAV_CMD_NAV_TAKEOFF),
                      TestTimeout::mediumMs());
}

void PX4TakeoffTest::_verifyArmCommandCount(int expectedCount)
{
    // An ARM sent in response to the takeoff ack goes out before this tracked no-op, so it is counted by then
    vehicle()->sendMavCommand(vehicle()->defaultComponentId(), MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,
                              false /* showError */);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED) == 1,
                      TestTimeout::mediumMs());
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_COMPONENT_ARM_DISARM), expectedCount);
}

void PX4TakeoffTest::_takeoffAfterRejectedTakeoff_data()
{
    QTest::addColumn<bool>("guidedTakeoff");
    QTest::addColumn<int>("expectedArmCommands");

    QTest::newRow("guided takeoff arms") << true << 1;
    QTest::newRow("other NAV_TAKEOFF does not arm") << false << 0;
}

void PX4TakeoffTest::_takeoffAfterRejectedTakeoff()
{
    QFETCH(bool, guidedTakeoff);
    QFETCH(int, expectedArmCommands);

    _prepareTakeoff();
    if (QTest::currentTestFailed()) {
        return;
    }

    mockLink()->setTakeoffFailureMode(true);
    expectAppMessage(QRegularExpression(QStringLiteral("NAV_TAKEOFF.*temporarily rejected")));
    vehicle()->guidedModeTakeoff(kTakeoffAltitudeRel);
    _waitForTakeoffResult();
    verifyExpectedLogMessage();
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_NAV_TAKEOFF), 1);

    mockLink()->setTakeoffFailureMode(false);
    if (guidedTakeoff) {
        vehicle()->guidedModeTakeoff(kTakeoffAltitudeRel);
    } else {
        // Stands in for a NAV_TAKEOFF from another code path, such as a MAVLink action
        vehicle()->sendMavCommand(vehicle()->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, NAN, NAN,
                                  0, NAN, NAN, NAN, static_cast<float>(kTakeoffAltitudeRel));
    }
    _waitForTakeoffResult();
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_NAV_TAKEOFF), 2);

    _verifyArmCommandCount(expectedArmCommands);
}

void PX4TakeoffTest::_repeatedTakeoffWhilePendingArmsOnce()
{
    _prepareTakeoff();
    if (QTest::currentTestFailed()) {
        return;
    }

    vehicle()->guidedModeTakeoff(kTakeoffAltitudeRel);
    // The second request fails as a duplicate. The first one is still waiting for its ack.
    expectAppMessage(QRegularExpression(QStringLiteral("Waiting on previous response to same command")));
    vehicle()->guidedModeTakeoff(kTakeoffAltitudeRel);
    verifyExpectedLogMessage();
    _waitForTakeoffResult();

    _verifyArmCommandCount(1);
}

UT_REGISTER_TEST(PX4TakeoffTest, TestLabel::Integration, TestLabel::Vehicle)
