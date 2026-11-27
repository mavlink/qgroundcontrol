#include "QGCCompressionJobTest.h"

#include <atomic>
#include <memory>
#include <stdexcept>

#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QSemaphore>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>
#include <QtTest/QSignalSpy>

#include "ArchiveFixtures.h"
#include "Compression.h"
#include "CompressionJob.h"
#include "FileFixtures.h"
#include "ProcessFixtures.h"

namespace {

/// Limit of what the decompression jobs of these tests write, which is a few KiB
constexpr qint64 kCompressionJobTestMaxBytes = 1024 * 1024;

/// What _testDeletedWhileRunning shares with the worker, which may outlive the test function if the test fails
struct QGCCompressionJobTestHeldWorker
{
    QSemaphore held;     ///< Released by the worker once it is in the callback
    QSemaphore proceed;  ///< Lets the worker leave the callback
    std::atomic_int calls{0};
    std::atomic_bool inCallback{false};
};

}  // namespace

void QGCCompressionJobTest::_testExtraction_data()
{
    QTest::addColumn<QByteArray>("archive");
    QTest::addColumn<QString>("onlyEntry");       // Empty: no filter
    QTest::addColumn<bool>("progressContinues");  // What the progress callback of the options returns
    QTest::addColumn<Compression::Error>("expectedError");
    QTest::addColumn<QStringList>("expectedEntries");

    QTest::newRow("all entries") << TestFixtures::readFile(ArchiveFixtures::zipResource) << QString() << true
                                 << Compression::Error::None << QStringList{QStringLiteral("manifest.json")};
    QTest::newRow("filtered") << TestFixtures::readFile(QStringLiteral(":/unittest/tree.zip"))
                              << QStringLiteral("readme.txt") << true << Compression::Error::None
                              << QStringList{QStringLiteral("readme.txt")};
    QTest::newRow("not an archive") << QByteArray("This is not a valid ZIP file") << QString() << true
                                    << Compression::Error::InvalidArchive << QStringList();
    QTest::newRow("cancelled by its progress callback")
        << TestFixtures::readFile(ArchiveFixtures::zipResource) << QString() << false << Compression::Error::Cancelled
        << QStringList();
}

// The job calls back once, on the thread of its context, with the outcome of the extraction, reports its progress
// up to 1.0 on success, and then deletes itself. It passes on the options, also the progress callback, which can
// cancel the extraction
void QGCCompressionJobTest::_testExtraction()
{
    QFETCH(QByteArray, archive);
    QFETCH(QString, onlyEntry);
    QFETCH(bool, progressContinues);
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
    const Compression::ExtractOptions options{
        .filter = filter, .progress = [progressContinues](qint64, qint64) { return progressContinues; }};
    job = CompressionJob::startExtraction(&context, archivePath, outputDir, options,
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
    if (expectedError == Compression::Error::InvalidArchive) {
        QCOMPARE(outcome.message, QStringLiteral("Failed to extract archive: ") + archivePath);
        QVERIFY2(outcome.detail.contains("Unrecognized archive format"), qPrintable(outcome.detail));
    }
    if (!fails) {
        QCOMPARE(outcome.filterName, QStringLiteral("none"));
        QVERIFY(!progressSpy.isEmpty());
        QCOMPARE(progressSpy.last().first().toReal(), 1.0);
        QCOMPARE(progressAtCallback, 1.0);
    } else {
        // Only an extraction that succeeds is reported as done
        QVERIFY(progressSpy.isEmpty() || (progressSpy.last().first().toReal() < 1.0));
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
    const QPointer<CompressionJob> job =
        CompressionJob::startExtraction(&context, QStringLiteral(":/unittest/tree.zip"), outputDir,
                                        {.filter = [standardException](const Compression::ArchiveEntry& entry) -> bool {
                                            // Thrown after a file and a directory have been written
                                            if (entry.name == "data/values.json") {
                                                if (standardException) {
                                                    throw std::runtime_error("refused");
                                                }
                                                throw 42;
                                            }
                                            return true;
                                        }},
                                        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });
    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());

    QCOMPARE(outcomes.size(), 1);
    QCOMPARE(outcomes.first().error, Compression::Error::InternalError);
    // The message is for the user, the exception's own text is the detail
    QCOMPARE(outcomes.first().message, Compression::errorName(Compression::Error::InternalError));
    QCOMPARE(outcomes.first().detail, standardException ? QStringLiteral("refused") : QString());
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
// It writes nothing after a failure, and leaves its input
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
    const QPointer<CompressionJob> job = CompressionJob::startDecompression(
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
        QCOMPARE(outcome.message,
                 QStringLiteral("Decompression failed: %1: the data is larger than the size limit").arg(inputPath));
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{QStringLiteral("data.bin.gz")});
    } else {
        QCOMPARE(outcome.filterName, expectedFilter);
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()),
                 (QStringList{QStringLiteral("data.bin"), QStringLiteral("data.bin.gz")}));
        QCOMPARE(TestFixtures::readFile(outputPath), QByteArray(4096, 'z'));
    }
}

void QGCCompressionJobTest::_testDecompressionProgressOfUnknownSize()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs /dev/zero, an input without a size");
#else
    // /dev/zero has no size and no end: the limit ends the operation. The job reads it through a link
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(QStringLiteral("zeros"));
    QVERIFY(QFile::link(QStringLiteral("/dev/zero"), inputPath));

    QObject context;
    QList<Compression::Outcome> outcomes;
    const QPointer<CompressionJob> job = CompressionJob::startDecompression(
        &context, inputPath, tempDir.filePath(QStringLiteral("zeros.bin")), 4 * 1024 * 1024,
        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });
    QSignalSpy progressSpy(job.data(), &CompressionJob::progressChanged);
    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(outcomes.size(), 1);
    QCOMPARE(outcomes.first().error, Compression::Error::SizeLimitExceeded);
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{QStringLiteral("zeros")});

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

void QGCCompressionJobTest::_testCancelledWhileRunning_data()
{
    QTest::addColumn<bool>("extract");

    QTest::newRow("decompression") << false;
    QTest::newRow("extraction") << true;
}

// A job cancelled while its worker reads the input stops, does not call back and leaves no output
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
        extract ? CompressionJob::startExtraction(&context, inputPath, outputPath, {}, done)
                : CompressionJob::startDecompression(&context, inputPath, outputPath, 0, done);
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
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{inputName});
}

// The destructor of a job returns only once its worker has stopped: no callback of the caller is left running with
// what it captured, and what the operation had written is removed before the caller removes more
void QGCCompressionJobTest::_testDeletedWhileRunning()
{
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("two.zip"));
    const QString outputDir = tempDir.filePath(QStringLiteral("output"));
    QVERIFY(TestFixtures::writeFile(archivePath,
                                    ArchiveFixtures::makeZip({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                                              {.name = "second.txt", .data = "second"}})));

    // The progress callback of the caller holds the worker at its second call, which follows the data of the first
    // file, until the context is being destroyed. Then it cancels the extraction, whose output the worker removes
    const auto worker = std::make_shared<QGCCompressionJobTestHeldWorker>();
    auto context = std::make_unique<QObject>();
    connect(context.get(), &QObject::destroyed, this, [worker]() { worker->proceed.release(); });
    bool calledBack = false;
    const QPointer<CompressionJob> job =
        CompressionJob::startExtraction(context.get(), archivePath, outputDir,
                                        {.progress =
                                             [worker](qint64, qint64) {
                                                 if (++worker->calls != 2) {
                                                     return true;
                                                 }
                                                 worker->inCallback = true;
                                                 worker->held.release();
                                                 worker->proceed.acquire();
                                                 worker->inCallback = false;
                                                 return false;
                                             }},
                                        [&calledBack](const Compression::Outcome&) { calledBack = true; });
    QVERIFY(worker->held.tryAcquire(1, TestTimeout::mediumMs()));
    QVERIFY(QFileInfo::exists(outputDir + QStringLiteral("/first.txt")));

    context.reset();

    // No event is handled in between: the destructor waited for the worker
    QVERIFY(job.isNull());
    QVERIFY(!worker->inCallback);
    QVERIFY(!QFileInfo::exists(outputDir));
    QVERIFY(!calledBack);
}

// The callback may delete the job through its context. The job has let go of the callback by then: destroyed with
// the job, the callback would lose what it captured while it runs
void QGCCompressionJobTest::_testCallbackDeletesContext()
{
    QTemporaryDir tempDir;
    auto context = std::make_unique<QObject>();
    // Held by the test and by the callback
    const auto captured = std::make_shared<int>(0);
    long holdersAfterDelete = 0;
    int calls = 0;
    const QPointer<CompressionJob> job = CompressionJob::startExtraction(
        context.get(), ArchiveFixtures::zipResource, tempDir.filePath(QStringLiteral("output")), {},
        [captured, &context, &holdersAfterDelete, &calls](const Compression::Outcome&) {
            // Read first, for the case that the callback is destroyed while it runs
            std::unique_ptr<QObject>& owner = context;
            long& holders = holdersAfterDelete;
            int& count = calls;
            const std::weak_ptr<int> watched = captured;

            owner.reset();
            holders = watched.use_count();
            ++count;
        });

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(calls, 1);
    QCOMPARE(holdersAfterDelete, 2L);
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

// However a job ends before it has called back, it does not call back. It leaves what its worker had written
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
        blocker = std::make_unique<TestFixtures::ThreadPoolBlocker>(CompressionJob::threadPool());
        QVERIFY(blocker->isBlocking());
    }

    auto context = std::make_unique<QObject>();
    bool calledBack = false;
    const auto done = [&calledBack](const Compression::Outcome&) { calledBack = true; };
    const QPointer<CompressionJob> job =
        extract ? CompressionJob::startExtraction(context.get(), ArchiveFixtures::zipResource, outputPath, {}, done)
                : CompressionJob::startDecompression(context.get(), inputPath, outputPath, kCompressionJobTestMaxBytes,
                                                     done);

    if (workerFinished) {
        // No event is handled here, so the job cannot report what its worker has done
        QVERIFY(CompressionJob::threadPool()->waitForDone(TestTimeout::mediumMs()));
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
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()),
                 workerFinished ? (QStringList{QStringLiteral("data.bin.gz"), QStringLiteral("output")})
                                : QStringList{QStringLiteral("data.bin.gz")});
    }
}

// A job runs on a pool of its own: work that occupies every thread of the global pool, such as QGC::runOnWorker() jobs,
// does not hold it back, and it does not hold them back
void QGCCompressionJobTest::_testRunsBesideGlobalPool()
{
    QTemporaryDir tempDir;
    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());

    QObject context;
    QList<Compression::Outcome> outcomes;
    const QPointer<CompressionJob> job = CompressionJob::startExtraction(
        &context, ArchiveFixtures::zipResource, tempDir.filePath(QStringLiteral("output")), {},
        [&outcomes](const Compression::Outcome& outcome) { outcomes.append(outcome); });

    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(outcomes.size(), 1);
    QVERIFY2(outcomes.first(), qPrintable(outcomes.first().description()));
}

QGC_REGISTER_PORTABLE_TEST(QGCCompressionJobTest, TestLabel::Unit, TestLabel::Utilities)
