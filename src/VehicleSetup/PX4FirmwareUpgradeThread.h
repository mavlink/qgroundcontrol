/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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
///     @brief PX4 Firmware Upgrade operations which occur on a seperate thread.
///     @author Don Gagne <don@thegagnes.com>

#ifndef PX4FirmwareUpgradeThread_H
#define PX4FirmwareUpgradeThread_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QTime>

#include "qextserialport.h"

#include <stdint.h>

#include "PX4Bootloader.h"

/// @brief Used to run bootloader commands on a seperate thread. These routines are mainly meant to to be called
///         internally by the PX4FirmwareUpgradeThreadController. Clients should call the various public methods
///         exposed by PX4FirmwareUpgradeThreadController.
class PX4FirmwareUpgradeThreadWorker : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadWorker(QObject* parent = NULL);
    ~PX4FirmwareUpgradeThreadWorker();
    
    enum {
        commandBootloader,
        commandProgram,
        commandVerify,
        commandErase,
        commandCancel
    };
    
public slots:
    void init(void);
    void findBoard(int msecTimeout);
    void findBootloader(const QString portName, int msecTimeout);
    void timeout(void);
    void cancelFind(void);
    void sendBootloaderReboot(void);
    void program(const QString firmwareFilename);
    void verify(const QString firmwareFilename);
    void erase(void);
    
signals:
    void foundBoard(bool firstTry, const QString portname, QString portDescription);
    void foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void bootloaderSyncFailed(void);
    void error(const int command, const QString errorString);
    void complete(const int command);
    void findTimeout(void);
    void updateProgress(int curr, int total);
    
private slots:
    void _findBoardOnce(void);
    void _findBootloaderOnce(void);
    void _updateProgramProgress(int curr, int total) { emit updateProgress(curr, total); }
    void _closeFind(void);
    
private:
    PX4Bootloader*      _bootloader;
    QextSerialPort*     _bootloaderPort;
    QTimer*             _timerTimeout;
    QTimer*             _timerRetry;
    QTime               _elapsed;
    QString             _portName;
    static const int    _retryTimeout = 1000;
    bool                _findBoardFirstAttempt;
};

/// @brief Provides methods to interact with the bootloader. The commands themselves are signalled
///         across to PX4FirmwareUpgradeThreadWorker so that they run on the seperate thread.
class PX4FirmwareUpgradeThreadController : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadController(QObject* parent = NULL);
    ~PX4FirmwareUpgradeThreadController(void);
    
    /// @brief Begins the process of searching for a PX4 board connected to any serial port.
    ///     @param msecTimeout Numbers of msecs to continue looking for a board to become available.
    void findBoard(int msecTimeout);
    
    /// @brief Begins the process of attempting to communicate with the bootloader on the specified port.
    ///     @param portName Name of port to attempt a bootloader connection on.
    ///     @param msecTimeout Number of msecs to continue to wait for a bootloader to appear on the port.
    void findBootloader(const QString& portName, int msecTimeout);
    
    /// @brief Cancel an in progress findBoard or FindBootloader
    void cancelFind(void) { emit _cancelFindOnThread(); }
    
    /// @brief Sends a reboot command to the bootloader
    void sendBootloaderReboot(void) { emit _sendBootloaderRebootOnThread(); }
    
    /// @brief Flash the specified firmware onto the board
    void program(const QString firmwareFilename) { emit _programOnThread(firmwareFilename); }
    
    /// @brief Verify the board flash with respect to the specified firmware image
    void verify(const QString firmwareFilename) { emit _verifyOnThread(firmwareFilename); }
    
    /// @brief Send and erase command to the bootloader
    void erase(void) { emit _eraseOnThread(); }

signals:
    /// @brief Emitted by the findBoard process when it finds the board.
    ///     @param firstTry true: board found on first attempt
    ///     @param portName Port that board is on
    ///     @param portDescription User friendly port description
    void foundBoard(bool firstTry, const QString portname, QString portDescription);
    
    /// @brief Emitted by the findBootloader process when has a connection to the bootloader
    void foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    
    /// @brief Emitted by the bootloader commands when an error occurs.
    ///     @param errorCommand Command which caused the error, using PX4FirmwareUpgradeThreadWorker command* enum values
    void error(const int errorCommand, const QString errorString);
    
    /// @brief Signalled when the findBootloader process connects to the port, but cannot sync to the
    ///         bootloader.
    void bootloaderSyncFailed(void);
    
    /// @brief Signalled when the findBoard or findBootloader process times out before success
    void findTimeout(void);
    
    /// @brief Signalled by the bootloader commands other than find* that they have complete successfully.
    ///     @param command Command which completed, using PX4FirmwareUpgradeThreadWorker command* enum values
    void complete(const int command);
    
    /// @brief Signalled to update progress for long running bootloader commands
    void updateProgress(int curr, int total);
    
    void _initThreadWorker(void);
    void _findBoardOnThread(int msecTimeout);
    void _findBootloaderOnThread(const QString& portName, int msecTimeout);
    void _sendBootloaderRebootOnThread(void);
    void _programOnThread(const QString firmwareFilename);
    void _verifyOnThread(const QString firmwareFilename);
    void _eraseOnThread(void);
    void _cancelFindOnThread(void);
    
private slots:
    void _foundBoard(bool firstTry, const QString portname, QString portDescription);
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void _bootloaderSyncFailed(void);
    void _error(const int errorCommand, const QString errorString) { emit error(errorCommand, errorString); }
    void _complete(const int command) { emit complete(command); }
    void _findTimeout(void);
    void _updateProgress(int curr, int total) { emit updateProgress(curr, total); }
    
private:
    PX4FirmwareUpgradeThreadWorker* _worker;
    QThread*                        _workerThread;  ///< Thread which PX4FirmwareUpgradeThreadWorker runs on
};

#endif
