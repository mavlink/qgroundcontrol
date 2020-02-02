/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief PX4 Bootloader Utility routines
///     @author Don Gagne <don@thegagnes.com>

#include "Bootloader.h"
#include "QGCLoggingCategory.h"

#include <QFile>
#include <QSerialPortInfo>
#include <QDebug>
#include <QTime>

#include "QGC.h"

Bootloader::Bootloader(QObject *parent) :
    QObject(parent)
{

}

bool Bootloader::_write(QSerialPort* port, const uint8_t* data, qint64 maxSize)
{
    qint64 bytesWritten = port->write((const char*)data, maxSize);
    if (bytesWritten == -1) {
        _errorString = tr("Write failed: %1").arg(port->errorString());
        qWarning() << _errorString;
        return false;
    }
    if (bytesWritten != maxSize) {
        _errorString = tr("Incorrect number of bytes returned for write: actual(%1) expected(%2)").arg(bytesWritten).arg(maxSize);
        qWarning() << _errorString;
        return false;
    }
    
    return true;
}

bool Bootloader::_write(QSerialPort* port, const uint8_t byte)
{
    uint8_t buf[1] = { byte };
    return _write(port, buf, 1);
}

bool Bootloader::_read(QSerialPort* port, uint8_t* data, qint64 maxSize, int readTimeout)
{
    qint64 bytesAlreadyRead = 0;
    
    while (bytesAlreadyRead < maxSize) {
        QTime timeout;
        timeout.start();
        while (port->bytesAvailable() < 1) {
            if (timeout.elapsed() > readTimeout) {
                _errorString = tr("Timeout waiting for bytes to be available");
                return false;
            }
            port->waitForReadyRead(100);
        }
        
        qint64 bytesRead;
        bytesRead = port->read((char*)&data[bytesAlreadyRead], maxSize);
        
        if (bytesRead == -1) {
            _errorString = tr("Read failed: error: %1").arg(port->errorString());
            return false;
        } else {
            Q_ASSERT(bytesRead != 0);
            bytesAlreadyRead += bytesRead;
        }
    }
    
    return true;
}

/// Read a PROTO_SYNC command response from the bootloader
///     @param responseTimeout Msecs to wait for response bytes to become available on port
bool Bootloader::_getCommandResponse(QSerialPort* port, int responseTimeout)
{
    uint8_t response[2];
    
    if (!_read(port, response, 2, responseTimeout)) {
        _errorString.prepend(tr("Get Command Response: "));
        return false;
    }
    
    // Make sure we get a good sync response
    if (response[0] != PROTO_INSYNC) {
        _errorString = tr("Invalid sync response: 0x%1 0x%2").arg(response[0], 2, 16, QLatin1Char('0')).arg(response[1], 2, 16, QLatin1Char('0'));
        return false;
    } else if (response[0] == PROTO_INSYNC && response[1] == PROTO_BAD_SILICON_REV) {
        _errorString = tr("This board is using a microcontroller with faulty silicon and an incorrect configuration and should be put out of service.");
        return false;
    } else if (response[1] != PROTO_OK) {
        QString responseCode = tr("Unknown response code");
        if (response[1] == PROTO_FAILED) {
            responseCode = "PROTO_FAILED";
        } else if (response[1] == PROTO_INVALID) {
            responseCode = "PROTO_INVALID";
        }
        _errorString = tr("Command failed: 0x%1 (%2)").arg(response[1], 2, 16, QLatin1Char('0')).arg(responseCode);
        return false;
    }
    
    return true;
}

/// Send a PROTO_GET_DEVICE command to retrieve a value from the PX4 bootloader
///     @param param Value to retrieve using INFO_BOARD_* enums
///     @param value Returned value
bool Bootloader::_getPX4BoardInfo(QSerialPort* port, uint8_t param, uint32_t& value)
{
    uint8_t buf[3] = { PROTO_GET_DEVICE, param, PROTO_EOC };
    
    if (!_write(port, buf, sizeof(buf))) {
        goto Error;
    }
    port->flush();
    if (!_read(port, (uint8_t*)&value, sizeof(value))) {
        goto Error;
    }
    if (!_getCommandResponse(port)) {
        goto Error;
    }
    
    return true;
    
Error:
    _errorString.prepend(tr("Get Board Info: "));
    return false;
}

/// Send a command to the bootloader
///     @param cmd Command to send using PROTO_* enums
/// @return true: Command sent and valid sync response returned
bool Bootloader::_sendCommand(QSerialPort* port, const uint8_t cmd, int responseTimeout)
{
    uint8_t buf[2] = { cmd, PROTO_EOC };
    
    if (!_write(port, buf, 2)) {
        goto Error;
    }
    port->flush();
    if (!_getCommandResponse(port, responseTimeout)) {
        goto Error;
    }
    
    return true;

Error:
    _errorString.prepend(tr("Send Command: "));
    return false;
}

bool Bootloader::erase(QSerialPort* port)
{
    // Erase is slow, need larger timeout
    if (!_sendCommand(port, PROTO_CHIP_ERASE, _eraseTimeout)) {
        _errorString = tr("Board erase failed: %1").arg(_errorString);
        return false;
    }
    
    return true;
}

bool Bootloader::program(QSerialPort* port, const FirmwareImage* image)
{
    if (image->imageIsBinFormat()) {
        return _binProgram(port, image);
    } else {
        return _ihxProgram(port, image);
    }
}

bool Bootloader::_binProgram(QSerialPort* port, const FirmwareImage* image)
{
    QFile firmwareFile(image->binFilename());
    if (!firmwareFile.open(QIODevice::ReadOnly)) {
        _errorString = tr("Unable to open firmware file %1: %2").arg(image->binFilename(), firmwareFile.errorString());
        return false;
    }
    uint32_t imageSize = (uint32_t)firmwareFile.size();
    
    uint8_t imageBuf[PROG_MULTI_MAX];
    uint32_t bytesSent = 0;
    _imageCRC = 0;
    
    Q_ASSERT(PROG_MULTI_MAX <= 0x8F);
    
    while (bytesSent < imageSize) {
        int bytesToSend = imageSize - bytesSent;
        if (bytesToSend > (int)sizeof(imageBuf)) {
            bytesToSend = (int)sizeof(imageBuf);
        }
        
        Q_ASSERT((bytesToSend % 4) == 0);
        
        int bytesRead = firmwareFile.read((char *)imageBuf, bytesToSend);
        if (bytesRead == -1 || bytesRead != bytesToSend) {
            _errorString = tr("Firmware file read failed: %1").arg(firmwareFile.errorString());
            return false;
        }
        
        Q_ASSERT(bytesToSend <= 0x8F);
        
        bool failed = true;
        if (_write(port, PROTO_PROG_MULTI)) {
            if (_write(port, (uint8_t)bytesToSend)) {
                if (_write(port, imageBuf, bytesToSend)) {
                    if (_write(port, PROTO_EOC)) {
                        port->flush();
                        if (_getCommandResponse(port)) {
                            failed = false;
                        }
                    }
                }
            }
        }
        if (failed) {
            _errorString = tr("Flash failed: %1 at address 0x%2").arg(_errorString).arg(bytesSent, 8, 16, QLatin1Char('0'));
            return false;
        }
        
        bytesSent += bytesToSend;
        
        // Calculate the CRC now so we can test it after the board is flashed.
        _imageCRC = QGC::crc32((uint8_t *)imageBuf, bytesToSend, _imageCRC);
        
        emit updateProgress(bytesSent, imageSize);
    }
    firmwareFile.close();
    
    // We calculate the CRC using the entire flash size, filling the remainder with 0xFF.
    while (bytesSent < _boardFlashSize) {
        const uint8_t fill = 0xFF;
        _imageCRC = QGC::crc32(&fill, 1, _imageCRC);
        bytesSent++;
    }
    
    return true;
}

bool Bootloader::_ihxProgram(QSerialPort* port, const FirmwareImage* image)
{
    uint32_t imageSize = image->imageSize();
    uint32_t bytesSent = 0;

    for (uint16_t index=0; index<image->ihxBlockCount(); index++) {
        bool        failed;
        uint16_t    flashAddress;
        QByteArray  bytes;
        
        if (!image->ihxGetBlock(index, flashAddress, bytes)) {
            _errorString = tr("Unable to retrieve block from ihx: index %1").arg(index);
            return false;
        }
        
        qCDebug(FirmwareUpgradeVerboseLog) << QString("Bootloader::_ihxProgram - address:0x%1 size:%2 block:%3").arg(flashAddress, 8, 16, QLatin1Char('0')).arg(bytes.count()).arg(index);
        
        // Set flash address
        
        failed = true;
        if (_write(port, PROTO_LOAD_ADDRESS) &&
                _write(port, flashAddress & 0xFF) &&
                _write(port, (flashAddress >> 8) & 0xFF) &&
                _write(port, PROTO_EOC)) {
            port->flush();
            if (_getCommandResponse(port)) {
                failed = false;
            }
        }
        
        if (failed) {
            _errorString = tr("Unable to set flash start address: 0x%2").arg(flashAddress, 8, 16, QLatin1Char('0'));
            return false;
        }
        
        // Flash
        
        int bytesIndex = 0;
        uint16_t bytesLeftToWrite = bytes.count();
        
        while (bytesLeftToWrite > 0) {
            uint8_t bytesToWrite;
        
            if (bytesLeftToWrite > PROG_MULTI_MAX) {
                bytesToWrite = PROG_MULTI_MAX;
            } else {
                bytesToWrite = bytesLeftToWrite;
            }
        
            failed = true;
            if (_write(port, PROTO_PROG_MULTI) &&
                    _write(port, bytesToWrite) &&
                    _write(port, &((uint8_t *)bytes.data())[bytesIndex], bytesToWrite) &&
                    _write(port, PROTO_EOC)) {
                port->flush();
                if (_getCommandResponse(port)) {
                    failed = false;
                }
            }
            if (failed) {
                _errorString = tr("Flash failed: %1 at address 0x%2").arg(_errorString).arg(flashAddress, 8, 16, QLatin1Char('0'));
                return false;
            }
            
            bytesIndex += bytesToWrite;
            bytesLeftToWrite -= bytesToWrite;
            bytesSent += bytesToWrite;
            
            emit updateProgress(bytesSent, imageSize);
        }
    }
    
    return true;
}

bool Bootloader::verify(QSerialPort* port, const FirmwareImage* image)
{
    bool ret;
    
    if (!image->imageIsBinFormat() || _bootloaderVersion <= 2) {
        ret = _verifyBytes(port, image);
    } else {
        ret = _verifyCRC(port);
    }
    
    reboot(port);
    
    return ret;
}

/// @brief Verify the flash on bootloader reading it back and comparing it against the original image
bool Bootloader::_verifyBytes(QSerialPort* port, const FirmwareImage* image)
{
    if (image->imageIsBinFormat()) {
        return _binVerifyBytes(port, image);
    } else {
        return _ihxVerifyBytes(port, image);
    }
}

bool Bootloader::_binVerifyBytes(QSerialPort* port, const FirmwareImage* image)
{
    Q_ASSERT(image->imageIsBinFormat());
    
    QFile firmwareFile(image->binFilename());
    if (!firmwareFile.open(QIODevice::ReadOnly)) {
        _errorString = tr("Unable to open firmware file %1: %2").arg(image->binFilename(), firmwareFile.errorString());
        return false;
    }
    uint32_t imageSize = (uint32_t)firmwareFile.size();
    
    if (!_sendCommand(port, PROTO_CHIP_VERIFY)) {
        return false;
    }
    
    uint8_t fileBuf[READ_MULTI_MAX];
    uint8_t readBuf[READ_MULTI_MAX];
    uint32_t bytesVerified = 0;
    
    Q_ASSERT(PROG_MULTI_MAX <= 0x8F);
    
    while (bytesVerified < imageSize) {
        int bytesToRead = imageSize - bytesVerified;
        if (bytesToRead > (int)sizeof(readBuf)) {
            bytesToRead = (int)sizeof(readBuf);
        }
        
        Q_ASSERT((bytesToRead % 4) == 0);
        
        int bytesRead = firmwareFile.read((char *)fileBuf, bytesToRead);
        if (bytesRead == -1 || bytesRead != bytesToRead) {
            _errorString = tr("Firmware file read failed: %1").arg(firmwareFile.errorString());
            return false;
        }
        
        Q_ASSERT(bytesToRead <= 0x8F);
        
        bool failed = true;
        if (_write(port, PROTO_READ_MULTI) &&
            _write(port, (uint8_t)bytesToRead) &&
            _write(port, PROTO_EOC)) {
            port->flush();
            if (_read(port, readBuf, bytesToRead)) {
                if (_getCommandResponse(port)) {
                    failed = false;
                }
            }
        }
        if (failed) {
            _errorString = tr("Read failed: %1 at address: 0x%2").arg(_errorString).arg(bytesVerified, 8, 16, QLatin1Char('0'));
            return false;
        }

        for (int i=0; i<bytesToRead; i++) {
            if (fileBuf[i] != readBuf[i]) {
                _errorString = tr("Compare failed: expected(0x%1) actual(0x%2) at address: 0x%3").arg(fileBuf[i], 2, 16, QLatin1Char('0')).arg(readBuf[i], 2, 16, QLatin1Char('0')).arg(bytesVerified + i, 8, 16, QLatin1Char('0'));
                return false;
            }
        }
        
        bytesVerified += bytesToRead;
        
        emit updateProgress(bytesVerified, imageSize);
    }
    
    firmwareFile.close();
    
    return true;
}

bool Bootloader::_ihxVerifyBytes(QSerialPort* port, const FirmwareImage* image)
{
    Q_ASSERT(!image->imageIsBinFormat());
    
    uint32_t imageSize = image->imageSize();
    uint32_t bytesVerified = 0;
    
    for (uint16_t index=0; index<image->ihxBlockCount(); index++) {
        bool        failed;
        uint16_t    readAddress;
        QByteArray  imageBytes;
        
        if (!image->ihxGetBlock(index, readAddress, imageBytes)) {
            _errorString = tr("Unable to retrieve block from ihx: index %1").arg(index);
            return false;
        }
        
        qCDebug(FirmwareUpgradeLog) << QString("Bootloader::_ihxVerifyBytes - address:0x%1 size:%2 block:%3").arg(readAddress, 8, 16, QLatin1Char('0')).arg(imageBytes.count()).arg(index);
        
        // Set read address
        
        failed = true;
        if (_write(port, PROTO_LOAD_ADDRESS) &&
            _write(port, readAddress & 0xFF) &&
            _write(port, (readAddress >> 8) & 0xFF) &&
            _write(port, PROTO_EOC)) {
            port->flush();
            if (_getCommandResponse(port)) {
                failed = false;
            }
        }
        
        if (failed) {
            _errorString = tr("Unable to set read start address: 0x%2").arg(readAddress, 8, 16, QLatin1Char('0'));
            return false;
        }
        
        // Read back
        
        int         bytesIndex = 0;
        uint16_t    bytesLeftToRead = imageBytes.count();
        
        while (bytesLeftToRead > 0) {
            uint8_t bytesToRead;
            uint8_t readBuf[READ_MULTI_MAX];
            
            if (bytesLeftToRead > READ_MULTI_MAX) {
                bytesToRead = READ_MULTI_MAX;
            } else {
                bytesToRead = bytesLeftToRead;
            }

            failed = true;
            if (_write(port, PROTO_READ_MULTI) &&
                _write(port, bytesToRead) &&
                _write(port, PROTO_EOC)) {
                port->flush();
                if (_read(port, readBuf, bytesToRead)) {
                    if (_getCommandResponse(port)) {
                        failed = false;
                    }
                }
            }
            if (failed) {
                _errorString = tr("Read failed: %1 at address: 0x%2").arg(_errorString).arg(readAddress, 8, 16, QLatin1Char('0'));
                return false;
            }
            
            // Compare
            
            for (int i=0; i<bytesToRead; i++) {
                if ((uint8_t)imageBytes[bytesIndex + i] != readBuf[i]) {
                    _errorString = tr("Compare failed: expected(0x%1) actual(0x%2) at address: 0x%3").arg(imageBytes[bytesIndex + i], 2, 16, QLatin1Char('0')).arg(readBuf[i], 2, 16, QLatin1Char('0')).arg(readAddress + i, 8, 16, QLatin1Char('0'));
                    return false;
                }
            }
            
            bytesVerified += bytesToRead;
            bytesIndex += bytesToRead;
            bytesLeftToRead -= bytesToRead;
            
            emit updateProgress(bytesVerified, imageSize);
        }
    }
    
    return true;
}

/// @Brief Verify the flash by comparing CRCs.
bool Bootloader::_verifyCRC(QSerialPort* port)
{
    uint8_t buf[2] = { PROTO_GET_CRC, PROTO_EOC };
    quint32 flashCRC;
    
    bool failed = true;
    if (_write(port, buf, 2)) {
        port->flush();
        if (_read(port, (uint8_t*)&flashCRC, sizeof(flashCRC), _verifyTimeout)) {
            if (_getCommandResponse(port)) {
                failed = false;
            }
        }
    }
    if (failed) {
        return false;
    }

    if (_imageCRC != flashCRC) {
        _errorString = tr("CRC mismatch: board(0x%1) file(0x%2)").arg(flashCRC, 4, 16, QLatin1Char('0')).arg(_imageCRC, 4, 16, QLatin1Char('0'));
        return false;
    }
    
    return true;
}

bool Bootloader::open(QSerialPort* port, const QString portName)
{
    Q_ASSERT(!port->isOpen());
    
    port->setPortName(portName);
    port->setBaudRate(QSerialPort::Baud115200);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
    
    if (!port->open(QIODevice::ReadWrite)) {
        _errorString = tr("Open failed on port %1: %2").arg(portName, port->errorString());
        return false;
    }
    
    return true;
}

bool Bootloader::sync(QSerialPort* port)
{
    // Send sync command
    if (_sendCommand(port, PROTO_GET_SYNC)) {
        return true;
    } else {
        _errorString.prepend("Sync: ");
        return false;
    }
}

bool Bootloader::getPX4BoardInfo(QSerialPort* port, uint32_t& bootloaderVersion, uint32_t& boardID, uint32_t& flashSize)
{
    
    if (!_getPX4BoardInfo(port, INFO_BL_REV, _bootloaderVersion)) {
        goto Error;
    }
    if (_bootloaderVersion < BL_REV_MIN || _bootloaderVersion > BL_REV_MAX) {
        _errorString = tr("Found unsupported bootloader version: %1").arg(_bootloaderVersion);
        goto Error;
    }
    
    if (!_getPX4BoardInfo(port, INFO_BOARD_ID, _boardID)) {
        goto Error;
    }
    
    if (!_getPX4BoardInfo(port, INFO_FLASH_SIZE, _boardFlashSize)) {
        qWarning() << _errorString;
        goto Error;
    }

    // Older V2 boards have large flash space but silicon error which prevents it from being used. Bootloader v5 and above
    // will correctly account/report for this. Older bootloaders will not. Newer V2 board which support larger flash space are
    // reported as V3 board id.
    if (_boardID == boardIDPX4FMUV2 && _bootloaderVersion >= _bootloaderVersionV2CorrectFlash && _boardFlashSize > _flashSizeSmall) {
        _boardID = boardIDPX4FMUV3;
    }
    
    bootloaderVersion = _bootloaderVersion;
    boardID = _boardID;
    flashSize = _boardFlashSize;
    
    return true;
    
Error:
    _errorString.prepend(tr("Get Board Info: "));
    return false;
}

bool Bootloader::get3DRRadioBoardId(QSerialPort* port, uint32_t& boardID)
{
    uint8_t buf[2] = { PROTO_GET_DEVICE, PROTO_EOC };
    
    if (!_write(port, buf, sizeof(buf))) {
        goto Error;
    }
    port->flush();
    
    if (!_read(port, (uint8_t*)buf, 2)) {
        goto Error;
    }
    if (!_getCommandResponse(port)) {
        goto Error;
    }
    
    boardID = buf[0];
    
    _bootloaderVersion = 0;
    _boardFlashSize = 0;
    
    return true;
    
Error:
    _errorString.prepend(tr("Get Board Id: "));
    return false;
}

bool Bootloader::reboot(QSerialPort* port)
{
    bool success = _write(port, PROTO_BOOT) && _write(port, PROTO_EOC);
    if (success) {
        port->waitForBytesWritten(100);
    }
    return success;
}
