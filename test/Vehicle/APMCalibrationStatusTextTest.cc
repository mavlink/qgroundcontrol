#include "APMCalibrationStatusTextTest.h"

#include "MockLink.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

void APMCalibrationStatusTextTest::_calibrationPrefixedTextShownAsVehicleMessage()
{
    QVERIFY(vehicle());

    const int unreadBefore = vehicle()->unreadMessageCount();

    mockLink()->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] progress <10>"));

    QTRY_COMPARE_WITH_TIMEOUT(vehicle()->unreadMessageCount(), unreadBefore + 1, TestTimeout::mediumMs());
    QVERIFY(vehicle()->formattedMessages().contains(QStringLiteral("[cal] progress")));
}

UT_REGISTER_TEST(APMCalibrationStatusTextTest, TestLabel::Integration, TestLabel::Vehicle)
