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
#include "LinkManager.h"
#include "FirmwareImage.h"

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QQuickItem>

#include "qextserialport.h"

#include <stdint.h>

/// Supported firmware types. If you modify these you will need to update the qml file as well.

// Firmware Upgrade MVC Controller for FirmwareUpgrade.qml.
class FirmwareUpgradeController : public QObject
{
    Q_OBJECT
    
public:
        typedef enum {
            AutoPilotStackPX4,
            AutoPilotStackAPM,
            PX4Flow,
            PX4Radio
        } AutoPilotStackType_t;

        typedef enum {
            StableFirmware,
            BetaFirmware,
            DeveloperFirmware,
            CustomFirmware
        } FirmwareType_t;

        typedef enum {
            QuadFirmware,
            X8Firmware,
            HexaFirmware,
            OctoFirmware,
            YFirmware,
            Y6Firmware,
            HeliFirmware,
            PlaneFirmware,
            RoverFirmware,
            DefaultVehicleFirmware = QuadFirmware,
        } FirmwareVehicleType_t;

        Q_ENUMS(AutoPilotStackType_t)
        Q_ENUMS(FirmwareType_t)
        Q_ENUMS(FirmwareVehicleType_t)

    class FirmwareIdentifier
    {
    public:
        FirmwareIdentifier(AutoPilotStackType_t stack = AutoPilotStackPX4,
                           FirmwareType_t firmware = StableFirmware,
                           FirmwareVehicleType_t vehicle = DefaultVehicleFirmware)
            : autopilotStackType(stack), firmwareType(firmware), firmwareVehicleType(vehicle) {}

        bool operator==(const FirmwareIdentifier& firmwareIDTrinity) const
        {
            return (firmwareIDTrinity.autopilotStackType == autopilotStackType &&
                    firmwareIDTrinity.firmwareType == firmwareType &&
                    firmwareIDTrinity.firmwareVehicleType == firmwareVehicleType);
        }

        // members
        AutoPilotStackType_t    autopilotStackType;
        FirmwareType_t          firmwareType;
        FirmwareVehicleType_t   firmwareVehicleType;
    };

    FirmwareUpgradeController(void);
    
    Q_PROPERTY(QString boardPort READ boardPort NOTIFY boardFound)
    Q_PROPERTY(QString boardDescription READ boardDescription NOTIFY boardFound)
    Q_PROPERTY(QString boardType MEMBER _foundBoardType NOTIFY boardFound)
    
    /// TextArea for log output
    Q_PROPERTY(QQuickItem* statusLog READ statusLog WRITE setStatusLog)
    
    /// Progress bar for you know what
    Q_PROPERTY(QQuickItem* progressBar READ progressBar WRITE setProgressBar)

    /// Returns true if there are active QGC connections
    Q_PROPERTY(bool qgcConnections READ qgcConnections NOTIFY qgcConnectionsChanged)
    
    /// Starts searching for boards on the background thread
    Q_INVOKABLE void startBoardSearch(void);
    
    /// Cancels whatever state the upgrade worker thread is in
    Q_INVOKABLE void cancel(void);
    
    /// Called when the firmware type has been selected by the user to continue the flash process.
    Q_INVOKABLE void flash(AutoPilotStackType_t stackType,
                           FirmwareType_t firmwareType = StableFirmware,
                           FirmwareVehicleType_t vehicleType = DefaultVehicleFirmware );
    
    // overload, not exposed to qml side
    void flash(const FirmwareIdentifier& firmwareId);

    // Property accessors
    
    QQuickItem* progressBar(void) { return _progressBar; }
    void setProgressBar(QQuickItem* progressBar) { _progressBar = progressBar; }
    
    QQuickItem* statusLog(void) { return _statusLog; }
    void setStatusLog(QQuickItem* statusLog) { _statusLog = statusLog; }
    
    bool qgcConnections(void);
    
    QString boardPort(void) { return _foundBoardInfo.portName(); }
    QString boardDescription(void) { return _foundBoardInfo.description(); }
    
signals:
    void boardFound(void);
    void noBoardFound(void);
    void boardGone(void);
    void flashComplete(void);
    void flashCancelled(void);
    void qgcConnectionsChanged(bool connections);
    void error(void);
    
private slots:
    void _downloadProgress(qint64 curr, qint64 total);
    void _downloadFinished(void);
    void _downloadError(QNetworkReply::NetworkError code);
    void _foundBoard(bool firstAttempt, const QSerialPortInfo& portInfo, int type);
    void _noBoardFound(void);
    void _boardGone();
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void _error(const QString& errorString);
    void _status(const QString& statusString);
    void _bootloaderSyncFailed(void);
    void _flashComplete(void);
    void _updateProgress(int curr, int total);
    void _eraseStarted(void);
    void _eraseComplete(void);
    void _eraseProgressTick(void);
    void _linkDisconnected(LinkInterface* link);

private:
    void _getFirmwareFile(FirmwareIdentifier firmwareIDTrinity);
    void _initFirmwareHash();
    void _downloadFirmware(void);
    void _appendStatusLog(const QString& text, bool critical = false);
    void _errorCancel(const QString& msg);
    
    QString _portName;
    QString _portDescription;

    // firmware hashes
    QHash<FirmwareIdentifier, QString> _rgPX4FMUV2Firmware;
    QHash<FirmwareIdentifier, QString> _rgAeroCoreFirmware;
    QHash<FirmwareIdentifier, QString> _rgPX4FMUV1Firmware;
    QHash<FirmwareIdentifier, QString> _rgPX4FLowFirmware;
    QHash<FirmwareIdentifier, QString> _rg3DRRadioFirmware;

    /// Information which comes back from the bootloader
    bool        _bootloaderFound;           ///< true: we have received the foundBootloader signals
    uint32_t    _bootloaderVersion;         ///< Bootloader version
    uint32_t    _bootloaderBoardID;         ///< Board ID
    uint32_t    _bootloaderBoardFlashSize;  ///< Flash size in bytes of board
    
    bool                 _startFlashWhenBootloaderFound;
    FirmwareIdentifier   _startFlashWhenBootloaderFoundFirmwareIdentity;

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
    
    QQuickItem*     _statusLog;         ///< Status log TextArea Qml control
    QQuickItem*     _progressBar;
    
    bool _searchingForBoard;    ///< true: searching for board, false: search for bootloader
    
    QSerialPortInfo _foundBoardInfo;
    QString         _foundBoardType;
    
    FirmwareImage*  _image;
};

// global hashing function
uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareIDTrinity);

#endif
