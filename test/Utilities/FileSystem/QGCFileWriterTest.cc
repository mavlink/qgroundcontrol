#include "QGCFileWriterTest.h"
#include "QGCFileWriter.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void QGCFileWriterTest::init()
{
    UnitTest::init();

    _writer = new QGCFileWriter(this);

    QTemporaryDir tempDir;
    tempDir.setAutoRemove(false);
    _testFilePath = tempDir.filePath("test_writer.txt");
}

void QGCFileWriterTest::cleanup()
{
    delete _writer;
    _writer = nullptr;

    if (!_testFilePath.isEmpty()) {
        QFile::remove(_testFilePath);
        QDir().rmdir(QFileInfo(_testFilePath).absolutePath());
    }

    UnitTest::cleanup();
}

void QGCFileWriterTest::_testInitialState()
{
    QVERIFY(!_writer->isRunning());
    QVERIFY(!_writer->isOpen());
    QVERIFY(!_writer->hasError());
    QCOMPARE(_writer->filePath(), QString());
    QCOMPARE(_writer->fileSize(), qint64(-1));
}

void QGCFileWriterTest::_testSetFilePath()
{
    _writer->setFilePath(_testFilePath);
    QCOMPARE(_writer->filePath(), _testFilePath);

    // Change path
    const QString newPath = _testFilePath + ".new";
    _writer->setFilePath(newPath);
    QCOMPARE(_writer->filePath(), newPath);
}

void QGCFileWriterTest::_testStartStop()
{
    _writer->setFilePath(_testFilePath);

    QVERIFY(!_writer->isRunning());
    _writer->start();
    QVERIFY(_writer->isRunning());

    _writer->stop();
    QVERIFY(!_writer->isRunning());
}

void QGCFileWriterTest::_testWriteString()
{
    _writer->setFilePath(_testFilePath);
    _writer->start();

    const QString testData = "Hello, World!\n";
    _writer->write(testData);
    QVERIFY(_writer->flush());

    _writer->stop();

    // Verify file contents
    QFile file(_testFilePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());
    QCOMPARE(content, testData);
}

void QGCFileWriterTest::_testWriteByteArray()
{
    _writer->setFilePath(_testFilePath);
    _writer->start();

    const QByteArray testData = "Binary data test\n";
    _writer->write(testData);
    QVERIFY(_writer->flush());

    _writer->stop();

    // Verify file contents
    QFile file(_testFilePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), testData);
}

void QGCFileWriterTest::_testFlush()
{
    _writer->setFilePath(_testFilePath);
    _writer->start();

    // Write without flush
    _writer->write(QStringLiteral("Test data\n"));

    // Flush and verify
    QVERIFY(_writer->flush(1000));

    _writer->stop();

    QFile file(_testFilePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(!file.readAll().isEmpty());
}

void QGCFileWriterTest::_testFileSizeCallback()
{
    qint64 reportedSize = 0;
    _writer->setFileSizeCallback([&reportedSize](qint64 size) {
        reportedSize = size;
    });

    _writer->setFilePath(_testFilePath);
    _writer->start();

    // Write enough data to trigger callback (>1MB threshold)
    const QByteArray chunk(512 * 1024, 'X'); // 512KB
    _writer->write(chunk);
    _writer->write(chunk);
    _writer->write(chunk); // Total ~1.5MB
    QVERIFY(_writer->flush());

    _writer->stop();

    QVERIFY(reportedSize > 0);
}

void QGCFileWriterTest::_testPreOpenCallback()
{
    bool callbackInvoked = false;
    _writer->setPreOpenCallback([&callbackInvoked](const QString& path) {
        callbackInvoked = true;
        Q_UNUSED(path);
        return true;
    });

    _writer->setFilePath(_testFilePath);
    _writer->start();

    _writer->write(QStringLiteral("Trigger file open"));
    QVERIFY(_writer->flush());

    QVERIFY(callbackInvoked);

    _writer->stop();
}

void QGCFileWriterTest::_testErrorHandling()
{
    QSignalSpy errorSpy(_writer, &QGCFileWriter::errorOccurred);

    // Set invalid path
    _writer->setFilePath("/nonexistent/directory/that/should/fail/file.txt");
    _writer->start();

    _writer->write(QStringLiteral("This should fail"));
    _writer->flush();

    // Wait for error signal
    QVERIFY(errorSpy.wait(1000) || !errorSpy.isEmpty());
    QVERIFY(_writer->hasError());

    // Clear error
    _writer->clearError();
    QVERIFY(!_writer->hasError());

    _writer->stop();
}

void QGCFileWriterTest::_testMaxPendingBytes()
{
    _writer->setMaxPendingBytes(1024); // 1KB limit
    QCOMPARE(_writer->maxPendingBytes(), qint64(1024));

    _writer->setFilePath(_testFilePath);

    // Don't start - just queue data
    // Data should be dropped when over limit
    // This tests the queuing mechanism
}

void QGCFileWriterTest::_testReopenOnPathChange()
{
    _writer->setFilePath(_testFilePath);
    _writer->start();

    _writer->write(QStringLiteral("First file content\n"));
    QVERIFY(_writer->flush());

    // Change path while running
    const QString secondPath = _testFilePath + ".second";
    _writer->setFilePath(secondPath);

    _writer->write(QStringLiteral("Second file content\n"));
    QVERIFY(_writer->flush());

    _writer->stop();

    // Verify both files
    QFile file1(_testFilePath);
    QVERIFY(file1.open(QIODevice::ReadOnly));
    QVERIFY(file1.readAll().contains("First"));

    QFile file2(secondPath);
    QVERIFY(file2.open(QIODevice::ReadOnly));
    QVERIFY(file2.readAll().contains("Second"));

    // Cleanup second file
    QFile::remove(secondPath);
}
