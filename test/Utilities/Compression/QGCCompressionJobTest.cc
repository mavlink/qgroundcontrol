#include "QGCCompressionJobTest.h"

#include <memory>
#include <stdexcept>

#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>
#include <QtTest/QSignalSpy>

#include "ArchiveFixtures.h"
#include "Compression.h"
#include "CompressionJob.h"
#include "FileFixtures.h"
#include "ProcessFixtures.h"

void QGCCompressionJobTest::_testExtraction_data()
{
    QTest::addColumn<QByteArray>("archive");
    QTest::addColumn<QString>("onlyEntry");  // Empty: no filter
    QTest::addColumn<Compression::Error>("expectedError");
    QTest::addColumn<QStringList>("expectedEntries");

    QTest::newRow("all entries") << TestFixtures::readFile(ArchiveFixtures::zipResource) << QString()
                                 << Compression::Error::None << QStringList{QStringLiteral("manifest.json")};
    QTest::newRow("filtered") << TestFixtures::readFile(QStringLiteral(":/unittest/tree.zip"))
                              << QStringLiteral("readme.txt") << Compression::Error::None
                              << QStringList{QStringLiteral("readme.txt")};
    QTest::newRow("not an archive") << QByteArray("This is not a valid ZIP file") << QString()
                                    << Compression::Error::InvalidArchive << QStringList();
}

// The job calls back once, on the thread of its context, with the outcome of the extraction, reports its progress
// up to 1.0 on success, and then deletes itself
void QGCCompressionJobTest::_testExtraction()
{
    QFETCH(QByteArray, archive);
    QFETCH(QString, onlyEntry);
    QFETCH(Compression::Error, expectedError);
    QFETCH(QStringList, expectedEntries);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("archive.zip"));
    const QString outputDir = tempDir.filePath(QStringLiteral("output"));
    QVERIFY(TestFixtures::writeFile(archivePath, archive));
    const bool fails = (expectedError != Compression::Error::None);

    Compression::EntryFilter filter;
    if (!onlyEntry.isEmpty()) {
        filter = [onlyEntry](const Compression::ArchiveEntry& entry) { return entry.name == onlyEntry; };
    }
    QObject context;
    QList<Compression::Outcome> outcomes;
    QThread* callbackThread = nullptr;
    QPointer<CompressionJob> job;
    qreal progressAtCallback = -1.0;
    job = CompressionJob::startExtraction(&context, archivePath, outputDir, 0, filter,
                                          [&](const Compression::Outcome& outcome) {
                                              outcomes.append(outcome);
                                              callbackThread = QThread::currentThread();
                                              progressAtCallback = job ? job->progress() : -1.0;
                                          });
    QVERIFY(job);
    QCOMPARE(job->parent(), &context);
    QSignalSpy progressSpy(job.data(), &CompressionJob::progressChanged);

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(outcomes.size(), 1);
    QCOMPARE(callbackThread, context.thread());
    const Compression::Outcome& outcome = outcomes.first();
    QCOMPARE(outcome.error, expectedError);
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), expectedEntries);
    if (fails) {
        QCOMPARE(outcome.message, QStringLiteral("Failed to extract archive: ") + archivePath);
        QVERIFY2(outcome.detail.contains("Unrecognized archive format"), qPrintable(outcome.detail));
    } else {
        QCOMPARE(outcome.filterName, QStringLiteral("none"));
        QVERIFY(!progressSpy.isEmpty());
        QCOMPARE(progressSpy.last().first().toReal(), 1.0);
        QCOMPARE(progressAtCallback, 1.0);
    }
}

void QGCCompressionJobTest::_testExtractionFilterThrows_data()
{
    QTest::addColumn<bool>("standardException");

    QTest::newRow("std::exception") << true;
    // Anything can be thrown, also what does not derive from std::exception
    QTest::newRow("another type") << false;
}

// The job reports what an entry filter throws on the worker thread as a failure, and nothing is left
void QGCCompressionJobTest::_testExtractionFilterThrows()
{
    QFETCH(bool, standardException);

    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");

    QObject context;
    QList<Compression::Outcome> outcomes;
    const QPointer<CompressionJob> job = CompressionJob::startExtraction(
        &context, QStringLiteral(":/unittest/tree.zip"), outputDir, 0,
        [standardException](const Compression::ArchiveEntry& entry) -> bool {
            // Thrown after a file and a directory have been written
            if (entry.name == "data/values.json") {
                if (standardException) {
                    throw std::runtime_error("refused");
                }
                throw 42;
            }
            return true;
        },
        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });
    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());

    QCOMPARE(outcomes.size(), 1);
    QCOMPARE(outcomes.first().error, Compression::Error::InternalError);
    if (standardException) {
        QCOMPARE(outcomes.first().message, QStringLiteral("refused"));
    } else {
        QVERIFY(!outcomes.first().message.isEmpty());
    }
    QVERIFY(!QFileInfo::exists(outputDir));
}

void QGCCompressionJobTest::_testDecompression_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<qint64>("maxBytes");
    QTest::addColumn<Compression::Error>("expectedError");
    QTest::addColumn<QString>("expectedFilter");

    const QByteArray content(4096, 'z');
    const QByteArray compressed = TestFixtures::gzipMember(content);
    constexpr qint64 limit = 1024 * 1024;

    QTest::newRow("compressed") << compressed << limit << Compression::Error::None << "gzip";
    // A file that only has the name of a compressed file is copied
    QTest::newRow("not compressed") << content << limit << Compression::Error::None << "none";
    QTest::newRow("over the limit") << compressed << qint64{1024} << Compression::Error::SizeLimitExceeded << QString();
}

// The job decompresses its input within its limit, calls back once on the thread of its context, and deletes itself.
// It consumes its input: the directory then holds the output alone, and nothing after a failure
void QGCCompressionJobTest::_testDecompression()
{
    QFETCH(QByteArray, input);
    QFETCH(qint64, maxBytes);
    QFETCH(Compression::Error, expectedError);
    QFETCH(QString, expectedFilter);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(QStringLiteral("data.bin.gz"));
    const QString outputPath = tempDir.filePath(QStringLiteral("data.bin"));
    QVERIFY(TestFixtures::writeFile(inputPath, input));
    const bool fails = (expectedError != Compression::Error::None);

    QObject context;
    QList<Compression::Outcome> outcomes;
    QThread* callbackThread = nullptr;
    const QPointer<CompressionJob> job = CompressionJob::startDecompressionConsumingInput(
        &context, inputPath, outputPath, maxBytes, [&](const Compression::Outcome& outcome) {
            outcomes.append(outcome);
            callbackThread = QThread::currentThread();
        });
    QVERIFY(job);
    QCOMPARE(job->parent(), &context);

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(outcomes.size(), 1);
    QCOMPARE(callbackThread, context.thread());
    const Compression::Outcome& outcome = outcomes.first();
    QVERIFY2(outcome.error == expectedError, qPrintable(outcome.description()));
    if (fails) {
        // The outcome names the input as the caller gave it
        QCOMPARE(outcome.message, QStringLiteral("Decompression failed: %1: size limit exceeded").arg(inputPath));
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList());
    } else {
        QCOMPARE(outcome.filterName, expectedFilter);
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{QStringLiteral("data.bin")});
        QCOMPARE(TestFixtures::readFile(outputPath), QByteArray(4096, 'z'));
    }
}

void QGCCompressionJobTest::_testDecompressionRefusesOutputPath_data()
{
    QTest::addColumn<QString>("outputName");
    QTest::addColumn<QString>("message");  // %1: the input path

    // The default of Compression::decompressFile() would come from the name the job took the input under
    QTest::newRow("no output path") << QString() << Compression::errorName(Compression::Error::IoError);
    // As Compression::decompressFile() refuses it, also in another spelling of the path
    QTest::newRow("the input") << "./data.bin.gz" << "Cannot decompress a file onto itself: %1";
}

// A job fails without an output path, and with one that names its input. It consumes its input all the same
void QGCCompressionJobTest::_testDecompressionRefusesOutputPath()
{
    QFETCH(QString, outputName);
    QFETCH(QString, message);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(QStringLiteral("data.bin.gz"));
    QVERIFY(TestFixtures::writeFile(inputPath, TestFixtures::gzipMember("data")));

    QObject context;
    QList<Compression::Outcome> outcomes;
    const QPointer<CompressionJob> job = CompressionJob::startDecompressionConsumingInput(
        &context, inputPath, outputName.isEmpty() ? QString() : tempDir.filePath(outputName),
        Compression::kMaxSmallFileBytes,
        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(outcomes.size(), 1);
    QCOMPARE(outcomes.first().error, Compression::Error::IoError);
    QCOMPARE(outcomes.first().message, message.replace(QStringLiteral("%1"), inputPath));
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList());
}

// An input that cannot be renamed, here a resource, is read where it is
void QGCCompressionJobTest::_testDecompressionOfInputThatCannotBeTaken()
{
    QTemporaryDir tempDir;
    const QString outputPath = tempDir.filePath(QStringLiteral("manifest.json"));

    QObject context;
    QList<Compression::Outcome> outcomes;
    const QPointer<CompressionJob> job = CompressionJob::startDecompressionConsumingInput(
        &context, ArchiveFixtures::gzResource, outputPath, Compression::kMaxSmallFileBytes,
        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(outcomes.size(), 1);
    QVERIFY2(outcomes.first(), qPrintable(outcomes.first().description()));
    QVERIFY(TestFixtures::readFile(outputPath).contains("\"name\""));
}

void QGCCompressionJobTest::_testDecompressionProgressOfUnknownSize()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs /dev/zero, an input without a size");
#else
    // /dev/zero has no size and no end: the limit ends the operation. The job consumes a link to it
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(QStringLiteral("zeros"));
    QVERIFY(QFile::link(QStringLiteral("/dev/zero"), inputPath));

    QObject context;
    QList<Compression::Outcome> outcomes;
    const QPointer<CompressionJob> job = CompressionJob::startDecompressionConsumingInput(
        &context, inputPath, tempDir.filePath(QStringLiteral("zeros.bin")), 4 * 1024 * 1024,
        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });
    QSignalSpy progressSpy(job.data(), &CompressionJob::progressChanged);
    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(outcomes.size(), 1);
    QCOMPARE(outcomes.first().error, Compression::Error::SizeLimitExceeded);
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList());

    // Without a total, the progress grows towards one half and stays below it
    QVERIFY(!progressSpy.isEmpty());
    qreal previous = 0.0;
    for (const QList<QVariant>& emission : std::as_const(progressSpy)) {
        const qreal progress = emission.first().toReal();
        QVERIFY2((progress > previous) && (progress < 0.5), qPrintable(QString::number(progress)));
        previous = progress;
    }
#endif
}

void QGCCompressionJobTest::_testDecompressionLeavesNewerInput_data()
{
    QTest::addColumn<bool>("cancel");

    QTest::newRow("completed") << false;
    QTest::newRow("cancelled") << true;
}

// A file written to the path of the input once the job has started, such as the next download of that name, is not
// the job's: the job neither reads nor removes it, however it ends
void QGCCompressionJobTest::_testDecompressionLeavesNewerInput()
{
    QFETCH(bool, cancel);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(QStringLiteral("data.bin.gz"));
    const QString outputPath = tempDir.filePath(QStringLiteral("data.bin"));
    QVERIFY(TestFixtures::writeFile(inputPath, TestFixtures::gzipMember("older")));

    // Holds the worker back until the newer file is there
    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());

    QObject context;
    QList<Compression::Outcome> outcomes;
    const QPointer<CompressionJob> job = CompressionJob::startDecompressionConsumingInput(
        &context, inputPath, outputPath, Compression::kMaxSmallFileBytes,
        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });
    const QByteArray newer = TestFixtures::gzipMember("newer");
    QVERIFY(TestFixtures::writeFile(inputPath, newer));
    if (cancel) {
        job->cancel();
    }
    blocker.release();

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(TestFixtures::readFile(inputPath), newer);
    if (cancel) {
        QVERIFY(outcomes.isEmpty());
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{QStringLiteral("data.bin.gz")});
    } else {
        QCOMPARE(outcomes.size(), 1);
        QVERIFY2(outcomes.first(), qPrintable(outcomes.first().description()));
        QCOMPARE(TestFixtures::readFile(outputPath), QByteArray("older"));
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()),
                 (QStringList{QStringLiteral("data.bin"), QStringLiteral("data.bin.gz")}));
    }
}

void QGCCompressionJobTest::_testCancelledWhileRunning_data()
{
    QTest::addColumn<bool>("extract");

    QTest::newRow("decompression") << false;
    QTest::newRow("extraction") << true;
}

// A job cancelled while its worker reads the input stops, does not call back and leaves no output. A decompression
// leaves no input either: once the job is gone, no file with the compressed content is left, also on Windows, where
// a file that the worker has open cannot be removed
void QGCCompressionJobTest::_testCancelledWhileRunning()
{
    QFETCH(bool, extract);

    QTemporaryDir tempDir;
    const QString inputName = extract ? QStringLiteral("zeros.zip") : QStringLiteral("zeros.gz");
    const QString inputPath = tempDir.filePath(inputName);
    const QString outputPath = tempDir.filePath("output");
    // Expands to 512 MiB, so the worker is still busy when its first progress report is handled here
    constexpr int memberCount = 8192;
    if (extract) {
        // One entry of that size
        QVERIFY(ArchiveFixtures::writeZipOfZeros(inputPath, "zeros.bin", qint64{memberCount} * 64 * 1024));
    } else {
        QVERIFY(ArchiveFixtures::writeGzipOfZeros(inputPath, memberCount));
    }

    QObject context;
    bool calledBack = false;
    const auto done = [&calledBack](const Compression::Outcome&) { calledBack = true; };
    const QPointer<CompressionJob> job =
        extract ? CompressionJob::startExtraction(&context, inputPath, outputPath, 0, nullptr, done)
                : CompressionJob::startDecompressionConsumingInput(&context, inputPath, outputPath, 0, done);
    qreal firstProgress = 0.0;
    connect(job.data(), &CompressionJob::progressChanged, this, [&job, &firstProgress](qreal progress) {
        if (qFuzzyIsNull(firstProgress)) {
            firstProgress = progress;
            job->cancel();
        }
    });

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::longMs());
    // The report arrived while the worker was running, so the cancellation stopped it
    QVERIFY(firstProgress > 0.0);
    QVERIFY(firstProgress < 1.0);
    QVERIFY(!calledBack);
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), extract ? QStringList{inputName} : QStringList());
}

void QGCCompressionJobTest::_testEndedWithoutCallback_data()
{
    QTest::addColumn<bool>("extract");
    QTest::addColumn<bool>("deleteContext");
    QTest::addColumn<bool>("workerFinished");

    for (const bool extract : {false, true}) {
        const char* const operation = extract ? "extraction" : "decompression";
        QTest::addRow("%s cancelled before it starts", operation) << extract << false << false;
        QTest::addRow("%s cancelled after its worker finished", operation) << extract << false << true;
        QTest::addRow("%s, context deleted before it starts", operation) << extract << true << false;
        QTest::addRow("%s, context deleted after its worker finished", operation) << extract << true << true;
    }
}

// However a job ends before it has called back, it does not call back. A decompression then leaves neither its input
// nor its output, also once its worker has written it. An extraction leaves what its worker had extracted
void QGCCompressionJobTest::_testEndedWithoutCallback()
{
    QFETCH(bool, extract);
    QFETCH(bool, deleteContext);
    QFETCH(bool, workerFinished);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(QStringLiteral("data.bin.gz"));
    const QString outputPath = tempDir.filePath(QStringLiteral("output"));
    QVERIFY(TestFixtures::writeFile(inputPath, TestFixtures::gzipMember("data")));

    // Holds the worker back before it starts
    std::unique_ptr<TestFixtures::ThreadPoolBlocker> blocker;
    if (!workerFinished) {
        blocker = std::make_unique<TestFixtures::ThreadPoolBlocker>();
        QVERIFY(blocker->isBlocking());
    }

    auto context = std::make_unique<QObject>();
    bool calledBack = false;
    const auto done = [&calledBack](const Compression::Outcome&) { calledBack = true; };
    const QPointer<CompressionJob> job =
        extract
            ? CompressionJob::startExtraction(context.get(), ArchiveFixtures::zipResource, outputPath, 0, nullptr, done)
            : CompressionJob::startDecompressionConsumingInput(context.get(), inputPath, outputPath,
                                                               Compression::kMaxSmallFileBytes, done);

    if (workerFinished) {
        // No event is handled here, so the job cannot report what its worker has done
        QVERIFY(QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs()));
        QVERIFY(QFileInfo::exists(extract ? outputPath + QStringLiteral("/manifest.json") : outputPath));
    }

    if (deleteContext) {
        context.reset();
        QVERIFY(job.isNull());
    } else {
        job->cancel();
    }
    if (blocker) {
        blocker->release();
    }

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QVERIFY(!calledBack);
    if (extract) {
        QCOMPARE(ArchiveFixtures::entriesBelow(outputPath),
                 workerFinished ? QStringList{QStringLiteral("manifest.json")} : QStringList());
    } else {
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList());
    }
}

QGC_REGISTER_PORTABLE_TEST(QGCCompressionJobTest, TestLabel::Unit, TestLabel::Utilities)
