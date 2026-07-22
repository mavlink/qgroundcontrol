#include "FTPControllerTest.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>

#include "FTPController.h"
#include "FTPManager.h"
#include "FTPManagerJob.h"
#include "MockLinkFTP.h"
#include "QGCArchiveModel.h"
#include "UnitTest.h"
#include "Vehicle.h"

void FTPControllerTest::_extractArchiveRejectsInvalidInput()
{
    FTPController controller(this);

    QVERIFY(!controller.extractArchive(QString()));
    QVERIFY(controller.errorString().contains(QStringLiteral("No archive path specified")));

    QVERIFY(!controller.extractArchive(QStringLiteral("/nonexistent/archive.zip")));
    QVERIFY(controller.errorString().contains(QStringLiteral("Archive file not found")));
}

void FTPControllerTest::_extractArchiveHappyPath()
{
    FTPController controller(this);
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());

    QSignalSpy completeSpy(&controller, &FTPController::extractionComplete);
    QSignalSpy extractingSpy(&controller, &FTPController::extractingChanged);
    QVERIFY(completeSpy.isValid());
    QVERIFY(extractingSpy.isValid());

    QVERIFY(controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), outputDir.path()));
    QVERIFY(controller.extracting());

    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::longMs());
    QCOMPARE(completeSpy.count(), 1);
    const QList<QVariant> args = completeSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), outputDir.path());
    QVERIFY(args.at(1).toString().isEmpty());

    QVERIFY(QFile::exists(outputDir.path() + QStringLiteral("/manifest.json")));
    QVERIFY(!controller.extracting());
    QVERIFY(controller.errorString().isEmpty());
    QVERIFY(extractingSpy.count() >= 2);
}

void FTPControllerTest::_extractArchiveConcurrentRejected()
{
    FTPController controller(this);
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());

    QVERIFY(controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), outputDir.path()));
    QVERIFY(controller.extracting());

    QVERIFY(!controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), outputDir.path()));
    QVERIFY(controller.errorString().contains(QStringLiteral("Extraction already in progress")));

    QSignalSpy completeSpy(&controller, &FTPController::extractionComplete);
    QVERIFY(completeSpy.isValid());
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::longMs());
    QVERIFY(!controller.extracting());
}

void FTPControllerTest::_extractArchiveReentrantCompletion()
{
    FTPController controller(this);
    QTemporaryDir firstOutputDir;
    QTemporaryDir secondOutputDir;
    QVERIFY(firstOutputDir.isValid());
    QVERIFY(secondOutputDir.isValid());

    QSignalSpy completeSpy(&controller, &FTPController::extractionComplete);
    bool secondStartAttempted = false;
    bool secondStartAccepted = false;
    connect(&controller, &FTPController::extractionComplete, this, [&](const QString&, const QString& error) {
        if (!secondStartAttempted && error.isEmpty()) {
            secondStartAttempted = true;
            secondStartAccepted =
                controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), secondOutputDir.path());
        }
    });

    QVERIFY(controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), firstOutputDir.path()));
    QVERIFY_SIGNAL_COUNT_WAIT(completeSpy, 2, TestTimeout::longMs());

    QVERIFY(secondStartAttempted);
    QVERIFY2(secondStartAccepted, qPrintable(controller.errorString()));
    QCOMPARE(completeSpy.at(0).at(0).toString(), firstOutputDir.path());
    QVERIFY(completeSpy.at(0).at(1).toString().isEmpty());
    QCOMPARE(completeSpy.at(1).at(0).toString(), secondOutputDir.path());
    QVERIFY(completeSpy.at(1).at(1).toString().isEmpty());
    QVERIFY(QFile::exists(firstOutputDir.filePath(QStringLiteral("manifest.json"))));
    QVERIFY(QFile::exists(secondOutputDir.filePath(QStringLiteral("manifest.json"))));
    QVERIFY(!controller.extracting());
    QVERIFY(controller.errorString().isEmpty());
}

void FTPControllerTest::_browseArchiveRejectsInvalidInput()
{
    FTPController controller(this);

    QVERIFY(!controller.browseArchive(QString()));
    QVERIFY(controller.errorString().contains(QStringLiteral("No archive path specified")));

    QVERIFY(!controller.browseArchive(QStringLiteral("/nonexistent/archive.zip")));
    QVERIFY(controller.errorString().contains(QStringLiteral("Archive file not found")));

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("not an archive");
    tempFile.flush();

    QVERIFY(!controller.browseArchive(tempFile.fileName()));
    QVERIFY(controller.errorString().contains(QStringLiteral("Not a supported archive format")));
}

void FTPControllerTest::_browseArchiveHappyPath()
{
    FTPController controller(this);

    QVERIFY(controller.browseArchive(QStringLiteral(":/unittest/manifest.json.zip")));
    QVERIFY(controller.errorString().isEmpty());
    QCOMPARE(controller.archiveModel()->archivePath(), QStringLiteral(":/unittest/manifest.json.zip"));
    QVERIFY(controller.archiveModel()->count() > 0);
    QVERIFY(controller.archiveModel()->contains(QStringLiteral("manifest.json")));
}

void FTPControllerTest::_noVehicleRejectsOperations()
{
    FTPController controller(this);
    controller.setVehicle(nullptr);

    QCOMPARE(controller.vehicle(), nullptr);
    QVERIFY(!controller.listDirectory(QStringLiteral("/")));
    QVERIFY(controller.errorString().contains(QStringLiteral("No active vehicle")));

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    QSignalSpy downloadSpy(&controller, &FTPController::downloadComplete);
    QVERIFY(!controller.downloadFile(QStringLiteral("/general.json"), downloadDir.path()));
    QCOMPARE(downloadSpy.size(), 1);
    QVERIFY(!downloadSpy.takeFirst()[1].toString().isEmpty());

    QTemporaryFile uploadFile;
    QVERIFY(uploadFile.open());
    QSignalSpy uploadSpy(&controller, &FTPController::uploadComplete);
    QVERIFY(!controller.uploadFile(uploadFile.fileName(), QStringLiteral("/upload.bin")));
    QCOMPARE(uploadSpy.size(), 1);
    QVERIFY(!uploadSpy.takeFirst()[1].toString().isEmpty());

    QSignalSpy deleteSpy(&controller, &FTPController::deleteComplete);
    QVERIFY(!controller.deleteFile(QStringLiteral("/delete.bin")));
    QCOMPARE(deleteSpy.size(), 1);
    QVERIFY(!deleteSpy.takeFirst()[1].toString().isEmpty());
    QVERIFY(!controller.busy());
}

void FTPControllerTest::_vehicleChangeCancelsOperation()
{
    QVERIFY(waitForInitialConnect());
    FTPController controller(this);
    QCOMPARE(controller.vehicle(), vehicle());

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    mockLink()->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    QSignalSpy completionSpy(&controller, &FTPController::downloadComplete);
    QSignalSpy vehicleSpy(&controller, &FTPController::vehicleChanged);

    QVERIFY(controller.downloadFile(QStringLiteral("/general.json"), downloadDir.path()));
    QVERIFY(controller.busy());
    QVERIFY(controller.downloadInProgress());
    QPointer<FTPDownloadJob> oldJob = vehicle()->ftpManager()->findChild<FTPDownloadJob*>();
    QVERIFY(oldJob);

    controller.setVehicle(nullptr);

    QCOMPARE(controller.vehicle(), nullptr);
    QCOMPARE(vehicleSpy.size(), 1);
    QCOMPARE(completionSpy.size(), 1);
    QCOMPARE(completionSpy.takeFirst()[1].toString(), QStringLiteral("Aborted"));
    QVERIFY(!controller.busy());
    QVERIFY(!controller.downloadInProgress());

    completionSpy.clear();
    oldJob->finished(QStringLiteral("/stale.bin"), QString(), QStringLiteral("stale warning"));
    QCOMPARE(completionSpy.size(), 0);

    mockLink()->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNone);
}

void FTPControllerTest::_vehicleChangeCompletionDeletesController()
{
    QVERIFY(waitForInitialConnect());
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    mockLink()->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);

    QPointer<FTPController> controller = new FTPController;
    QVERIFY(controller->downloadFile(QStringLiteral("/general.json"), downloadDir.path()));
    connect(
        controller, &FTPController::downloadComplete, this,
        [controller](const QString&, const QString&) { delete controller.data(); }, Qt::DirectConnection);

    controller->setVehicle(nullptr);
    QVERIFY(!controller);
    mockLink()->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNone);
}

void FTPControllerTest::_downloadWarningSurfaced()
{
    QVERIFY(waitForInitialConnect());
    FTPController controller(this);
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    mockLink()->mockLinkFTP()->setResetCommandResponseDropCount(1);
    QSignalSpy completionSpy(&controller, &FTPController::downloadComplete);
    const QString remoteFile = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    QVERIFY(controller.downloadFile(remoteFile, downloadDir.path(), QStringLiteral("warning.bin")));
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());

    const QList<QVariant> arguments = completionSpy.takeFirst();
    QVERIFY(arguments.at(1).toString().isEmpty());
    QVERIFY(controller.errorString().contains(QStringLiteral("resetting remote sessions")));
    QCOMPARE(controller.lastDownloadFile(), downloadDir.filePath(QStringLiteral("warning.bin")));
}

void FTPControllerTest::_directoryTruncationSurfacedAndReset()
{
    FTPController controller(this);
    QSignalSpy truncatedSpy(&controller, &FTPController::directoryTruncatedChanged);
    QVERIFY(truncatedSpy.isValid());

    controller._setOperation(FTPController::Operation::List);
    controller._setBusy(true);
    controller._handleDirectoryComplete({QStringLiteral("Flog.ulg\t123")}, QString(), true);

    QVERIFY(controller.directoryTruncated());
    QCOMPARE(truncatedSpy.size(), 1);
    QVERIFY(controller.errorString().contains(QString::number(FTPController::kMaximumDirectoryEntries)));

    controller._resetDirectoryState();
    QVERIFY(!controller.directoryTruncated());
    QCOMPARE(truncatedSpy.size(), 2);
}

void FTPControllerTest::_completionDirectConnectionDeletion()
{
    QPointer<FTPController> controller = new FTPController;
    controller->_setOperation(FTPController::Operation::Download);
    controller->_setBusy(true);
    connect(controller, &FTPController::busyChanged, controller, [controller]() {
        if (controller && !controller->busy()) {
            delete controller.data();
        }
    });

    controller->_handleDownloadComplete(QStringLiteral("/tmp/completed.bin"), QString(), QString());
    QVERIFY(!controller);
}

UT_REGISTER_TEST(FTPControllerTest, TestLabel::Integration, TestLabel::Vehicle)
