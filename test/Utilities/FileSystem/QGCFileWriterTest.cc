#include "QGCFileWriterTest.h"

#ifdef Q_OS_LINUX
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include <atomic>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "QGCFileWriter.h"

#ifdef Q_OS_LINUX
// Whether the kernel has the thread asleep, as it is while it waits for another thread to exit
static bool fileWriterTestThreadSleeps(long kernelThreadId)
{
    return TestFixtures::readFile(QStringLiteral("/proc/self/task/%1/status").arg(kernelThreadId))
        .contains("\nState:\tS");
}
#endif

void QGCFileWriterTest::init()
{
    UnitTest::init();
    _tempDir.emplace();
    QVERIFY(_tempDir->isValid());
    _path = _tempDir->filePath(QStringLiteral("out.log"));
}

void QGCFileWriterTest::cleanup()
{
    _tempDir.reset();
    UnitTest::cleanup();
}

void QGCFileWriterTest::_testFlushWritesQueuedData()
{
    QGCFileWriter writer;
    writer.setFilePath(_path);
    writer.write("first");
    writer.writeDeferred([]() { return QByteArray("second"); });

    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(_path), QByteArray("firstsecond"));
}

void QGCFileWriterTest::_testFlushAfterManyWrites()
{
    QGCFileWriter writer;
    writer.setFilePath(_path);

    // Let the worker finish several batches before the flush under test.
    QByteArray expected;
    for (int i = 0; i < 3; ++i) {
        const QByteArray chunk = QByteArray::number(i) + ';';
        writer.write(chunk);
        expected += chunk;
        QCOMPARE_TRUE_WAIT(writer.fileSize(), qint64(expected.size()), TestTimeout::mediumMs());
    }

    // Large enough that the worker is still writing it when flush() is entered.
    const QByteArray large(8 * 1024 * 1024, 'x');
    expected += large;
    writer.write(large);
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(writer.pendingBytes(), qint64(0));
    QVERIFY(fileContentsEqual(_path, expected));
}

void QGCFileWriterTest::_testConsecutiveFlushes()
{
    QGCFileWriter writer;
    writer.setFilePath(_path);
    writer.write("data");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));

    // Nothing is queued any more: a flush that waits for the timeout returns false.
    QVERIFY(writer.flush(TestTimeout::shortMs()));
    QVERIFY(writer.flush(TestTimeout::shortMs()));
}

void QGCFileWriterTest::_testRecoversAfterError_data()
{
    QTest::addColumn<bool>("writeError");
    QTest::addColumn<bool>("changePath");
    QTest::addColumn<bool>("clearError");

    QTest::newRow("open error, path becomes writable") << false << false << false;
    QTest::newRow("open error, path becomes writable, error cleared") << false << false << true;
    QTest::newRow("open error, new path, error cleared") << false << true << true;
    QTest::newRow("write error, new path, error cleared") << true << true << true;
}

void QGCFileWriterTest::_testRecoversAfterError()
{
    QFETCH(bool, writeError);
    QFETCH(bool, changePath);
    QFETCH(bool, clearError);

    // Opens for writing, but every write fails with "no space left on device"
    const QString fullDevice = QStringLiteral("/dev/full");
    if (writeError && !QFile::exists(fullDevice)) {
        QSKIP("Needs /dev/full to provoke a write error");
    }

    const QString failingPath = writeError ? fullDevice : _tempDir->filePath(QStringLiteral("missing/out.log"));

    QGCFileWriter writer;
    QSignalSpy errorSpy(&writer, &QGCFileWriter::errorOccurred);
    writer.setFilePath(failingPath);
    if (writeError) {
        // Qt's complaint about appending to a character device
        expectLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("Cannot call seek")));
    }
    writer.write("lost");

    QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());
    if (writeError) {
        verifyExpectedLogMessage();
    }
    QVERIFY(writer.hasError());
    QVERIFY(!writer.lastError().isEmpty());
    QVERIFY(!writer.flush(TestTimeout::shortMs()));
    // What could not be written is dropped, not kept for later
    QCOMPARE(writer.pendingBytes(), qint64(0));

    QString goodPath = failingPath;
    if (changePath) {
        goodPath = _path;
        writer.setFilePath(goodPath);
    } else {
        QVERIFY(QDir().mkpath(QFileInfo(failingPath).absolutePath()));
    }
    if (clearError) {
        writer.clearError();
        QVERIFY(!writer.hasError());
    }

    writer.write("recovered");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QVERIFY(!writer.hasError());
    QCOMPARE(TestFixtures::readFile(goodPath), QByteArray("recovered"));
    QCOMPARE(errorSpy.count(), 1);
}

void QGCFileWriterTest::_testCloseReportsWriteError_data()
{
    QTest::addColumn<bool>("flushFirst");

    // Which of the worker's two write paths gets the data depends on timing; the caller sees the same either way.
    // Here the worker usually finds the request to close before it has looked at its queue, and writes what is
    // queued on its way out.
    QTest::newRow("closed right after the first write") << false;
    // Here it is usually waiting for data, and takes the write as a batch before it finds the request to close.
    QTest::newRow("closed after an earlier flush") << true;
}

void QGCFileWriterTest::_testCloseReportsWriteError()
{
    QFETCH(bool, flushFirst);

    // Opens for writing, but every write fails with "no space left on device"
    const QString fullDevice = QStringLiteral("/dev/full");
    if (!QFile::exists(fullDevice)) {
        QSKIP("Needs /dev/full to provoke a write error");
    }

    QGCFileWriter writer;
    QSignalSpy errorSpy(&writer, &QGCFileWriter::errorOccurred);
    writer.setFilePath(fullDevice);
    // Qt's complaint about appending to a character device
    expectLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("Cannot call seek")));
    if (flushFirst) {
        // Opens the file without writing to it
        writer.writeDeferred([]() { return QByteArray(); });
        QVERIFY(writer.flush(TestTimeout::mediumMs()));
    }
    writer.write("lost");
    writer.close();
    verifyExpectedLogMessage();

    QVERIFY(writer.hasError());
    QVERIFY(!writer.lastError().isEmpty());
    QVERIFY(!writer.isRunning());
    QVERIFY(!writer.isOpen());
    QCOMPARE(writer.pendingBytes(), qint64(0));
    QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());

    writer.setFilePath(_path);
    QVERIFY(!writer.hasError());
    writer.write("recovered");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(_path), QByteArray("recovered"));
    QCOMPARE(errorSpy.count(), 1);
}

void QGCFileWriterTest::_testWriteWhileClosingAfterError()
{
#ifdef Q_OS_LINUX
    // Opens for writing, but every write fails with "no space left on device"
    const QString fullDevice = QStringLiteral("/dev/full");
    if (!QFile::exists(fullDevice)) {
        QSKIP("Needs /dev/full to provoke a write error");
    }

    QSemaphore workerExiting;
    QSemaphore releaseWorker;
    QSemaphore written;
    std::atomic<long> closeThreadId{0};

    QGCFileWriter writer;
    QSignalSpy errorSpy(&writer, &QGCFileWriter::errorOccurred);
    const std::unique_ptr<QThread> closeThread(QThread::create([&writer, &closeThreadId]() {
        closeThreadId.store(::syscall(SYS_gettid));
        writer.close();
    }));
    const std::unique_ptr<QThread> writeThread(QThread::create([&writer, &written]() {
        writer.write("kept;");
        written.release();
    }));
    // Runs before the writer is destroyed, which waits for its worker.
    const auto releaseOnExit = qScopeGuard([&releaseWorker, &closeThread, &writeThread]() {
        releaseWorker.release();
        (void) closeThread->wait();
        (void) writeThread->wait();
    });

    // The formatter runs in the worker, whose write fails. It has the worker wait on its way out, with its error
    // set, until the test lets it go.
    writer.setFilePath(fullDevice);
    // Qt's complaint about appending to a character device
    expectLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("Cannot call seek")));
    writer.writeDeferred([&workerExiting, &releaseWorker]() {
        QThread* const worker = QThread::currentThread();
        (void) QObject::connect(
            worker, &QThread::finished, worker,
            [&workerExiting, &releaseWorker]() {
                workerExiting.release();
                releaseWorker.acquire();
            },
            Qt::DirectConnection);
        return QByteArray("lost;");
    });
    QVERIFY(workerExiting.tryAcquire(1, TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    QVERIFY(writer.hasError());

    // close() waits for the worker to exit, and does not hold the mutex while it waits. Nothing else can make its
    // thread sleep here.
    closeThread->start();
    QTRY_VERIFY_WITH_TIMEOUT(fileWriterTestThreadSleeps(closeThreadId.load()), TestTimeout::mediumMs());

    // write() finds the error meanwhile. It used to join the worker which close() was waiting for and to start
    // another, which close() then took for the one it had stopped.
    writeThread->start();
    QVERIFY2(written.tryAcquire(1, TestTimeout::mediumMs()),
             "write() waited for the worker which close() was waiting for");
    releaseWorker.release();
    QVERIFY(closeThread->wait(TestTimeout::mediumMs()));

    // close() has stopped the only worker there was. What was written meanwhile waits for the next one.
    QVERIFY(!writer.isRunning());
    QVERIFY(!writer.hasError());
    QCOMPARE(writer.pendingBytes(), qint64(5));

    writer.setFilePath(_path);
    writer.write("next;");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(_path), QByteArray("kept;next;"));
    QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(errorSpy.count(), 1);
#else
    QSKIP("Needs /dev/full to provoke a write error, and /proc to see that close() is waiting");
#endif
}

void QGCFileWriterTest::_testDestructorWritesQueuedData()
{
    QByteArray expected;
    {
        QGCFileWriter writer;
        writer.setFilePath(_path);
        for (int i = 0; i < 100; ++i) {
            const QByteArray chunk = QByteArray::number(i) + ';';
            writer.write(chunk);
            expected += chunk;
        }
    }

    QCOMPARE(TestFixtures::readFile(_path), expected);
}

void QGCFileWriterTest::_testCloseWritesQueuedDataAndNextWriteReopens()
{
    const QString rotatedPath = _tempDir->filePath(QStringLiteral("out.1.log"));

    QGCFileWriter writer;
    writer.setFilePath(_path);
    QByteArray expected;
    for (int i = 0; i < 100; ++i) {
        const QByteArray chunk = QByteArray::number(i) + ';';
        writer.write(chunk);
        expected += chunk;
    }
    writer.close();

    QCOMPARE(TestFixtures::readFile(_path), expected);
    QVERIFY(!writer.isRunning());
    QVERIFY(!writer.isOpen());
    QCOMPARE(writer.pendingBytes(), qint64(0));

    // LogManager rotates its log this way: close, rename the file, write on to the same path.
    QVERIFY(QFile::rename(_path, rotatedPath));
    writer.write("next;");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(_path), QByteArray("next;"));
    QCOMPARE(TestFixtures::readFile(rotatedPath), expected);
}

void QGCFileWriterTest::_testChangingPathSwitchesFile()
{
    const QString firstPath = _tempDir->filePath(QStringLiteral("first.log"));
    const QString secondPath = _tempDir->filePath(QStringLiteral("second.log"));

    QGCFileWriter writer;
    writer.setFilePath(firstPath);
    writer.write("one;");
    // What is queued for the first file is written to it before the path changes.
    writer.setFilePath(secondPath);
    QCOMPARE(TestFixtures::readFile(firstPath), QByteArray("one;"));

    writer.write("two;");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(secondPath), QByteArray("two;"));

    // An empty path turns writing off.
    writer.setFilePath(QString());
    QVERIFY(!writer.isRunning());
    writer.write("three;");
    QVERIFY(writer.flush(TestTimeout::shortMs()));
    QVERIFY(!writer.isRunning());
    QVERIFY(!writer.hasError());
    QCOMPARE(TestFixtures::readFile(firstPath), QByteArray("one;"));
    QCOMPARE(TestFixtures::readFile(secondPath), QByteArray("two;"));
}

void QGCFileWriterTest::_testAppendsAndReportsFileSize()
{
    QVERIFY(TestFixtures::writeFile(_path, "old;"));

    QGCFileWriter writer;
    QSignalSpy sizeSpy(&writer, &QGCFileWriter::fileSizeChanged);
    writer.setFilePath(_path);
    writer.write("new;");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(_path), QByteArray("old;new;"));

    // LogManager rotates on this size, so it has to count what the file already held.
    QCOMPARE(writer.fileSize(), qint64(8));
    QVERIFY_SIGNAL_COUNT_WAIT(sizeSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(sizeSpy.constLast().constFirst().toLongLong(), qint64(8));
}

void QGCFileWriterTest::_testWritesKeepQueueOrder()
{
    QSemaphore workerHeld;
    QSemaphore releaseWorker;
    QGCFileWriter writer;
    // Runs before the writer is destroyed, which waits for its worker.
    const auto releaseOnExit = qScopeGuard([&releaseWorker]() { releaseWorker.release(); });
    writer.setFilePath(_path);

    // Holds the worker in its first batch, so that everything else is queued behind it.
    writer.writeDeferred([&workerHeld, &releaseWorker]() {
        workerHeld.release();
        releaseWorker.acquire();
        return QByteArray("0;");
    });
    QVERIFY(workerHeld.tryAcquire(1, TestTimeout::mediumMs()));

    QByteArray expected("0;");
    std::atomic<int> formatted{0};
    for (int i = 1; i <= 50; ++i) {
        const QByteArray chunk = QByteArray::number(i) + ';';
        if ((i % 2) == 0) {
            writer.write(chunk);
        } else {
            writer.writeDeferred([chunk, &formatted]() {
                ++formatted;
                return chunk;
            });
        }
        expected += chunk;
    }
    releaseWorker.release();

    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(_path), expected);
    QCOMPARE(formatted.load(), 25);
}

void QGCFileWriterTest::_testWritesOverPendingLimitAreDropped()
{
    QSemaphore workerHeld;
    QSemaphore releaseWorker;
    QGCFileWriter writer;
    // Runs before the writer is destroyed, which waits for its worker.
    const auto releaseOnExit = qScopeGuard([&releaseWorker]() { releaseWorker.release(); });
    writer.setFilePath(_path);
    writer.setMaxPendingBytes(10);

    // Holds the worker, so that the writes below stay pending.
    writer.writeDeferred([&workerHeld, &releaseWorker]() {
        workerHeld.release();
        releaseWorker.acquire();
        return QByteArray();
    });
    QVERIFY(workerHeld.tryAcquire(1, TestTimeout::mediumMs()));

    // A write is refused while more than the limit is waiting to be written.
    writer.write("12345678");
    writer.write("abcdefgh");
    QCOMPARE(writer.pendingBytes(), qint64(16));
    writer.write("dropped!");
    QCOMPARE(writer.pendingBytes(), qint64(16));

    releaseWorker.release();
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(writer.pendingBytes(), qint64(0));
    QCOMPARE(TestFixtures::readFile(_path), QByteArray("12345678abcdefgh"));

    // With the backlog written, writes are accepted again.
    writer.write("more");
    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(TestFixtures::readFile(_path), QByteArray("12345678abcdefghmore"));
}

void QGCFileWriterTest::_testWriteWithoutPathIsDropped_data()
{
    QTest::addColumn<bool>("deferred");

    QTest::newRow("write") << false;
    QTest::newRow("deferred write") << true;
}

void QGCFileWriterTest::_testWriteWithoutPathIsDropped()
{
    QFETCH(bool, deferred);

    QGCFileWriter writer;
    if (deferred) {
        writer.writeDeferred([]() { return QByteArray("data"); });
    } else {
        writer.write("data");
    }

    QVERIFY(writer.flush(TestTimeout::shortMs()));
    QVERIFY(!writer.isRunning());
    QVERIFY(!writer.hasError());
}

UT_REGISTER_TEST(QGCFileWriterTest, TestLabel::Unit, TestLabel::Utilities)
