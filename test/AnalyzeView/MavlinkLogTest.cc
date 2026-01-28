#include "MavlinkLogTest.h"
#include "TestHelpers.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MAVLinkProtocol.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>

void MavlinkLogTest::init()
{
    UnitTest::init();

    // Make sure temp directory is clear of mavlink logs
    _cleanupTempLogFiles();
}

void MavlinkLogTest::cleanup()
{
    // Clean up any remaining log files
    _cleanupTempLogFiles();

    UnitTest::cleanup();
}

void MavlinkLogTest::_cleanupTempLogFiles()
{
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    for (const QString &logFile : logFiles) {
        tmpDir.remove(logFile);
    }
}

QString MavlinkLogTest::_createTempLogFile(bool zeroLength)
{
    const QString templatePath = QDir::tempPath() + QStringLiteral("/%1.%2")
                                     .arg(_tempLogFileTemplate, _logFileExtension);
    QTemporaryFile tempLogFile(templatePath);
    tempLogFile.setAutoRemove(false);

    QString filePath;
    if (tempLogFile.open()) {
        filePath = tempLogFile.fileName();
        if (!zeroLength) {
            tempLogFile.write("foo");
        }
        tempLogFile.close();
    }
    return filePath;
}

int MavlinkLogTest::_countTempLogFiles()
{
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    return logFiles.count();
}

void MavlinkLogTest::_zeroLengthLogFilesDeleted_test()
{
    // Create a zero-length temp log file
    QString filePath = _createTempLogFile(true /* zeroLength */);
    QVERIFY(!filePath.isEmpty());
    QVERIFY(QFile::exists(filePath));

    // checkForLostLogFiles should delete zero-length files silently
    MAVLinkProtocol::instance()->checkForLostLogFiles();

    // Wait for file operation to complete
    QVERIFY(TestHelpers::waitFor([&]() { return !QFile::exists(filePath); }, TestHelpers::kShortTimeoutMs));
}

void MavlinkLogTest::_deleteTempLogFiles_test()
{
    // Create multiple temp log files
    QString file1 = _createTempLogFile(false);
    QString file2 = _createTempLogFile(true);
    QVERIFY(!file1.isEmpty());
    QVERIFY(!file2.isEmpty());
    QCOMPARE(_countTempLogFiles(), 2);

    // deleteTempLogFiles should remove all temp log files
    MAVLinkProtocol::deleteTempLogFiles();

    // Verify all files are gone
    QCOMPARE(_countTempLogFiles(), 0);
    QVERIFY(!QFile::exists(file1));
    QVERIFY(!QFile::exists(file2));
}

void MavlinkLogTest::_nonZeroLengthLogFileProcessed_test()
{
    // Create a non-zero length temp log file
    QString filePath = _createTempLogFile(false /* zeroLength */);
    QVERIFY(!filePath.isEmpty());
    QVERIFY(QFile::exists(filePath));
    QCOMPARE(_countTempLogFiles(), 1);

    // checkForLostLogFiles processes non-zero files
    // In headless mode without UI, the file may be moved to telemetry save path
    // or deleted if save path is not configured. Either way, temp file should be gone.
    MAVLinkProtocol::instance()->checkForLostLogFiles();

    // Wait for file operation to complete - the temp file should be removed
    // (either deleted or moved to save location)
    QVERIFY(TestHelpers::waitFor([this]() { return _countTempLogFiles() == 0; }, TestHelpers::kShortTimeoutMs));
}
