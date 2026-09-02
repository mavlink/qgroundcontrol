#include "MAVLinkBandwidthControllerTest.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>

#include "FTPManager.h"
#include "MAVLinkBandwidthController.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

void MAVLinkBandwidthControllerTest::_firmwareModeAvailabilityTest()
{
    MAVLinkBandwidthController controller;

    QVERIFY(!controller.streamingSupported());
    QCOMPARE(controller.testMode(), MAVLinkBandwidthController::TestMode::MavFtp);

    _disconnectMockLink();
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY_TRUE_WAIT(controller.streamingSupported(), TestTimeout::longMs());
    QCOMPARE(controller.testMode(), MAVLinkBandwidthController::TestMode::Streaming);
    controller.installScriptAndReboot(controller.activeVehicleId() + 1);
    QVERIFY(!controller.scriptDeploying());
    QVERIFY(controller.statusText().contains(QStringLiteral("active vehicle changed")));
}

void MAVLinkBandwidthControllerTest::_scriptInstallAndRebootTest()
{
    _disconnectMockLink();
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    QTemporaryFile existingScript;
    QVERIFY(existingScript.open());
    QCOMPARE(existingScript.write("return function() end\n"), 22);
    existingScript.close();
    QSignalSpy existingUploadSpy(vehicle()->ftpManager(), &FTPManager::uploadComplete);
    QVERIFY(vehicle()->ftpManager()->upload(
        MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/APM/scripts/mavlink_bandwidth.lua"), existingScript.fileName()));
    QVERIFY_SIGNAL_WAIT(existingUploadSpy, TestTimeout::longMs());
    QVERIFY(existingUploadSpy.first().at(1).toString().isEmpty());

    MAVLinkBandwidthController controller;
    QSignalSpy deploymentSpy(&controller, &MAVLinkBandwidthController::scriptDeploymentFinished);
    QVERIFY(deploymentSpy.isValid());

    QPointer<MockLink> testMockLink = mockLink();
    controller.installScriptAndReboot(controller.activeVehicleId());
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

    _disconnectMockLink();
    QCOMPARE(controller.transmittedPayloadBytes(), 0U);
    QCOMPARE(controller.receivedPayloadBytes(), 0U);
}

void MAVLinkBandwidthControllerTest::_mavftpCancelRestartTest()
{
    MAVLinkBandwidthController controller;
    controller.setTestMode(MAVLinkBandwidthController::TestMode::MavFtp);
    controller.setFtpFileSizeKiB(64);

    controller.startTest();
    QVERIFY(controller.running());
    controller.stopTest();
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

void MAVLinkBandwidthControllerTest::_mavftpControllerDestructionCleanupTest()
{
    mockLink()->mockLinkFTP()->setBurstReadDelayMs(20);
    auto* controller = new MAVLinkBandwidthController;
    controller->setTestMode(MAVLinkBandwidthController::TestMode::MavFtp);
    controller->setFtpFileSizeKiB(64);
    controller->startTest();
    QVERIFY_TRUE_WAIT(controller->progress() >= 0.5, TestTimeout::longMs());

    delete controller;
    QVERIFY_TRUE_WAIT(mockLink()->mockLinkFTP()->uploadedFiles().isEmpty(), TestTimeout::longMs());
}

UT_REGISTER_TEST(MAVLinkBandwidthControllerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::AnalyzeView)
