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
///     @author Don Gagne <don@thegagnes.com>

#ifndef Bootloader_H
#define Bootloader_H

#include "FirmwareImage.h"

#include "qextserialport.h"

#include <stdint.h>

/// Bootloader Utility routines. Works with PX4 bootloader and 3DR Radio bootloader.
class Bootloader : public QObject
{
    Q_OBJECT
    
public:
    explicit Bootloader(QObject *parent = 0);
    
    /// @brief Returns the error message associated with the last failed call to one of the bootloader
    ///         utility routine below.
    QString errorString(void) { return _errorString; }
    
    /// @brief Opens a port to the bootloader
    bool open(QextSerialPort* port, const QString portName);
    
    /// @brief Read a PROTO_SYNC response from the bootloader
    /// @return true: Valid sync response was received
    bool sync(QextSerialPort* port);
    
    /// @brief Erases the current program
    bool erase(QextSerialPort* port);
    
    /// @brief Program the board with the specified image
    bool program(QextSerialPort* port, const FirmwareImage* image);
    
    /// @brief Verify the board flash.
    bool verify(QextSerialPort* port, const FirmwareImage* image);
    
    /// @brief Retrieve a set of board info from the bootloader of PX4 FMU and PX4 Flow boards
    ///     @param bootloaderVersion Returned INFO_BL_REV
    ///     @param boardID Returned INFO_BOARD_ID
    ///     @param flashSize Returned INFO_FLASH_SIZE
    bool getPX4BoardInfo(QextSerialPort* port, uint32_t& bootloaderVersion, uint32_t& boardID, uint32_t& flashSize);
    
    /// @brief Retrieve the board id from a 3DR Radio
    bool get3DRRadioBoardId(QextSerialPort* port, uint32_t& boardID);
    
    /// @brief Sends a PROTO_REBOOT command to the bootloader
    bool reboot(QextSerialPort* port);
    
    // Supported bootloader board ids
    static const int boardIDPX4FMUV1 = 5;   ///< PX4 V1 board
    static const int boardIDPX4FMUV2 = 9;   ///< PX4 V2 board
    static const int boardIDPX4Flow = 6;    ///< PX4 Flow board
    static const int boardIDAeroCore = 98;  ///< Gumstix AeroCore board
    static const int boardID3DRRadio = 78;  ///< 3DR Radio
    
signals:
    /// @brief Signals progress indicator for long running bootloader utility routines
    void updateProgress(int curr, int total);
    
private:
    bool _binProgram(QextSerialPort* port, const FirmwareImage* image);
    bool _ihxProgram(QextSerialPort* port, const FirmwareImage* image);
    
    bool _write(QextSerialPort* port, const uint8_t* data, qint64 maxSize);
    bool _write(QextSerialPort* port, const uint8_t byte);
    
    bool _read(QextSerialPort* port, uint8_t* data, qint64 maxSize, int readTimeout = _readTimout);
    
    bool _sendCommand(QextSerialPort* port, uint8_t cmd, int responseTimeout = _responseTimeout);
    bool _getCommandResponse(QextSerialPort* port, const int responseTimeout = _responseTimeout);
    
    bool _getPX4BoardInfo(QextSerialPort* port, uint8_t param, uint32_t& value);
    
    bool _verifyBytes(QextSerialPort* port, const FirmwareImage* image);
    bool _binVerifyBytes(QextSerialPort* port, const FirmwareImage* image);
    bool _ihxVerifyBytes(QextSerialPort* port, const FirmwareImage* image);
    bool _verifyCRC(QextSerialPort* port);

    enum {
        // protocol bytes
        PROTO_INSYNC =          0x12,   ///< 'in sync' byte sent before status
        PROTO_EOC =             0x20,   ///< end of command
        
        // Reply bytes
        PROTO_OK =              0x10,   ///< INSYNC/OK      - 'ok' response
        PROTO_FAILED =          0x11,   ///< INSYNC/FAILED  - 'fail' response
        PROTO_INVALID =         0x13,	///< INSYNC/INVALID - 'invalid' response for bad commands
        
        // Command bytes
        PROTO_GET_SYNC =        0x21,   ///< NOP for re-establishing sync
        PROTO_GET_DEVICE =      0x22,   ///< get device ID bytes
        PROTO_CHIP_ERASE =      0x23,   ///< erase program area and reset program address
        PROTO_LOAD_ADDRESS =    0x24,	///< set next programming address
        PROTO_PROG_MULTI =      0x27,   ///< write bytes at program address and increment
        PROTO_GET_CRC =         0x29,	///< compute & return a CRC
        PROTO_BOOT =            0x30,   ///< boot the application
        
        // Command bytes - Rev 2 boootloader only
        PROTO_CHIP_VERIFY	=   0x24, ///< begin verify mode
        PROTO_READ_MULTI	=   0x28, ///< read bytes at programm address and increment
        
        INFO_BL_REV         =   1,    ///< bootloader protocol revision
        BL_REV_MIN          =   2,    ///< Minimum supported bootlader protocol
        BL_REV_MAX			=   4,    ///< Maximum supported bootloader protocol
        INFO_BOARD_ID		=   2,    ///< board type
        INFO_BOARD_REV		=   3,    ///< board revision
        INFO_FLASH_SIZE		=   4,    ///< max firmware size in bytes
        
        PROG_MULTI_MAX		=   64,   ///< write size for PROTO_PROG_MULTI, must be multiple of 4
        READ_MULTI_MAX		=   255   ///< read size for PROTO_READ_MULTI, must be multiple of 4
    };
    
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
