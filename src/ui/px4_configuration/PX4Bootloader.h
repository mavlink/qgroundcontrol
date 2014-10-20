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
///     @brief PX4 Bootloader Utility routines
///     @author Don Gagne <don@thegagnes.com>

#ifndef PX4Bootloader_H
#define PX4Bootloader_H

#include <QSerialPort>

class PX4Bootloader : public QObject
{
    Q_OBJECT
    
public:
    explicit PX4Bootloader(QObject *parent = 0);
    ~PX4Bootloader();
    
    QString errorString(void) { return _errorString; }
    
    bool write(const uint8_t byte);
    bool write(const uint8_t* data, qint64 maxSize);
    bool read(uint8_t* data, qint64 maxSize, int readTimeout = 2000);
    bool getCommandResponse(const int responseTimeout = 2000);
    bool getBoardInfo(uint8_t param, uint32_t& value);
    bool sendCommand(uint8_t cmd, int responseTimeout = 2000);
    bool program(const QString& firmwareFilename);
    bool open(const QString portName);
    bool sync(void);
    bool getBoardInfo(uint32_t& bootloaderVersion, uint32_t& boardID, uint32_t& flashSize);
    void close(void);
    void sendFMUReboot(const QString portName);
    
signals:
    void updateProgramProgress(int curr, int total);
    
private:
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
        
        PROG_MULTI_MAX		= 60,		/**< protocol max is 255, must be multiple of 4 */
        READ_MULTI_MAX		= 60		/**< protocol max is 255, something overflows with >= 64 */
        
    };
    
    struct serialPortErrorString {
        int         error;
        const char* errorString;
    };
    
    bool _findBootloader(void);
    bool _downloadFirmware(void);
    
    const char* _serialPortErrorString(int error);
    
    
    QString _portName;
    QString _portDescription;
    uint32_t _bootloaderVersion;
    
    static const int _boardIDPX4FMUV1 = 5;  ///< Board ID for PX4 V1 board
    static const int _boardIDPX4FMUV2 = 9;  ///< Board ID for PX4 V2 board
    static const int _boardIDPX4Flow = 6;   ///< Board ID for PX4 Floaw board
    
    uint32_t    _boardID;
    uint32_t    _boardFlashSize;
    uint32_t    _imageCRC;
    
    QString _firmwareFilename;
    
    QSerialPort _port;
    QString _errorString;
    
    static const struct serialPortErrorString _rgSerialPortErrors[14];
};

#endif // PX4FirmwareUpgrade_H
