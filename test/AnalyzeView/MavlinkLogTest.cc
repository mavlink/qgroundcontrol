#include "MavlinkLogTest.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

void MavlinkLogTest::init()
{
    UnitTest::init();
    // Make sure temp directory is clear of mavlink logs
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    for (const QString& logFile : logFiles) {
        bool success = tmpDir.remove(logFile);
        Q_UNUSED(success);
        Q_ASSERT(success);
    }
}

void MavlinkLogTest::cleanup()
{
    // Make sure no left over logs in temp directory
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
    UnitTest::cleanup();
}

void MavlinkLogTest::_createTempLogFile(bool zeroLength)
{
    const QString templatePath =
        QDir::tempPath() + QStringLiteral("/%1.%2").arg(_tempLogFileTemplate, _logFileExtension);
    QTemporaryFile tempLogFile(templatePath);
    tempLogFile.setAutoRemove(false);
    if (tempLogFile.open()) {
        if (!zeroLength) {
            tempLogFile.write("foo");
        }
        tempLogFile.close();
    }
}

void MavlinkLogTest::_bootLogDetectionCancel_test()
{
    // Create a fake mavlink log
    _createTempLogFile(false);
    // Kick the protocol to check for lost log files and wait for signals to move through
    connect(this, &MavlinkLogTest::checkForLostLogFiles, MAVLinkProtocol::instance(),
            &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
    QTest::qWait(1000);
}

void MavlinkLogTest::_bootLogDetectionSave_test()
{
    // Create a fake mavlink log
    _createTempLogFile(false);
    // We should get a message box, followed by a getSaveFileName dialog.
    QDir logSaveDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    // Kick the protocol to check for lost log files and wait for signals to move through
    connect(this, &MavlinkLogTest::checkForLostLogFiles, MAVLinkProtocol::instance(),
            &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
    QTest::qWait(1000);
    // Make sure the file is there and delete it
    QCOMPARE(logSaveDir.remove(_saveLogFilename), true);
}

void MavlinkLogTest::_bootLogDetectionZeroLength_test()
{
    // Create a fake empty mavlink log
    _createTempLogFile(true);
    // Kick the protocol to check for lost log files and wait for signals to move through
    connect(this, &MavlinkLogTest::checkForLostLogFiles, MAVLinkProtocol::instance(),
            &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
    QTest::qWait(1000);
    // Zero length log files should not generate any additional UI pop-ups. It should just be deleted silently.
}

void MavlinkLogTest::_connectLogWorker(bool arm)
{
    _connectMockLink();
    QDir logSaveDir;
    if (arm) {
        MultiVehicleManager::instance()->activeVehicle()->setArmedShowError(true);
        QTest::qWait(500);  // Wait long enough for heartbeat to come through
        // On Disconnect: We should get a getSaveFileName dialog.
        logSaveDir.setPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    }
    _disconnectMockLink();
    if (arm) {
        // Make sure the file is there and delete it
        QCOMPARE(logSaveDir.remove(_saveLogFilename), true);
    }
}

void MavlinkLogTest::_connectLogNoArm_test()
{
    _connectLogWorker(false);
}

void MavlinkLogTest::_connectLogArm_test()
{
    _connectLogWorker(true);
}

void MavlinkLogTest::_deleteTempLogFiles_test()
{
    // Verify that the MAVLinkProtocol::deleteTempLogFiles api works correctly
    _createTempLogFile(false);
    MAVLinkProtocol::deleteTempLogFiles();
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
}

UT_REGISTER_TEST(MavlinkLogTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
