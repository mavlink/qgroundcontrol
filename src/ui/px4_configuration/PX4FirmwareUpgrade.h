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
///     @brief PX4 Firmware Upgrade UI
///     @author Don Gagne <don@thegagnes.com>

#ifndef PX4FirmwareUpgrade_H
#define PX4FirmwareUpgrade_H

#include <QWidget>
#include <QUrl>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "qextserialport.h"

#include <stdint.h>

#include "PX4FirmwareUpgradeThread.h"

#include "ui_PX4FirmwareUpgrade.h"

namespace Ui {
    class PX4RCCalibration;
}

class PX4FirmwareUpgrade : public QWidget
{
    Q_OBJECT

public:
    explicit PX4FirmwareUpgrade(QWidget *parent = 0);
    ~PX4FirmwareUpgrade();

private slots:
    void _tryAgainButton(void);
    void _cancelButton(void);
    void _nextButton(void);
    void _firmwareSelected(int index);
    void _downloadProgress(qint64 curr, qint64 total);
    void _downloadFinished(void);
    void _downloadError(QNetworkReply::NetworkError code);
    void _foundBoard(bool firstTry, const QString portname, QString portDescription);
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void _error(const int command, const QString errorString);
    void _bootloaderSyncFailed(void);
    void _findTimeout(void);
    void _complete(const int command);
    void _updateProgress(int curr, int total);
    void _restart(void);
    void _eraseProgressTick(void);

private:
    /// @brief The various states that the upgrade process progresses through.
    enum upgradeStates {
        upgradeStateBegin,
        upgradeStateBoardSearch,
        upgradeStateBoardNotFound,
        upgradeStateBootloaderSearch,
        upgradeStateBootloaderNotFound,
        upgradeStateBootloaderError,
        upgradeStateFirmwareSelect,
        upgradeStateFirmwareDownloading,
        upgradeStateDownloadFailed,
        upgradeStateErasing,
        upgradeStateEraseError,
        upgradeStateFlashing,
        upgradeStateFlashError,
        upgradeStateVerifying,
        upgradeStateVerifyError,
        upgradeStateBoardUpgraded,
        upgradeStateMax
    };
    
    void _setupState(enum upgradeStates state);
    void _updateIndicatorUI(void);
    
    void _findBoard(void);
    void _findBootloader(void);
    void _cancel(void);
    void _cancelFind(void);
    void _getFirmwareFile(void);
    
    void _setBoardIcon(int boardID);
    void _setFirmwareCombo(int boardID);
    
    void _downloadFirmware(void);
    void _cancelDownload(void);
    
    void _erase(void);
    
    typedef void (PX4FirmwareUpgrade::*stateFunc)(void);
    struct stateMachineEntry {
        enum upgradeStates  state;      ///< State machine state, used to verify correctness of entry
        stateFunc           next;       ///< Method to call when Next is clicked, NULL for Next not available
        stateFunc           cancel;     ///< Method to call when Cancel is clicked, NULL for Cancel not available
        stateFunc           tryAgain;   ///< Method to call when Try Again is clicked, NULL for Try Again not available
        const char*         msg;        ///< Text message to display to user for this state
    };
    
    const struct stateMachineEntry* _getStateMachineEntry(enum upgradeStates state);
    
    enum upgradeStates _upgradeState;       ///< Current state of the upgrade state machines
    
    QString _portName;
    QString _portDescription;
    uint32_t _bootloaderVersion;

    static const int _boardIDPX4FMUV1 = 5;  ///< Board ID for PX4 V1 board
    static const int _boardIDPX4FMUV2 = 9;  ///< Board ID for PX4 V2 board
    static const int _boardIDPX4Flow = 6;   ///< Board ID for PX4 Flow board

    uint32_t    _boardID;           ///< Board ID
    uint32_t    _boardFlashSize;    ///< Flash size in bytes of board
    uint32_t    _imageSize;         ///< Image size of firmware being flashed

    QPixmap _boardIcon;             ///< Icon used to display image of board
    
    QString _firmwareFilename;      ///< Image which we are going to flash to the board
    
    QNetworkAccessManager*  _downloadManager;       ///< Used for firmware file downloading across the internet
    QNetworkReply*          _downloadNetworkReply;  ///< Used for firmware file downloading across the internet
    
    /// @brief Thread controller which is used to run bootloader commands on seperate thread
    PX4FirmwareUpgradeThreadController* _threadController;
    
    static const int    _eraseTickMsec = 500;       ///< Progress bar update tick time for erase
    static const int    _eraseTotalMsec = 15000;    ///< Estimated amount of time erase takes
    int                 _eraseTickCount;            ///< Number of ticks for erase progress update
    QTimer              _eraseTimer;                ///< Timer used to update progress bar for erase

    static const int    _findBoardTimeoutMsec = 30000;      ///< Amount of time for user to plug in USB
    static const int    _findBootloaderTimeoutMsec = 5000;  ///< Amount time to look for bootloader
    
    Ui::PX4FirmwareUpgrade* _ui;
};

#endif // PX4FirmwareUpgrade_H
