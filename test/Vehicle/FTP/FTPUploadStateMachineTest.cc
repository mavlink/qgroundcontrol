#include "FTPUploadStateMachineTest.h"
#include "FTPUploadStateMachine.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void FTPUploadStateMachineTest::_testCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPUploadStateMachine stateMachine(_vehicle, this);

    QVERIFY(!stateMachine.isOperationInProgress());
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    _disconnectMockLink();
}

void FTPUploadStateMachineTest::_testMachineName()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPUploadStateMachine stateMachine(_vehicle, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("FTPUpload"));

    _disconnectMockLink();
}

void FTPUploadStateMachineTest::_testStart()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPUploadStateMachine stateMachine(_vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &FTPUploadStateMachine::uploadComplete);

    // Create a temporary file to upload
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test content for upload");
    tempFile.close();

    bool started = stateMachine.upload(MAV_COMP_ID_AUTOPILOT1, tempFile.fileName(), "/test/uploaded.txt");
    QVERIFY(started);
    QVERIFY(stateMachine.isOperationInProgress());

    // Clean up
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPUploadStateMachineTest::_testCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPUploadStateMachine stateMachine(_vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &FTPUploadStateMachine::uploadComplete);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test content for upload");
    tempFile.close();

    stateMachine.upload(MAV_COMP_ID_AUTOPILOT1, tempFile.fileName(), "/test/uploaded.txt");
    QVERIFY(stateMachine.isOperationInProgress());

    stateMachine.cancel();

    QVERIFY(completeSpy.count() > 0 || completeSpy.wait(100));
    QVERIFY(!stateMachine.isOperationInProgress());

    // Verify the signal arguments
    QList<QVariant> arguments = completeSpy.takeFirst();
    QCOMPARE(arguments[1].toString(), QStringLiteral("Aborted"));

    _disconnectMockLink();
}

void FTPUploadStateMachineTest::_testDoubleStartPrevented()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPUploadStateMachine stateMachine(_vehicle, this);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test content");
    tempFile.close();

    // Start first operation
    bool started1 = stateMachine.upload(MAV_COMP_ID_AUTOPILOT1, tempFile.fileName(), "/test/file1.txt");
    QVERIFY(started1);
    QVERIFY(stateMachine.isOperationInProgress());

    // Second start should fail
    bool started2 = stateMachine.upload(MAV_COMP_ID_AUTOPILOT1, tempFile.fileName(), "/test/file2.txt");
    QVERIFY(!started2);

    // Clean up
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPUploadStateMachineTest::_testComponentIdHandling()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPUploadStateMachine stateMachine(_vehicle, this);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test content");
    tempFile.close();

    // MAV_COMP_ID_ALL should be converted to MAV_COMP_ID_AUTOPILOT1
    stateMachine.upload(MAV_COMP_ID_ALL, tempFile.fileName(), "/test/file.txt");
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPUploadStateMachineTest::_testInvalidLocalFile()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPUploadStateMachine stateMachine(_vehicle, this);

    // Try to upload a non-existent file
    bool started = stateMachine.upload(MAV_COMP_ID_AUTOPILOT1, "/nonexistent/file.txt", "/test/file.txt");
    QVERIFY(!started);
    QVERIFY(!stateMachine.isOperationInProgress());

    _disconnectMockLink();
}
