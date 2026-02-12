#include "FTPControllerTest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "FTPController.h"
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
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());

    QSignalSpy completeSpy(&controller, &FTPController::extractionComplete);
    QSignalSpy extractingSpy(&controller, &FTPController::extractingChanged);
    QVERIFY(completeSpy.isValid());
    QVERIFY(extractingSpy.isValid());

    QVERIFY(controller.extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), outputDir.path()));
    QVERIFY(controller.extracting());

    QVERIFY(completeSpy.wait(10000));
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
    QVERIFY(completeSpy.wait(10000));
    QVERIFY(!controller.extracting());
}

UT_REGISTER_TEST(FTPControllerTest, TestLabel::Integration, TestLabel::Vehicle)
