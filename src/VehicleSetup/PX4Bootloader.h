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

#include "qextserialport.h"

#include <stdint.h>

/// @brief This class is used to communicate with the Bootloader.
class PX4Bootloader : public QObject
{
    Q_OBJECT
    
public:
    explicit PX4Bootloader(QObject *parent = 0);
    
    /// @brief Returns the error message associated with the last failed call to one of the bootloader
    ///         utility routine below.
    QString errorString(void) { return _errorString; }
    
    /// @brief Write a byte to the port
    ///     @param port Port to write to
    ///     @param data Bytes to write
    ///     @param maxSize Number of bytes to write
    /// @return true: success
    bool write(QextSerialPort* port, const uint8_t* data, qint64 maxSize);
    bool write(QextSerialPort* port, const uint8_t byte);
    
    /// @brief Read a set of bytes from the port
    ///     @param data Read bytes into this buffer
    ///     @param maxSize Number of bytes to read
    ///     @param readTimeout Msecs to wait for bytes to become available on port
    bool read(QextSerialPort* port, uint8_t* data, qint64 maxSize, int readTimeout = _readTimout);
    
    /// @brief Read a PROTO_SYNC command response from the bootloader
    ///     @param responseTimeout Msecs to wait for response bytes to become available on port
    bool getCommandResponse(QextSerialPort* port, const int responseTimeout = _responseTimeout);
    
    /// @brief Send a PROTO_GET_DEVICE command to retrieve a value from the bootloader
    ///     @param param Value to retrieve using INFO_BOARD_* enums
    ///     @param value Returned value
    bool getBoardInfo(QextSerialPort* port, uint8_t param, uint32_t& value);
    
    /// @brief Send a command to the bootloader
    ///     @param cmd Command to send using PROTO_* enums
    /// @return true: Command sent and valid sync response returned
    bool sendCommand(QextSerialPort* port, uint8_t cmd, int responseTimeout = _responseTimeout);
    
    /// @brief Program the board with the specified firmware
    bool program(QextSerialPort* port, const QString& firmwareFilename);
    
    /// @brief Verify the board flash. How it works depend on bootloader rev
    ///         Rev 2: Read the flash back and compare it against the firmware file
    ///         Rev 3: Compare CRCs for flash and file
    bool verify(QextSerialPort* port, const QString firmwareFilename);
    
    /// @brief Read a PROTO_SYNC response from the bootloader
    /// @return true: Valid sync response was received
    bool sync(QextSerialPort* port);
    
    /// @brief Retrieve a set of board info from the bootloader
    ///     @param bootloaderVersion Returned INFO_BL_REV
    ///     @param boardID Returned INFO_BOARD_ID
    ///     @param flashSize Returned INFO_FLASH_SIZE
    bool getBoardInfo(QextSerialPort* port, uint32_t& bootloaderVersion, uint32_t& boardID, uint32_t& flashSize);
    
    /// @brief Opens a port to the bootloader
    bool open(QextSerialPort* port, const QString portName);
    
    /// @brief Sends a PROTO_REBOOT command to the bootloader
    bool sendBootloaderReboot(QextSerialPort* port);
    
    /// @brief Sends a PROTO_ERASE command to the bootlader
    bool erase(QextSerialPort* port);
    
signals:
    /// @brief Signals progress indicator for long running bootloader utility routines
    void updateProgramProgress(int curr, int total);
    
private:
    enum {
        // protocol bytes
        PROTO_INSYNC =      0x12,   ///< 'in sync' byte sent before status
        PROTO_EOC = 		0x20,   ///< end of command
        
        // Reply bytes
        PROTO_OK =          0x10,   ///< INSYNC/OK      - 'ok' response
        PROTO_FAILED =		0x11,   ///< INSYNC/FAILED  - 'fail' response
        PROTO_INVALID =		0x13,	///< INSYNC/INVALID - 'invalid' response for bad commands
        
        // Command bytes
        PROTO_GET_SYNC =	0x21,   ///< NOP for re-establishing sync
        PROTO_GET_DEVICE =	0x22,   ///< get device ID bytes
        PROTO_CHIP_ERASE =	0x23,   ///< erase program area and reset program address
        PROTO_PROG_MULTI =	0x27,   ///< write bytes at program address and increment
        PROTO_GET_CRC =		0x29,	///< compute & return a CRC
        PROTO_BOOT =		0x30,   ///< boot the application
        
        // Command bytes - Rev 2 boootloader only
        PROTO_CHIP_VERIFY	= 0x24, ///< begin verify mode
        PROTO_READ_MULTI	= 0x28, ///< read bytes at programm address and increment
        
        INFO_BL_REV         = 1,    ///< bootloader protocol revision
        BL_REV_MIN          = 2,    ///< Minimum supported bootlader protocol
        BL_REV_MAX			= 4,    ///< Maximum supported bootloader protocol
        INFO_BOARD_ID		= 2,    ///< board type
        INFO_BOARD_REV		= 3,    ///< board revision
        INFO_FLASH_SIZE		= 4,    ///< max firmware size in bytes
        
        PROG_MULTI_MAX		= 32,   ///< write size for PROTO_PROG_MULTI, must be multiple of 4
        READ_MULTI_MAX		= 32    ///< read size for PROTO_READ_MULTI, must be multiple of 4
    };
    
    bool _findBootloader(void);
    bool _downloadFirmware(void);
    bool _bootloaderVerifyRev2(QextSerialPort* port, const QString firmwareFilename);
    bool _bootloaderVerifyRev3(QextSerialPort* port);
    
    uint32_t    _boardID;           ///< board id for currently connected board
    uint32_t    _boardFlashSize;    ///< flash size for currently connected board
    uint32_t    _imageCRC;          ///< CRC for image in currently selected firmware file
    uint32_t    _bootloaderVersion; ///< Bootloader version
    
    QString _firmwareFilename;      ///< Currently selected firmware file to flash
    
    QString _errorString;           ///< Last error
    
    static const int _eraseTimeout = 20000;     ///< Msecs to wait for response from erase command
    static const int _rebootTimeout = 10000;    ///< Msecs to wait for reboot command to cause serial port to disconnect
    static const int _verifyTimeout = 5000;     ///< Msecs to wait for response to PROTO_GET_CRC command
    static const int _readTimout = 2000;        ///< Msecs to wait for read bytes to become avilable
    static const int _responseTimeout = 2000;   ///< Msecs to wait for command response bytes
};

#endif // PX4FirmwareUpgrade_H
