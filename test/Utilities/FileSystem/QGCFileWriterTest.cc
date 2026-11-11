#include "QGCFileWriterTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "QGCFileWriter.h"

QByteArray QGCFileWriterTest::_fileContents(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

void QGCFileWriterTest::_testFlushWritesQueuedData()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("out.log"));

    QGCFileWriter writer;
    writer.setFilePath(path);
    writer.write("first");
    writer.writeDeferred([]() { return QByteArray("second"); });

    QVERIFY(writer.flush(TestTimeout::mediumMs()));
    QCOMPARE(_fileContents(path), QByteArray("firstsecond"));
}

void QGCFileWriterTest::_testFlushAfterManyWrites()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("out.log"));

    QGCFileWriter writer;
    writer.setFilePath(path);

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
    QVERIFY(_fileContents(path) == expected);
}

void QGCFileWriterTest::_testConsecutiveFlushes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCFileWriter writer;
    writer.setFilePath(tempDir.filePath(QStringLiteral("out.log")));
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

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString failingPath = writeError ? fullDevice : tempDir.filePath(QStringLiteral("missing/out.log"));

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
        goodPath = tempDir.filePath(QStringLiteral("out.log"));
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
    QCOMPARE(_fileContents(goodPath), QByteArray("recovered"));
    QCOMPARE(errorSpy.count(), 1);
}

void QGCFileWriterTest::_testDestructorWritesQueuedData()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("out.log"));

    QByteArray expected;
    {
        QGCFileWriter writer;
        writer.setFilePath(path);
        for (int i = 0; i < 100; ++i) {
            const QByteArray chunk = QByteArray::number(i) + ';';
            writer.write(chunk);
            expected += chunk;
        }
    }

    QCOMPARE(_fileContents(path), expected);
}

UT_REGISTER_TEST(QGCFileWriterTest, TestLabel::Unit, TestLabel::Utilities)
