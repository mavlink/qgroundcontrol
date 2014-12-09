/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @brief Test for mavlink log collection
///
///     @author Don Gagne <don@thegagnes.com>

#include "MavlinkLogTest.h"
#include "MainWindow.h"
#include "MockLink.h"
#include "QGCTemporaryFile.h"

UT_REGISTER_TEST(MavlinkLogTest)

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
    foreach(QString logFile, logFiles) {
        bool success = tmpDir.remove(logFile);
        Q_UNUSED(success);
        Q_ASSERT(success);
    }
}

void MavlinkLogTest::cleanup(void)
{
    UnitTest::cleanup();

    // Make sure no left over logs in temp directory
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
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

    MainWindow* mainWindow = MainWindow::_create(NULL, MainWindow::CUSTOM_MODE_PX4);
    Q_CHECK_PTR(mainWindow);
    
    checkExpectedMessageBox();
    checkExpectedFileDialog();

    mainWindow->close();
    
    delete mainWindow;
}

void MavlinkLogTest::_bootLogDetectionSave_test(void)
{
    // Create a fake mavlink log
    _createTempLogFile(false);
    
    // We should get a message box, followed by a getSaveFileName dialog.
    setExpectedMessageBox(QMessageBox::Ok);
    QDir logSaveDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    QString logSaveFile(logSaveDir.filePath(_saveLogFilename));
    setExpectedFileDialog(getSaveFileName, QStringList(logSaveFile));
    
    MainWindow* mainWindow = MainWindow::_create(NULL, MainWindow::CUSTOM_MODE_PX4);
    Q_CHECK_PTR(mainWindow);
    
    checkExpectedMessageBox();
    checkExpectedFileDialog();
    
    // Make sure the file is there and delete it
    QCOMPARE(logSaveDir.remove(_saveLogFilename), true);
    
    mainWindow->close();
    
    delete mainWindow;
}

void MavlinkLogTest::_bootLogDetectionZeroLength_test(void)
{
    // Create a fake empty mavlink log
    _createTempLogFile(true);
    
    // Zero length log files should not generate any additional UI pop-ups. It should just be deleted silently.
    MainWindow* mainWindow = MainWindow::_create(NULL, MainWindow::CUSTOM_MODE_PX4);
    Q_CHECK_PTR(mainWindow);
    
    mainWindow->close();
    
    delete mainWindow;
}

void MavlinkLogTest::_connectLog_test(void)
{
    MainWindow* mainWindow = MainWindow::_create(NULL, MainWindow::CUSTOM_MODE_PX4);
    Q_CHECK_PTR(mainWindow);
    
    LinkManager* linkMgr = LinkManager::instance();
    Q_CHECK_PTR(linkMgr);
    
    MockLink* link = new MockLink();
    Q_CHECK_PTR(link);
    // FIXME: LinkManager/MainWindow needs to be re-architected so that you don't have to addLink to MainWindow to get things to work
    mainWindow->addLink(link);
    linkMgr->connectLink(link);
    QTest::qWait(5000); // Give enough time for UI to settle and heartbeats to go through
    
    // On Disconnect: We should get a getSaveFileName dialog.
    QDir logSaveDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    QString logSaveFile(logSaveDir.filePath(_saveLogFilename));
    setExpectedFileDialog(getSaveFileName, QStringList(logSaveFile));
    
    linkMgr->disconnectLink(link);
    QTest::qWait(1000); // Need to allow signals to move between threads

    checkExpectedFileDialog();
    
    // Make sure the file is there and delete it
    QCOMPARE(logSaveDir.remove(_saveLogFilename), true);
    
    // MainWindow deletes itself on close
    mainWindow->close();
    QTest::qWait(1000); // Need to allow signals to move between threads to shutdown MainWindow
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
