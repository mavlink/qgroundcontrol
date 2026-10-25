#include "PX4CalibrationStatusTextTest.h"

#include <QtTest/QSignalSpy>

#include "MockLink.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

void PX4CalibrationStatusTextTest::_calibrationTextNotShownAsVehicleMessage()
{
    QVERIFY(vehicle());

    QSignalSpy textSpy(vehicle(), &Vehicle::textMessageReceived);
    const int unreadBefore = vehicle()->unreadMessageCount();
    const int criticalBefore = vehicle()->unreadCriticalMessageCount();

    mockLink()->sendStatusTextMessage(MAV_SEVERITY_CRITICAL, QStringLiteral("[cal] calibration cancelled"));
    mockLink()->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] progress <10>"));
    // Sent last so the [cal] texts before it have been handled once it shows up
    mockLink()->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("Regular message"));

    QTRY_COMPARE_WITH_TIMEOUT(vehicle()->unreadMessageCount(), unreadBefore + 1, TestTimeout::mediumMs());
    QCOMPARE(vehicle()->unreadCriticalMessageCount(), criticalBefore);
    QVERIFY(!vehicle()->formattedMessages().contains(QStringLiteral("[cal]")));

    QStringList received;
    for (const QList<QVariant>& args : std::as_const(textSpy)) {
        received.append(args.at(3).toString());
    }
    QVERIFY(received.contains(QStringLiteral("[cal] calibration cancelled")));
    QVERIFY(received.contains(QStringLiteral("[cal] progress <10>")));
}

UT_REGISTER_TEST(PX4CalibrationStatusTextTest, TestLabel::Integration, TestLabel::Vehicle)
