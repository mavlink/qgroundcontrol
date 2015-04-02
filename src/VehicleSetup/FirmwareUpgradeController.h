/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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
///     @author Don Gagne <don@thegagnes.com>

#ifndef FirmwareUpgradeController_H
#define FirmwareUpgradeController_H

#include "PX4FirmwareUpgradeThread.h"

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QQuickItem>

#include "qextserialport.h"

#include <stdint.h>

// Firmware Upgrade MVC Controller for FirmwareUpgrade.qml.
class FirmwareUpgradeController : public QObject
{
    Q_OBJECT
    
public:
    FirmwareUpgradeController(void);
    
    /// Supported firmware types
    typedef enum {
        StableFirmware,
        BetaFirmware,
        DeveloperFirmware,
        CustomFirmware
    } FirmwareType_t;

    Q_ENUMS(FirmwareType_t)
    
    /// Firmare type to load
    Q_PROPERTY(FirmwareType_t firmwareType READ firmwareType WRITE setFirmwareType)
    
    /// Upgrade push button in UI
    Q_PROPERTY(QQuickItem* upgradeButton READ upgradeButton WRITE setUpgradeButton)

    /// TextArea for log output
    Q_PROPERTY(QQuickItem* statusLog READ statusLog WRITE setStatusLog)
    
    /// Progress bar for you know what
    Q_PROPERTY(QQuickItem* progressBar READ progressBar WRITE setProgressBar)
    
    /// Begins the firware upgrade process
    Q_INVOKABLE void doFirmwareUpgrade(void);

    FirmwareType_t firmwareType(void) { return _firmwareType; }
    void setFirmwareType(FirmwareType_t firmwareType) { _firmwareType = firmwareType; }
    
    QQuickItem* upgradeButton(void) { return _upgradeButton; }
    void setUpgradeButton(QQuickItem* upgradeButton) { _upgradeButton = upgradeButton; }
    
    QQuickItem* progressBar(void) { return _progressBar; }
    void setProgressBar(QQuickItem* progressBar) { _progressBar = progressBar; }
    
    QQuickItem* statusLog(void) { return _statusLog; }
    void setStatusLog(QQuickItem* statusLog) { _statusLog = statusLog; }
    
private slots:
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
    void _findBoard(void);
    void _findBootloader(void);
    void _cancel(void);
    void _getFirmwareFile(void);
    
    void _downloadFirmware(void);
    
    void _erase(void);
        
    void _appendStatusLog(const QString& text);
    
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
    
    FirmwareType_t  _firmwareType;      ///< Firmware type to load
    QQuickItem*     _upgradeButton;     ///< Upgrade button in ui
    QQuickItem*     _statusLog;         ///< Status log TextArea Qml control
    QQuickItem*     _progressBar;
    
    bool _searchingForBoard;    ///< true: searching for board, false: search for bootloader
};

#endif
