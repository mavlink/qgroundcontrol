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

#include "PX4Bootloader.h"
#include "IntelHexFirmware.h"

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
    FoundBoard3drRadio
} PX4FirmwareUpgradeFoundBoardType_t;

/// @brief Used to run bootloader commands on a seperate thread. These routines are mainly meant to to be called
///         internally by the PX4FirmwareUpgradeThreadController. Clients should call the various public methods
///         exposed by PX4FirmwareUpgradeThreadController.
class PX4FirmwareUpgradeThreadWorker : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadWorker(QObject* parent = NULL);
    ~PX4FirmwareUpgradeThreadWorker();
    
public slots:
    void init(void);
    void startFindBoardLoop(void);
    void sendBootloaderReboot(void);
    void binFlash(const QString& binFilename);
    void ihxFlash(const IntelHexFirmware& ihxFirmware);
    
signals:
    void foundBoard(bool firstAttempt, const QSerialPortInfo &portInfo, int type);
    void boardGone();
    void noBoardFound();
    void foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void bootloaderSyncFailed(void);
    void error(const QString& errorString);
    void status(const QString& status);
    void flashComplete();
    void updateProgress(int curr, int total);
    
private slots:
    void _findBoardOnce(void);
    void _updateProgramProgress(int curr, int total) { emit updateProgress(curr, total); }
    
private:
    bool _findBoardFromPorts(QSerialPortInfo& portInfo, PX4FirmwareUpgradeFoundBoardType_t& type);
    void _findBootloader(const QSerialPortInfo& portInfo, bool radioMode);
    void _3drRadioForceBootloader(const QSerialPortInfo& portInfo);
    bool _erase(void);
    void _binOrIhxFlash(const QString* binFilename, const IntelHexFirmware* ihxFirmware, bool firmwareIsBin);
    
    PX4Bootloader*      _bootloader;
    QextSerialPort*     _bootloaderPort;
    QTimer*             _timerRetry;
    QTime               _elapsed;
    static const int    _retryTimeout = 1000;
    
    bool                _foundBoard;            ///< true: board is currently connected
    bool                _findBoardFirstAttempt; ///< true: this is our first try looking for a board
    QSerialPortInfo     _foundBoardPortInfo;    ///< port info for found board
    
    // Serial port info for supported devices
    
    static const int    _px4VendorId = 9900;
    
    static const int    _pixhawkFMUV2ProductId = 17;
    static const int    _pixhawkFMUV1ProductId = 16;
    static const int    _flowProductId = 21;
    
    static const int    _3drRadioVendorId = 1027;
    static const int    _3drRadioProductId = 24597;
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
    void sendBootloaderReboot(void) { emit _sendBootloaderRebootOnThread(); }
    
    void binFlash(const QString& binFilename) { emit _binFlashOnThread(binFilename); }
    void ihxFlash(const IntelHexFirmware& ihxFirmware) { emit _ihxFlashOnThread(ihxFirmware); }
    
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
    
    void flashComplete(void);
    
    /// @brief Signalled to update progress for long running bootloader commands
    void updateProgress(int curr, int total);
    
    // Internal signals to communicator with thread worker
    void _initThreadWorker(void);
    void _startFindBoardLoopOnThread(void);
    void _sendBootloaderRebootOnThread(void);
    void _binFlashOnThread(const QString& binFilename);
    void _ihxFlashOnThread(const IntelHexFirmware& ihxFirmware);
    
private slots:
    void _foundBoard(bool firstAttempt, const QSerialPortInfo& portInfo, int type);
    void _noBoardFound(void);
    void _boardGone(void);
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void _bootloaderSyncFailed(void);
    void _error(const QString& errorString) { emit error(errorString); }
    void _status(const QString& statusText) { emit status(statusText); }
    void _flashComplete(void) { emit flashComplete(); }
    void _updateProgress(int curr, int total) { emit updateProgress(curr, total); }
    
private:
    PX4FirmwareUpgradeThreadWorker* _worker;
    QThread*                        _workerThread;  ///< Thread which PX4FirmwareUpgradeThreadWorker runs on
};

#endif
