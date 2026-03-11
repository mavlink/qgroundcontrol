#include "FTPControllerTest.h"

#include <QtCore/QFile>
#include <QtTest/QSignalSpy>

#include "FTPController.h"
#include "QGCArchiveModel.h"
#include "UnitTest.h"

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
    QTemporaryDir* const outputDir = createTempDir();
    QVERIFY(outputDir && outputDir->isValid());

    QSignalSpy completeSpy(&controller, &FTPController::extractionComplete);
    QSignalSpy extractingSpy(&controller, &FTPController::extractingChanged);
    QVERIFY(completeSpy.isValid());
    QVERIFY(extractingSpy.isValid());

    QVERIFY(controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), outputDir->path()));
    QVERIFY(controller.extracting());

    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::longMs());
    QCOMPARE(completeSpy.count(), 1);
    const QList<QVariant> args = completeSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), outputDir->path());
    QVERIFY(args.at(1).toString().isEmpty());

    QVERIFY(QFile::exists(outputDir->path() + QStringLiteral("/manifest.json")));
    QVERIFY(!controller.extracting());
    QVERIFY(controller.errorString().isEmpty());
    QVERIFY(extractingSpy.count() >= 2);
}

void FTPControllerTest::_extractArchiveConcurrentRejected()
{
    FTPController controller(this);
    QTemporaryDir* const outputDir = createTempDir();
    QVERIFY(outputDir && outputDir->isValid());

    QVERIFY(controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), outputDir->path()));
    QVERIFY(controller.extracting());

    QVERIFY(!controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), outputDir->path()));
    QVERIFY(controller.errorString().contains(QStringLiteral("Extraction already in progress")));

    QSignalSpy completeSpy(&controller, &FTPController::extractionComplete);
    QVERIFY(completeSpy.isValid());
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::longMs());
    QVERIFY(!controller.extracting());
}

void FTPControllerTest::_browseArchiveRejectsInvalidInput()
{
    FTPController controller(this);

    QVERIFY(!controller.browseArchive(QString()));
    QVERIFY(controller.errorString().contains(QStringLiteral("No archive path specified")));

    QVERIFY(!controller.browseArchive(QStringLiteral("/nonexistent/archive.zip")));
    QVERIFY(controller.errorString().contains(QStringLiteral("Archive file not found")));

    QTemporaryFile* const tempFile = createTempFile();
    QVERIFY(tempFile != nullptr);
    QVERIFY(tempFile->isOpen());
    tempFile->write("not an archive");
    tempFile->flush();

    QVERIFY(!controller.browseArchive(tempFile->fileName()));
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

UT_REGISTER_TEST(FTPControllerTest, TestLabel::Integration, TestLabel::Vehicle)
