#include "FTPControllerTest.h"

#include <functional>

#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "FTPController.h"
#include "FTPManager.h"
#include "FileFixtures.h"
#include "MockLink.h"
#include "MockLinkFTP.h"
#include "Vehicle.h"

void FTPControllerTest::_testCancelReportsAbortedOnce_data()
{
    QTest::addColumn<QString>("operation");

    QTest::newRow("list") << "list";
    QTest::newRow("download") << "download";
    QTest::newRow("upload") << "upload";
    QTest::newRow("delete") << "delete";
}

// A cancelled request calls nothing back, so the controller ends the operation itself: as aborted, once, with the
// request gone from the queue of the vehicle, and ready for the next operation
void FTPControllerTest::_testCancelReportsAbortedOnce()
{
    QFETCH(QString, operation);

    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    FTPController controller;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString localFile = directory.filePath(QStringLiteral("upload.bin"));
    QVERIFY(TestFixtures::writeFile(localFile, QByteArray(4 * 1024, 'u')));

    QSignalSpy downloadSpy(&controller, &FTPController::downloadComplete);
    QSignalSpy uploadSpy(&controller, &FTPController::uploadComplete);
    QSignalSpy deleteSpy(&controller, &FTPController::deleteComplete);
    // A listing has no signal of its own
    QSignalSpy* completeSpy = nullptr;
    if (operation == QStringLiteral("list")) {
        QVERIFY(controller.listDirectory(QStringLiteral("/")));
    } else if (operation == QStringLiteral("download")) {
        QVERIFY(controller.downloadFile(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(4 * 1024),
                                        directory.path()));
        completeSpy = &downloadSpy;
    } else if (operation == QStringLiteral("upload")) {
        QVERIFY(controller.uploadFile(localFile, QStringLiteral("/mock/upload/cancelled.bin")));
        completeSpy = &uploadSpy;
    } else {
        QVERIFY(controller.deleteFile(QStringLiteral("/no-such-file")));
        completeSpy = &deleteSpy;
    }
    QVERIFY(controller.busy());

    // Under way on the vehicle
    FTPRequest* const request = controller.findChild<FTPRequest*>();
    QVERIFY(request);
    QSignalSpy startedSpy(request, &FTPRequest::started);
    QVERIFY(UnitTest::waitForSignal(startedSpy, TestTimeout::mediumMs(), QStringLiteral("started")));

    controller.cancelActiveOperation();
    QVERIFY(!controller.busy());
    QCOMPARE(controller.errorString(), QStringLiteral("Aborted"));
    QCOMPARE(ftpManager->requestCount(), 0);
    QCOMPARE(downloadSpy.count() + uploadSpy.count() + deleteSpy.count(), completeSpy ? 1 : 0);
    if (completeSpy) {
        QCOMPARE(completeSpy->first().at(1).toString(), QStringLiteral("Aborted"));
    }

    // The controller takes the next operation, and nothing more comes of the cancelled one
    QVERIFY(controller.listDirectory(QStringLiteral("/")));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), TestTimeout::longMs());
    QCOMPARE(controller.errorString(), QString());
    QCOMPARE(controller.directoryEntries().count(), 6);
    QCOMPARE(downloadSpy.count() + uploadSpy.count() + deleteSpy.count(), completeSpy ? 1 : 0);
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList{QStringLiteral("upload.bin")});
}

// The vehicle may go away during an operation, and its FTPManager with it: the operation ends with the error, once,
// and the next one is refused rather than asked of a manager that is gone
void FTPControllerTest::_testVehicleGoneEndsOperation()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(100);
    FTPController controller;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString uri = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(100 * 1024);

    QSignalSpy downloadSpy(&controller, &FTPController::downloadComplete);
    QVERIFY(controller.downloadFile(uri, directory.path()));
    QTRY_VERIFY_WITH_TIMEOUT(controller.progress() > 0.0F, TestTimeout::longMs());

    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    _disconnectMockLink();

    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), TestTimeout::mediumMs());
    QCOMPARE(downloadSpy.count(), 1);
    QCOMPARE(downloadSpy.first().at(1).toString(), QStringLiteral("Vehicle disconnected"));
    QCOMPARE(controller.errorString(), QStringLiteral("Vehicle disconnected"));
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList());

    expectLogMessage("Vehicle.FTPController", QtWarningMsg, QRegularExpression("Failed to start download"));
    QVERIFY(!controller.downloadFile(uri, directory.path()));
    verifyExpectedLogMessage();
    QCOMPARE(controller.errorString(), QStringLiteral("Failed to download %1").arg(uri));
    QVERIFY(!controller.busy());
    QCOMPARE(downloadSpy.count(), 2);
}

// A controller made while no vehicle is active, as the page that shows it can be, fails every operation with its error
void FTPControllerTest::_testWithoutVehicle()
{
    FTPController controller;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString localFile = directory.filePath(QStringLiteral("upload.bin"));
    QVERIFY(TestFixtures::writeFile(localFile, "data"));
    const QString uri = QStringLiteral("/general.json");

    struct Operation
    {
        const char* name;
        std::function<bool()> start;
        QString warning;
        QString error;
    };

    const QList<Operation> operations = {
        {"list", [&]() { return controller.listDirectory(uri); }, QStringLiteral("Failed to start list operation"),
         QStringLiteral("Failed to list %1").arg(uri)},
        {"download", [&]() { return controller.downloadFile(uri, directory.path()); },
         QStringLiteral("Failed to start download"), QStringLiteral("Failed to download %1").arg(uri)},
        {"upload", [&]() { return controller.uploadFile(localFile, uri); }, QStringLiteral("Failed to start upload"),
         QStringLiteral("Failed to upload upload.bin")},
        {"delete", [&]() { return controller.deleteFile(uri); }, QStringLiteral("Failed to start delete"),
         QStringLiteral("Failed to delete %1").arg(uri)},
    };
    for (const Operation& operation : operations) {
        expectLogMessage("Vehicle.FTPController", QtWarningMsg, QRegularExpression(operation.warning));
        QVERIFY2(!operation.start(), operation.name);
        verifyExpectedLogMessage();
        QVERIFY2(!QTest::currentTestFailed(), operation.name);
        QVERIFY2(controller.errorString() == operation.error, qPrintable(controller.errorString()));
        QVERIFY2(!controller.busy(), operation.name);
    }
}

// The MAVLink FTP browser tells the user that the file name is why a download was refused
void FTPControllerTest::_testNamesRefusedFileName()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPController controller;
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression("Not a file name"));
    expectLogMessage("Vehicle.FTPController", QtWarningMsg, QRegularExpression("Failed to start download"));
    QVERIFY(!controller.downloadFile(QStringLiteral("/general.json"), destination.path(), QStringLiteral("COM1.json")));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(controller.errorString(),
             QStringLiteral("\"COM1.json\" is not a file name that every system can save, so it was not downloaded"));
    QCOMPARE(QDir(destination.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot), QStringList());
    _disconnectMockLink();
}

UT_REGISTER_TEST(FTPControllerTest, TestLabel::Integration, TestLabel::Vehicle)
