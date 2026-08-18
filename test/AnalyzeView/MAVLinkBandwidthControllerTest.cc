#include "MAVLinkBandwidthControllerTest.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>

#include "MAVLinkBandwidthController.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"

void MAVLinkBandwidthControllerTest::_firmwareModeAvailabilityTest()
{
    MAVLinkBandwidthController controller;

    QVERIFY(!controller.streamingSupported());
    QCOMPARE(controller.testMode(), MAVLinkBandwidthController::TestMode::MavFtp);

    _disconnectMockLink();
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY_TRUE_WAIT(controller.streamingSupported(), TestTimeout::longMs());
    QCOMPARE(controller.testMode(), MAVLinkBandwidthController::TestMode::Streaming);
}

void MAVLinkBandwidthControllerTest::_scriptInstallAndRebootTest()
{
    _disconnectMockLink();
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    MAVLinkBandwidthController controller;
    QSignalSpy deploymentSpy(&controller, &MAVLinkBandwidthController::scriptDeploymentFinished);
    QVERIFY(deploymentSpy.isValid());

    QPointer<MockLink> testMockLink = mockLink();
    controller.installScriptAndReboot();
    QVERIFY_SIGNAL_WAIT(deploymentSpy, TestTimeout::longMs());
    QCOMPARE(deploymentSpy.count(), 1);
    QVERIFY2(deploymentSpy.first().at(0).toBool(), qPrintable(deploymentSpy.first().at(1).toString()));
    QCOMPARE(controller.scriptDeploymentProgress(), 1.F);

    QFile embeddedScript(QStringLiteral(":/mavlink-bandwidth/mavlink_bandwidth.lua"));
    QVERIFY(embeddedScript.open(QIODevice::ReadOnly));
    const QByteArray expectedContents = embeddedScript.readAll();
    QVERIFY(!expectedContents.isEmpty());
    QVERIFY(testMockLink);
    QCOMPARE(testMockLink->mockLinkFTP()->uploadedFileContents(QStringLiteral("/APM/scripts/mavlink_bandwidth.lua")),
             expectedContents);

    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->activeVehicle() == nullptr, TestTimeout::longMs());
}

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
