/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "tst_PX4Lifecycle.h"

#include "Vehicle.h"

#include <QtCore/QLoggingCategory>
#include <QtTest/QTest>

Q_DECLARE_LOGGING_CATEGORY(PX4SITLTestLog)

void PX4LifecycleTest::testArmTakeoffLandDisarm()
{
    QVERIFY(vehicle());
    QVERIFY(!vehicle()->armed());

    // Arm — use a generous timeout. SIH may need time for EKF convergence
    // after initial connect before preflight checks pass.
    // QGC's sendMavCommand retries automatically on rejection.
    QVERIFY2(armVehicle(30000), "Failed to arm vehicle");
    QVERIFY(vehicle()->armed());

    // Takeoff to 10m
    QVERIFY2(takeoff(10.0f), "Failed to reach takeoff altitude");

    // Hold for 5 seconds
    QTest::qWait(5000);

    // Verify still airborne
    const float alt = vehicle()->altitudeRelative()->rawValue().toFloat();
    QVERIFY2(alt > 5.0f, qPrintable(QStringLiteral("Expected alt > 5m, got %1m").arg(alt)));

    // Land
    QVERIFY2(land(60000), "Failed to land");

    // PX4 auto-disarms after landing (COM_DISARM_LAND default = 2s)
    QVERIFY2(waitForArmedState(false, 15000), "Vehicle did not auto-disarm after landing");

    qCInfo(PX4SITLTestLog) << "Full lifecycle complete: arm → takeoff → hold → land → disarm";
}

void PX4LifecycleTest::testFirmwareIdentification()
{
    QVERIFY(vehicle());

    QCOMPARE(vehicle()->firmwareType(), MAV_AUTOPILOT_PX4);
    QVERIFY(!vehicle()->firmwareVersionTypeString().isEmpty());
    QVERIFY(vehicle()->id() > 0);

    // Verify PX4FirmwarePlugin was selected (not generic)
    QVERIFY(vehicle()->firmwarePlugin());
    QVERIFY(vehicle()->firmwarePluginInstanceData());

    qCInfo(PX4SITLTestLog) << "PX4 firmware identified:"
            << "version=" << vehicle()->firmwareVersionTypeString()
            << "vehicleId=" << vehicle()->id();
}

UT_REGISTER_TEST(PX4LifecycleTest, TestLabel::SITL, TestLabel::Vehicle)
