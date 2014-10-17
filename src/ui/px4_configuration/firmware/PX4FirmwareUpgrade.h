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
#include <QSerialPort>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "ui_PX4FirmwareUpgrade.h"

// FIXME: Get rid of this
class QGCFirmwareUpgradeWorker;

class PX4FirmwareUpgrade : public QWidget
{
    Q_OBJECT

public:
    explicit PX4FirmwareUpgrade(QWidget *parent = 0);

signals:
    void connectLinks(void);
    void disconnectLinks(void);

private slots:
    void _tryAgain(void);
    void _cancelUpgrade(void);
    void _nextStep(void);
    void _firmwareSelected(int index);
    void _downloadProgress(qint64 curr, qint64 total);
    void _downloadFinished(void);
    void _downloadError(QNetworkReply::NetworkError code);

private:
    /// @brief The various states that the upgrade process progresses through.
    enum upgradeStates {
        upgradeStateBegin,
        upgradeStateBoardFound,
        upgradeStateBoardNotFound,
        upgradeStateBootloaderFound,
        upgradeStateBootloaderNotFound,
        upgradeStateFirmwareSelected,
        upgradeStateFirmwareDownloading,
        upgradeStateFirmwareDownloaded,
        upgradeStateDownloadFailed,
        upgradeStateBoardUpgraded,
        upgradeStateBoardUpgradeFailed
    };
    
    /// @brief Bits which can be or'ed together to identify a set of wizard buttons.
    enum wizardButtons {
        wizardButtonTryAgain = 1 << 1,
        wizardButtonSkip = 1 << 2,
        wizardButtonCancel = 1 << 3,
        wizardButtonNext = 1 << 4,
    };
    
    // FIXME: Reduce to only the ones we use
    enum {
        
        PROTO_NOP		= 0x00,
        PROTO_OK		= 0x10,
        PROTO_FAILED		= 0x11,
        PROTO_INSYNC		= 0x12,
        PROTO_INVALID   = 0x13,
        PROTO_EOC		= 0x20,
        PROTO_GET_SYNC		= 0x21,
        PROTO_GET_DEVICE	= 0x22,
        PROTO_CHIP_ERASE	= 0x23,
        PROTO_CHIP_VERIFY	= 0x24,
        PROTO_PROG_MULTI	= 0x27,
        PROTO_READ_MULTI	= 0x28,
        PROTO_GET_CRC		= 0x29,
        PROTO_REBOOT		= 0x30,
        
        INFO_BL_REV		= 1,		/**< bootloader protocol revision */
        BL_REV_MIN  = 2,        ///< Minimum supported bootlader protocol
        BL_REV_MAX			= 4,		/**< Maximum supported bootloader protocol  */
        INFO_BOARD_ID		= 2,		/**< board type */
        INFO_BOARD_REV		= 3,		/**< board revision */
        INFO_FLASH_SIZE		= 4,		/**< max firmware size in bytes */
        
        PROG_MULTI_MAX		= 128,		/**< protocol max is 255, must be multiple of 4 */
        READ_MULTI_MAX		= 60		/**< protocol max is 255, something overflows with >= 64 */
        
    };
    
    struct serialPortErrorString {
        int         error;
        const char* errorString;
    };
    
    void _upgradeBegin(void);
    void _upgradeBoardNotFound(void);
    void _upgradeBoardFound(void);
    void _upgradeBootloaderFound(void);
    void _upgradeBootloaderNotFound(void);
    void _upgradeFirmwareSelected(void);
    void _upgradeFirmwareDownloading(void);
    void _upgradeFirmwareDownloaded(void);
    void _upgradeDownloadFailed(void);
    void _upgradeBoardIDMismatch(void);
    void _upgradeBoardUpgraded(void);
    void _upgradeBoardUpgradeFailed(void);
    
    void _enableWizardButtons(unsigned int buttons);
    void _setBoardIcon(int boardID);
    void _setFirmwareCombo(int boardID);
    
    bool _findBoard(void);
    bool _findBootloader(void);
    bool _downloadFirmware(void);
    
    const char* _serialPortErrorString(int error);
    
    bool _bootloaderWrite(const char data, const QString errorPrefix);
    bool _bootloaderWrite(const char* data, qint64 maxSize, const QString errorPrefix);
    bool _bootloaderRead(char* data, qint64 maxSize, const QString errorPrefix, int readTimeout = 2000);
    bool _bootloaderGetCommandResponse(const QString& errorPrefix, int responseTimeout = 2000);
    bool _bootloaderGetBoardInfo(char param, uint32_t& value);
    bool _bootloaderCommand(char cmd, const QString& errorPrefix, int responseTimeout = 2000);
    bool _bootloaderProgram(void);
    
    enum upgradeStates _upgradeState;
    
    QString _portName;
    QString _portDescription;
    uint32_t _bootloaderVersion;

    static const int _boardIDPX4FMUV1 = 5;  ///< Board ID for PX4 V1 board
    static const int _boardIDPX4FMUV2 = 9;  ///< Board ID for PX4 V2 board
    static const int _boardIDPX4Flow = 6;   ///< Board ID for PX4 Floaw board

    uint32_t    _boardID;
    uint32_t    _boardFlashSize;
    uint32_t    _imageSize;

    QPixmap _boardIcon;
    
    QString _firmwareFilename;
    
    QSerialPort _bootloaderPort;
    QString _portErrorString;
    
    static const struct serialPortErrorString _rgSerialPortErrors[14];
    
    QNetworkAccessManager*  _downloadManager;
    QNetworkReply*          _downloadNetworkReply;
    
    Ui::PX4FirmwareUpgrade      _ui;
};

#endif // PX4FirmwareUpgrade_H
