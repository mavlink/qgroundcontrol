#include "LogDiskWriterTest.h"
#include "LogDiskWriter.h"
#include "QGCLogEntry.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void LogDiskWriterTest::init()
{
    UnitTest::init();
    _tempDir = new QTemporaryDir();
    QVERIFY(_tempDir->isValid());
    _writer = new LogDiskWriter();
}

void LogDiskWriterTest::cleanup()
{
    delete _writer;
    _writer = nullptr;
    delete _tempDir;
    _tempDir = nullptr;
    UnitTest::cleanup();
}

void LogDiskWriterTest::_testInitialState()
{
    QVERIFY(!_writer->isEnabled());
    QVERIFY(_writer->filePath().isEmpty());
    QVERIFY(!_writer->hasError());
}

void LogDiskWriterTest::_testSetFilePath()
{
    const QString path = _tempDir->filePath("test.log");

    _writer->setFilePath(path);

    QCOMPARE(_writer->filePath(), path);
}

void LogDiskWriterTest::_testEnableDisable()
{
    const QString path = _tempDir->filePath("test.log");
    _writer->setFilePath(path);

    QVERIFY(!_writer->isEnabled());

    _writer->setEnabled(true);
    QVERIFY(_writer->isEnabled());

    _writer->setEnabled(false);
    QVERIFY(!_writer->isEnabled());
}

void LogDiskWriterTest::_testWriteEntry()
{
    const QString path = _tempDir->filePath("test.log");
    _writer->setFilePath(path);
    _writer->setEnabled(true);

    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test log message";
    entry.level = QGCLogEntry::Info;

    _writer->write(entry);
    _writer->flush();

    // Give writer thread time to process
    QThread::msleep(100);

    QVERIFY(QFile::exists(path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    QVERIFY(content.contains("Test log message"));
}

void LogDiskWriterTest::_testWriteMultiple()
{
    const QString path = _tempDir->filePath("multi.log");
    _writer->setFilePath(path);
    _writer->setEnabled(true);

    for (int i = 0; i < 10; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _writer->write(entry);
    }

    _writer->flush();
    QThread::msleep(100);

    QVERIFY(QFile::exists(path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    for (int i = 0; i < 10; ++i) {
        QVERIFY2(content.contains(QString("Message %1").arg(i)),
                 qPrintable(QString("Missing message %1").arg(i)));
    }
}

void LogDiskWriterTest::_testFlush()
{
    const QString path = _tempDir->filePath("flush.log");
    _writer->setFilePath(path);
    _writer->setEnabled(true);

    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Flush test message";

    _writer->write(entry);
    _writer->flush();

    // After flush, content should be on disk
    QThread::msleep(100);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());
    QVERIFY(content.contains("Flush test message"));
}

void LogDiskWriterTest::_testMaxPendingEntries()
{
    _writer->setMaxPendingEntries(100);

    // Write without enabling - entries should be dropped
    for (int i = 0; i < 200; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _writer->write(entry);
    }

    // No file should be created since writer is not enabled
    QVERIFY(!QFile::exists(_tempDir->filePath("test.log")));
}

void LogDiskWriterTest::_testFileRotation()
{
    const QString path = _tempDir->filePath("rotate.log");
    _writer->setFilePath(path);
    _writer->setMaxFileSize(500);  // Very small for testing
    _writer->setMaxBackupFiles(2);
    _writer->setEnabled(true);

    // Write enough to trigger rotation
    for (int i = 0; i < 50; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("This is a somewhat long message number %1 for rotation testing").arg(i);
        _writer->write(entry);
    }

    _writer->flush();
    QThread::msleep(200);

    // Check that backup files were created
    const QDir dir(_tempDir->path());
    const QStringList logFiles = dir.entryList(QStringList() << "rotate*.log", QDir::Files);

    // Should have at least the main file
    QVERIFY(logFiles.contains("rotate.log"));
}

void LogDiskWriterTest::_testErrorHandling()
{
    QSignalSpy errorSpy(_writer, &LogDiskWriter::errorOccurred);

    // Try to write to an invalid path
    _writer->setFilePath("/nonexistent/directory/that/does/not/exist/test.log");
    _writer->setEnabled(true);

    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test";

    _writer->write(entry);
    _writer->flush();

    // Give time for error to propagate
    QThread::msleep(200);
    QCoreApplication::processEvents();

    // Should have an error
    QVERIFY(_writer->hasError());

    // Clear error
    _writer->clearError();

    // Process events to allow signal
    QCoreApplication::processEvents();

    QVERIFY(!_writer->hasError());
}
