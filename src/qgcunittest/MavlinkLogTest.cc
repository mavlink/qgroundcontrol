/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Test for mavlink log collection
///
///     @author Don Gagne <don@thegagnes.com>

#include "MavlinkLogTest.h"
#include "MockLink.h"
#include "QGCTemporaryFile.h"
#include "QGCApplication.h"
#include "UAS.h"
#include "MultiVehicleManager.h"

const char* MavlinkLogTest::_tempLogFileTemplate = "FlightDataXXXXXX"; ///< Template for temporary log file
const char* MavlinkLogTest::_logFileExtension = "mavlink";             ///< Extension for log files
const char* MavlinkLogTest::_saveLogFilename = "qgroundcontrol.mavlink.ut";        ///< Filename to save log files to


MavlinkLogTest::MavlinkLogTest(void)
{
    
}

void MavlinkLogTest::init(void)
{
    UnitTest::init();
    
    // Make sure temp directory is clear of mavlink logs
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    for(const QString &logFile: logFiles) {
        bool success = tmpDir.remove(logFile);
        Q_UNUSED(success);
        Q_ASSERT(success);
    }
}

void MavlinkLogTest::cleanup(void)
{
    // Make sure no left over logs in temp directory
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
    
    UnitTest::cleanup();
}

void MavlinkLogTest::_createTempLogFile(bool zeroLength)
{
    QGCTemporaryFile tempLogFile(QString("%1.%2").arg(_tempLogFileTemplate).arg(_logFileExtension));
    
    tempLogFile.open();
    if (!zeroLength) {
        tempLogFile.write("foo");
    }
    tempLogFile.close();
}

void MavlinkLogTest::_bootLogDetectionCancel_test(void)
{
    // Create a fake mavlink log
    _createTempLogFile(false);
    
    // We should get a message box, followed by a getSaveFileName dialog.
    setExpectedMessageBox(QMessageBox::Ok);
    setExpectedFileDialog(getSaveFileName, QStringList());

    // Kick the protocol to check for lost log files and wait for signals to move through
    connect(this, &MavlinkLogTest::checkForLostLogFiles, qgcApp()->toolbox()->mavlinkProtocol(), &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
    QTest::qWait(1000);
    
    checkExpectedMessageBox();
    checkExpectedFileDialog();
}

void MavlinkLogTest::_bootLogDetectionSave_test(void)
{
    // Create a fake mavlink log
    _createTempLogFile(false);
    
    // We should get a message box, followed by a getSaveFileName dialog.
    setExpectedMessageBox(QMessageBox::Ok);
    QDir logSaveDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QString logSaveFile(logSaveDir.filePath(_saveLogFilename));
    setExpectedFileDialog(getSaveFileName, QStringList(logSaveFile));
    
    // Kick the protocol to check for lost log files and wait for signals to move through
    connect(this, &MavlinkLogTest::checkForLostLogFiles, qgcApp()->toolbox()->mavlinkProtocol(), &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
    QTest::qWait(1000);
    
    checkExpectedMessageBox();
    checkExpectedFileDialog();
    
    // Make sure the file is there and delete it
    QCOMPARE(logSaveDir.remove(_saveLogFilename), true);
}

void MavlinkLogTest::_bootLogDetectionZeroLength_test(void)
{
    // Create a fake empty mavlink log
    _createTempLogFile(true);
    
    // Kick the protocol to check for lost log files and wait for signals to move through
    connect(this, &MavlinkLogTest::checkForLostLogFiles, qgcApp()->toolbox()->mavlinkProtocol(), &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
    QTest::qWait(1000);
    
    // Zero length log files should not generate any additional UI pop-ups. It should just be deleted silently.
}

void MavlinkLogTest::_connectLogWorker(bool arm)
{
    _connectMockLink();

    QDir logSaveDir;
    
    if (arm) {
        qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->setArmed(true);
        QTest::qWait(500); // Wait long enough for heartbeat to come through
        
        // On Disconnect: We should get a getSaveFileName dialog.
        logSaveDir.setPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        QString logSaveFile(logSaveDir.filePath(_saveLogFilename));
        setExpectedFileDialog(getSaveFileName, QStringList(logSaveFile));
    }
    
    _disconnectMockLink();

    if (arm) {
        checkExpectedFileDialog();
    
        // Make sure the file is there and delete it
        QCOMPARE(logSaveDir.remove(_saveLogFilename), true);
    }
}

void MavlinkLogTest::_connectLogNoArm_test(void)
{
    _connectLogWorker(false);
}

void MavlinkLogTest::_connectLogArm_test(void)
{
    _connectLogWorker(true);
}

void MavlinkLogTest::_deleteTempLogFiles_test(void)
{
    // Verify that the MAVLinkProtocol::deleteTempLogFiles api works correctly
    
    _createTempLogFile(false);
    MAVLinkProtocol::deleteTempLogFiles();
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
}
