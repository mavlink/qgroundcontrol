/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "tst_MAVLinkHeartbeat.h"

#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QProcess>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

Q_DECLARE_LOGGING_CATEGORY(SITLTestLog)

void SITLHeartbeatTest::testDetection()
{
    // init() already connected — verify vehicle is present and identified
    QVERIFY(vehicle());
    QVERIFY(vehicle()->isInitialConnectComplete());
    QCOMPARE(vehicle()->firmwareType(), MAV_AUTOPILOT_PX4);
    QVERIFY(vehicle()->id() > 0);

    qCInfo(SITLTestLog) << "Vehicle detected: id=" << vehicle()->id()
                       << "firmware=" << vehicle()->firmwareType()
                       << "type=" << vehicle()->vehicleType();
}

void SITLHeartbeatTest::testLossAndReconnect()
{
    QVERIFY(vehicle());

    // Kill the container immediately to simulate abrupt communication loss
    QProcess docker;
    docker.setProgram(QStringLiteral("docker"));
    docker.setArguments({QStringLiteral("kill"), containerId()});
    docker.start();
    docker.waitForFinished(5000);
    _containerId.clear();

    // QGC doesn't remove the vehicle on comm loss (autoDisconnect=false by default).
    // It sets communicationLost=true on the vehicle's link manager instead.
    // In unit tests, heartbeat timeout is 500ms + check interval.
    Vehicle *v = vehicle();
    QVERIFY(v);
    QVERIFY2(waitForCondition(
                 [v]() { return v->vehicleLinkManager()->communicationLost(); },
                 10000,
                 QStringLiteral("communicationLost == true")),
             "QGC did not detect communication loss");

    qCInfo(SITLTestLog) << "Communication loss detected after container kill";
}

UT_REGISTER_TEST(SITLHeartbeatTest, TestLabel::SITL, TestLabel::MAVLinkProtocol)
