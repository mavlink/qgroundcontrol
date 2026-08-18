#include "MAVLinkBandwidthControllerTest.h"

#include <QtTest/QSignalSpy>

#include "MAVLinkBandwidthController.h"

void MAVLinkBandwidthControllerTest::_mavftpRoundTripTest()
{
    MAVLinkBandwidthController controller;
    controller.setTestMode(MAVLinkBandwidthController::TestMode::MavFtp);
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

void MAVLinkBandwidthControllerTest::_mavftpCancelRestartTest()
{
    MAVLinkBandwidthController controller;
    controller.setTestMode(MAVLinkBandwidthController::TestMode::MavFtp);
    controller.setFtpFileSizeKiB(64);

    controller.startTest();
    QVERIFY(controller.running());
    controller.stopTest();
    QVERIFY_TRUE_WAIT(!controller.running(), TestTimeout::longMs());
    QCOMPARE(controller.statusText(), QStringLiteral("MAVFTP test stopped."));
    QVERIFY(controller.canStart());

    controller.startTest();
    QVERIFY(controller.running());
    QVERIFY_TRUE_WAIT(!controller.running(), TestTimeout::longMs());
    QVERIFY2(controller.statusText().contains(QStringLiteral("complete and verified")),
             qPrintable(controller.statusText()));
    QCOMPARE(controller.transmittedPayloadBytes(), 64U * 1024U);
    QCOMPARE(controller.receivedPayloadBytes(), 64U * 1024U);
}

UT_REGISTER_TEST(MAVLinkBandwidthControllerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::AnalyzeView)
