#include "MAVLinkBandwidthControllerTest.h"

#include <QtTest/QSignalSpy>

#include "MAVLinkBandwidthController.h"

void MAVLinkBandwidthControllerTest::_mavftpRoundTripTest_data()
{
    QTest::addColumn<int>("direction");

    QTest::newRow("upload") << static_cast<int>(MAVLinkBandwidthController::Direction::QgcToVehicle);
    QTest::newRow("download") << static_cast<int>(MAVLinkBandwidthController::Direction::VehicleToQgc);
}

void MAVLinkBandwidthControllerTest::_mavftpRoundTripTest()
{
    QFETCH(int, direction);

    MAVLinkBandwidthController controller;
    controller.setTestMode(MAVLinkBandwidthController::TestMode::MavFtp);
    controller.setDirection(static_cast<MAVLinkBandwidthController::Direction>(direction));
    controller.setFtpFileSizeKiB(64);

    QCOMPARE(controller.testMode(), MAVLinkBandwidthController::TestMode::MavFtp);
    QVERIFY(controller.vehicleAvailable());
    QVERIFY(controller.canStart());

    QSignalSpy runningSpy(&controller, &MAVLinkBandwidthController::runningChanged);
    QVERIFY(runningSpy.isValid());

    controller.startTest();
    QVERIFY(controller.running());
    QVERIFY_TRUE_WAIT(!controller.running(), TestTimeout::longMs());

    QVERIFY2(controller.statusText().contains(QStringLiteral("complete and verified")),
             qPrintable(controller.statusText()));
    QCOMPARE(controller.transmittedPayloadBytes(), 64U * 1024U);
    QCOMPARE(controller.receivedPayloadBytes(), 64U * 1024U);
    QCOMPARE(controller.progress(), 1.);
    QVERIFY(controller.transmitRateKbps() > 0.);
    QVERIFY(controller.receiveRateKbps() > 0.);
    QVERIFY(runningSpy.count() >= 2);
}

UT_REGISTER_TEST(MAVLinkBandwidthControllerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::AnalyzeView)
