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

#include "Bootloader.h"
#include "FirmwareImage.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QTime>
#include <QSerialPortInfo>

#include "qextserialport.h"

#include <stdint.h>

typedef enum {
    FoundBoardPX4FMUV1,
    FoundBoardPX4FMUV2,
    FoundBoardPX4Flow,
    FoundBoard3drRadio,
    FoundBoardAeroCore
} PX4FirmwareUpgradeFoundBoardType_t;

class PX4FirmwareUpgradeThreadController;

/// @brief Used to run bootloader commands on a seperate thread. These routines are mainly meant to to be called
///         internally by the PX4FirmwareUpgradeThreadController. Clients should call the various public methods
///         exposed by PX4FirmwareUpgradeThreadController.
class PX4FirmwareUpgradeThreadWorker : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadWorker(PX4FirmwareUpgradeThreadController* controller);
    ~PX4FirmwareUpgradeThreadWorker();
    
signals:
    void updateProgress(int curr, int total);
    void foundBoard(bool firstAttempt, const QSerialPortInfo& portInfo, int type);
    void noBoardFound(void);
    void boardGone(void);
    void foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void bootloaderSyncFailed(void);
    void error(const QString& errorString);
    void status(const QString& statusText);
    void eraseStarted(void);
    void eraseComplete(void);
    void flashComplete(void);
    
private slots:
    void _init(void);
    void _startFindBoardLoop(void);
    void _reboot(void);
    void _flash(void);
    void _findBoardOnce(void);
    void _updateProgress(int curr, int total) { emit updateProgress(curr, total); }
    void _cancel(void);
    
private:
    bool _findBoardFromPorts(QSerialPortInfo& portInfo, PX4FirmwareUpgradeFoundBoardType_t& type);
    bool _findBootloader(const QSerialPortInfo& portInfo, bool radioMode, bool errorOnNotFound);
    void _3drRadioForceBootloader(const QSerialPortInfo& portInfo);
    bool _erase(void);
    
    PX4FirmwareUpgradeThreadController* _controller;
    
    Bootloader*      _bootloader;
    QextSerialPort*     _bootloaderPort;
    QTimer*             _timerRetry;
    QTime               _elapsed;
    static const int    _retryTimeout = 100;
    
    bool                _foundBoard;            ///< true: board is currently connected
    bool                _findBoardFirstAttempt; ///< true: this is our first try looking for a board
    QSerialPortInfo     _foundBoardPortInfo;    ///< port info for found board
};

/// @brief Provides methods to interact with the bootloader. The commands themselves are signalled
///         across to PX4FirmwareUpgradeThreadWorker so that they run on the seperate thread.
class PX4FirmwareUpgradeThreadController : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadController(QObject* parent = NULL);
    ~PX4FirmwareUpgradeThreadController(void);
    
    /// @brief Begins the process of searching for a supported board connected to any serial port. This will
    /// continue until cancelFind is called. Signals foundBoard and boardGone as boards come and go.
    void startFindBoardLoop(void);
    
    void cancel(void);
    
    /// @brief Sends a reboot command to the bootloader
    void reboot(void) { emit _rebootOnThread(); }
    
    void flash(const FirmwareImage* image);
    
    const FirmwareImage* image(void) { return _image; }
    
signals:
    /// @brief Emitted by the find board process when it finds a board.
    void foundBoard(bool firstAttempt, const QSerialPortInfo &portInfo, int type);
    
    void noBoardFound(void);
    
    /// @brief Emitted by the find board process when a board it previously reported as found disappears.
    void boardGone(void);
    
    /// @brief Emitted by the findBootloader process when has a connection to the bootloader
    void foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    
    /// @brief Emitted by the bootloader commands when an error occurs.
    void error(const QString& errorString);
    
    void status(const QString& status);
    
    /// @brief Signalled when the findBootloader process connects to the port, but cannot sync to the
    ///         bootloader.
    void bootloaderSyncFailed(void);
    
    void eraseStarted(void);
    void eraseComplete(void);
    
    void flashComplete(void);
    
    /// @brief Signalled to update progress for long running bootloader commands
    void updateProgress(int curr, int total);
    
    // Internal signals to communicate with thread worker
    void _initThreadWorker(void);
    void _startFindBoardLoopOnThread(void);
    void _rebootOnThread(void);
    void _flashOnThread(void);
    void _cancel(void);
    
private slots:
    void _foundBoard(bool firstAttempt, const QSerialPortInfo& portInfo, int type) { emit foundBoard(firstAttempt, portInfo, type); }
    void _noBoardFound(void) { emit noBoardFound(); }
    void _boardGone(void) { emit boardGone(); }
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize) { emit foundBootloader(bootloaderVersion, boardID, flashSize); }
    void _bootloaderSyncFailed(void) { emit bootloaderSyncFailed(); }
    void _error(const QString& errorString) { emit error(errorString); }
    void _status(const QString& statusText) { emit status(statusText); }
    void _eraseStarted(void) { emit eraseStarted(); }
    void _eraseComplete(void) { emit eraseComplete(); }
    void _flashComplete(void) { emit flashComplete(); }
    
private:
    void _updateProgress(int curr, int total) { emit updateProgress(curr, total); }
    
    PX4FirmwareUpgradeThreadWorker* _worker;
    QThread*                        _workerThread;  ///< Thread which PX4FirmwareUpgradeThreadWorker runs on
    
    const FirmwareImage* _image;
};

#endif
