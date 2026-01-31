#include "FTPControllerTest.h"
#include "FTPController.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void FTPControllerTest::_testInitialState()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QVERIFY(!controller.busy());
    QVERIFY(!controller.listInProgress());
    QVERIFY(!controller.downloadInProgress());
    QVERIFY(!controller.uploadInProgress());
    QVERIFY(!controller.deleteInProgress());
    QCOMPARE(controller.progress(), 0.0F);
    QVERIFY(controller.errorString().isEmpty());
    QVERIFY(controller.currentPath().isEmpty());
    QVERIFY(controller.directoryEntries().isEmpty());
    QVERIFY(controller.lastDownloadFile().isEmpty());
    QVERIFY(controller.lastUploadTarget().isEmpty());
    QVERIFY(!controller.lastDownloadIsArchive());
    QVERIFY(!controller.extracting());
    QCOMPARE(controller.extractionProgress(), 0.0F);

    _disconnectMockLink();
}

void FTPControllerTest::_testListDirectory()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy busySpy(&controller, &FTPController::busyChanged);
    QSignalSpy operationSpy(&controller, &FTPController::activeOperationChanged);

    bool started = controller.listDirectory(QStringLiteral("mftp://[;comp=1]/"));
    QVERIFY(started);
    QVERIFY(controller.busy());
    QVERIFY(controller.listInProgress());
    QVERIFY(!controller.downloadInProgress());
    QVERIFY(!controller.uploadInProgress());
    QVERIFY(!controller.deleteInProgress());

    QVERIFY(busySpy.count() > 0 || busySpy.wait(100));
    QVERIFY(operationSpy.count() > 0);

    controller.cancelActiveOperation();

    _disconnectMockLink();
}

void FTPControllerTest::_testListDirectoryCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy busySpy(&controller, &FTPController::busyChanged);

    controller.listDirectory(QStringLiteral("mftp://[;comp=1]/"));
    QVERIFY(controller.busy());

    controller.cancelActiveOperation();

    QVERIFY(busySpy.count() > 0 || busySpy.wait(100));
    QVERIFY(!controller.listInProgress());

    _disconnectMockLink();
}

void FTPControllerTest::_testDownloadFile()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy busySpy(&controller, &FTPController::busyChanged);
    QSignalSpy operationSpy(&controller, &FTPController::activeOperationChanged);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    bool started = controller.downloadFile(
        QStringLiteral("mftp://[;comp=1]/test/file.txt"),
        tempDir.path());
    QVERIFY(started);
    QVERIFY(controller.busy());
    QVERIFY(controller.downloadInProgress());
    QVERIFY(!controller.listInProgress());
    QVERIFY(!controller.uploadInProgress());
    QVERIFY(!controller.deleteInProgress());

    QVERIFY(busySpy.count() > 0 || busySpy.wait(100));
    QVERIFY(operationSpy.count() > 0);

    controller.cancelActiveOperation();

    _disconnectMockLink();
}

void FTPControllerTest::_testDownloadFileCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy completeSpy(&controller, &FTPController::downloadComplete);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    controller.downloadFile(
        QStringLiteral("mftp://[;comp=1]/test/file.txt"),
        tempDir.path());
    QVERIFY(controller.busy());

    controller.cancelActiveOperation();

    QVERIFY(completeSpy.count() > 0 || completeSpy.wait(100));
    QVERIFY(!controller.downloadInProgress());

    _disconnectMockLink();
}

void FTPControllerTest::_testUploadFile()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy busySpy(&controller, &FTPController::busyChanged);
    QSignalSpy operationSpy(&controller, &FTPController::activeOperationChanged);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test content for upload");
    tempFile.close();

    bool started = controller.uploadFile(
        tempFile.fileName(),
        QStringLiteral("mftp://[;comp=1]/test/uploaded.txt"));
    QVERIFY(started);
    QVERIFY(controller.busy());
    QVERIFY(controller.uploadInProgress());
    QVERIFY(!controller.listInProgress());
    QVERIFY(!controller.downloadInProgress());
    QVERIFY(!controller.deleteInProgress());

    QVERIFY(busySpy.count() > 0 || busySpy.wait(100));
    QVERIFY(operationSpy.count() > 0);

    controller.cancelActiveOperation();

    _disconnectMockLink();
}

void FTPControllerTest::_testUploadFileCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy completeSpy(&controller, &FTPController::uploadComplete);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test content for upload");
    tempFile.close();

    controller.uploadFile(
        tempFile.fileName(),
        QStringLiteral("mftp://[;comp=1]/test/uploaded.txt"));
    QVERIFY(controller.busy());

    controller.cancelActiveOperation();

    QVERIFY(completeSpy.count() > 0 || completeSpy.wait(100));
    QVERIFY(!controller.uploadInProgress());

    _disconnectMockLink();
}

void FTPControllerTest::_testDeleteFile()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy busySpy(&controller, &FTPController::busyChanged);
    QSignalSpy operationSpy(&controller, &FTPController::activeOperationChanged);

    bool started = controller.deleteFile(QStringLiteral("mftp://[;comp=1]/test/file.txt"));
    QVERIFY(started);
    QVERIFY(controller.busy());
    QVERIFY(controller.deleteInProgress());
    QVERIFY(!controller.listInProgress());
    QVERIFY(!controller.downloadInProgress());
    QVERIFY(!controller.uploadInProgress());

    QVERIFY(busySpy.count() > 0 || busySpy.wait(100));
    QVERIFY(operationSpy.count() > 0);

    controller.cancelActiveOperation();

    _disconnectMockLink();
}

void FTPControllerTest::_testDeleteFileCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QSignalSpy completeSpy(&controller, &FTPController::deleteComplete);

    controller.deleteFile(QStringLiteral("mftp://[;comp=1]/test/file.txt"));
    QVERIFY(controller.busy());

    controller.cancelActiveOperation();

    QVERIFY(completeSpy.count() > 0 || completeSpy.wait(100));
    QVERIFY(!controller.deleteInProgress());

    _disconnectMockLink();
}

void FTPControllerTest::_testConcurrentOperationsPrevented()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test content");
    tempFile.close();

    // Start list operation
    bool started1 = controller.listDirectory(QStringLiteral("mftp://[;comp=1]/"));
    QVERIFY(started1);
    QVERIFY(controller.busy());

    // Concurrent operations should fail
    bool started2 = controller.downloadFile(
        QStringLiteral("mftp://[;comp=1]/test/file.txt"),
        tempDir.path());
    QVERIFY(!started2);

    bool started3 = controller.uploadFile(
        tempFile.fileName(),
        QStringLiteral("mftp://[;comp=1]/test/uploaded.txt"));
    QVERIFY(!started3);

    bool started4 = controller.deleteFile(QStringLiteral("mftp://[;comp=1]/test/file.txt"));
    QVERIFY(!started4);

    bool started5 = controller.listDirectory(QStringLiteral("mftp://[;comp=1]/other"));
    QVERIFY(!started5);

    controller.cancelActiveOperation();

    _disconnectMockLink();
}

void FTPControllerTest::_testInvalidLocalDirectory()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    bool started = controller.downloadFile(
        QStringLiteral("mftp://[;comp=1]/test/file.txt"),
        QStringLiteral("/nonexistent/path/that/does/not/exist"));
    QVERIFY(!started);
    QVERIFY(!controller.busy());
    QVERIFY(!controller.downloadInProgress());

    _disconnectMockLink();
}

void FTPControllerTest::_testInvalidLocalFile()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPController controller;

    bool started = controller.uploadFile(
        QStringLiteral("/nonexistent/file.txt"),
        QStringLiteral("mftp://[;comp=1]/test/uploaded.txt"));
    QVERIFY(!started);
    QVERIFY(!controller.busy());
    QVERIFY(!controller.uploadInProgress());

    _disconnectMockLink();
}
