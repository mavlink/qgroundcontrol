#include "FTPDownloadStateMachineTest.h"
#include "FTPDownloadStateMachine.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void FTPDownloadStateMachineTest::_testCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDownloadStateMachine stateMachine(_vehicle, this);

    QVERIFY(!stateMachine.isOperationInProgress());
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    _disconnectMockLink();
}

void FTPDownloadStateMachineTest::_testMachineName()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDownloadStateMachine stateMachine(_vehicle, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("FTPDownload"));

    _disconnectMockLink();
}

void FTPDownloadStateMachineTest::_testStart()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDownloadStateMachine stateMachine(_vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &FTPDownloadStateMachine::downloadComplete);

    // Use temp directory for download
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    bool started = stateMachine.download(MAV_COMP_ID_AUTOPILOT1, "/test/file.txt", tempDir.path());
    QVERIFY(started);
    QVERIFY(stateMachine.isOperationInProgress());

    // Clean up
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPDownloadStateMachineTest::_testCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDownloadStateMachine stateMachine(_vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &FTPDownloadStateMachine::downloadComplete);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    stateMachine.download(MAV_COMP_ID_AUTOPILOT1, "/test/file.txt", tempDir.path());
    QVERIFY(stateMachine.isOperationInProgress());

    stateMachine.cancel();

    QVERIFY(completeSpy.count() > 0 || completeSpy.wait(100));
    QVERIFY(!stateMachine.isOperationInProgress());

    // Verify the signal arguments
    QList<QVariant> arguments = completeSpy.takeFirst();
    QCOMPARE(arguments[1].toString(), QStringLiteral("Aborted"));

    _disconnectMockLink();
}

void FTPDownloadStateMachineTest::_testDoubleStartPrevented()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDownloadStateMachine stateMachine(_vehicle, this);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Start first operation
    bool started1 = stateMachine.download(MAV_COMP_ID_AUTOPILOT1, "/test/file1.txt", tempDir.path());
    QVERIFY(started1);
    QVERIFY(stateMachine.isOperationInProgress());

    // Second start should fail
    bool started2 = stateMachine.download(MAV_COMP_ID_AUTOPILOT1, "/test/file2.txt", tempDir.path());
    QVERIFY(!started2);

    // Clean up
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPDownloadStateMachineTest::_testComponentIdHandling()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDownloadStateMachine stateMachine(_vehicle, this);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // MAV_COMP_ID_ALL should be converted to MAV_COMP_ID_AUTOPILOT1
    stateMachine.download(MAV_COMP_ID_ALL, "/test/file.txt", tempDir.path());
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPDownloadStateMachineTest::_testInvalidLocalDirectory()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDownloadStateMachine stateMachine(_vehicle, this);

    // Try to download to a non-existent/invalid directory
    bool started = stateMachine.download(MAV_COMP_ID_AUTOPILOT1, "/test/file.txt", "/nonexistent/path/that/does/not/exist");
    QVERIFY(!started);
    QVERIFY(!stateMachine.isOperationInProgress());

    _disconnectMockLink();
}
