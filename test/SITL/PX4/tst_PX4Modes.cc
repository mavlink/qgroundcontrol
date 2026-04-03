/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "tst_PX4Modes.h"

#include "Vehicle.h"

#include <QtCore/QLoggingCategory>
#include <QtTest/QTest>

Q_DECLARE_LOGGING_CATEGORY(PX4SITLTestLog)

void PX4ModesTest::testTransitions()
{
    QVERIFY(vehicle());

    // Get available modes
    const QStringList modes = vehicle()->flightModes();
    QVERIFY(!modes.isEmpty());

    qCInfo(PX4SITLTestLog) << "Testing mode transitions. Available modes:" << modes;

    // Test transitions to modes that can be set while disarmed
    // Position Hold is a safe mode to transition to while on the ground
    if (modes.contains(QStringLiteral("Position"))) {
        QVERIFY2(setFlightMode(QStringLiteral("Position"), 5000),
                 "Failed to transition to Position mode");
        QCOMPARE(vehicle()->flightMode(), QStringLiteral("Position"));
        qCInfo(PX4SITLTestLog) << "Transitioned to Position mode";
    }

    // Mission mode
    if (modes.contains(QStringLiteral("Mission"))) {
        QVERIFY2(setFlightMode(QStringLiteral("Mission"), 5000),
                 "Failed to transition to Mission mode");
        QCOMPARE(vehicle()->flightMode(), QStringLiteral("Mission"));
        qCInfo(PX4SITLTestLog) << "Transitioned to Mission mode";
    }

    qCInfo(PX4SITLTestLog) << "Mode transition test complete";
}

UT_REGISTER_TEST(PX4ModesTest, TestLabel::SITL, TestLabel::Vehicle)
