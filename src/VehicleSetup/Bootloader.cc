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

#include "Bootloader.h"
#include "QGCLoggingCategory.h"

#include <QFile>
#include <QSerialPortInfo>
#include <QDebug>
#include <QTime>

#include "QGC.h"

static const quint32 crctab[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static quint32 crc32(const uint8_t *src, unsigned len, unsigned state)
{
    for (unsigned i = 0; i < len; i++) {
        state = crctab[(state ^ src[i]) & 0xff] ^ (state >> 8);
    }
    return state;
}

Bootloader::Bootloader(QObject *parent) :
    QObject(parent)
{

}

bool Bootloader::_write(QextSerialPort* port, const uint8_t* data, qint64 maxSize)
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

bool Bootloader::_write(QextSerialPort* port, const uint8_t byte)
{
    uint8_t buf[1] = { byte };
    return _write(port, buf, 1);
}

bool Bootloader::_read(QextSerialPort* port, uint8_t* data, qint64 maxSize, int readTimeout)
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
            QGC::SLEEP::usleep(100);
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
bool Bootloader::_getCommandResponse(QextSerialPort* port, int responseTimeout)
{
    uint8_t response[2];
    
    if (!_read(port, response, 2, responseTimeout)) {
        _errorString.prepend("Get Command Response: ");
        return false;
    }
    
    // Make sure we get a good sync response
    if (response[0] != PROTO_INSYNC) {
        _errorString = tr("Invalid sync response: 0x%1 0x%2").arg(response[0], 2, 16, QLatin1Char('0')).arg(response[1], 2, 16, QLatin1Char('0'));
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
bool Bootloader::_getPX4BoardInfo(QextSerialPort* port, uint8_t param, uint32_t& value)
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
    _errorString.prepend("Get Board Info: ");
    return false;
}

/// Send a command to the bootloader
///     @param cmd Command to send using PROTO_* enums
/// @return true: Command sent and valid sync response returned
bool Bootloader::_sendCommand(QextSerialPort* port, const uint8_t cmd, int responseTimeout)
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
    _errorString.prepend("Send Command: ");
    return false;
}

bool Bootloader::erase(QextSerialPort* port)
{
    // Erase is slow, need larger timeout
    if (!_sendCommand(port, PROTO_CHIP_ERASE, _eraseTimeout)) {
        _errorString = tr("Board erase failed: %1").arg(_errorString);
        return false;
    }
    
    return true;
}

bool Bootloader::program(QextSerialPort* port, const FirmwareImage* image)
{
    if (image->imageIsBinFormat()) {
        return _binProgram(port, image);
    } else {
        return _ihxProgram(port, image);
    }
}

bool Bootloader::_binProgram(QextSerialPort* port, const FirmwareImage* image)
{
    QFile firmwareFile(image->binFilename());
    if (!firmwareFile.open(QIODevice::ReadOnly)) {
        _errorString = tr("Unable to open firmware file %1: %2").arg(image->binFilename()).arg(firmwareFile.errorString());
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
        _imageCRC = crc32((uint8_t *)imageBuf, bytesToSend, _imageCRC);
        
        emit updateProgress(bytesSent, imageSize);
    }
    firmwareFile.close();
    
    // We calculate the CRC using the entire flash size, filling the remainder with 0xFF.
    while (bytesSent < _boardFlashSize) {
        const uint8_t fill = 0xFF;
        _imageCRC = crc32(&fill, 1, _imageCRC);
        bytesSent++;
    }
    
    return true;
}

bool Bootloader::_ihxProgram(QextSerialPort* port, const FirmwareImage* image)
{
    uint32_t imageSize = image->imageSize();
    uint32_t bytesSent = 0;

    for (uint16_t index=0; index<image->ihxBlockCount(); index++) {
        bool        failed;
        uint16_t    flashAddress;
        QByteArray  bytes;
        
        if (!image->ihxGetBlock(index, flashAddress, bytes)) {
            _errorString = QString("Unable to retrieve block from ihx: index %1").arg(index);
            return false;
        }
        
        qCDebug(FirmwareUpgradeLog) << QString("Bootloader::_ihxProgram - address:%1 size:%2 block:%3").arg(flashAddress).arg(bytes.count()).arg(index);
        
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
            _errorString = QString("Unable to set flash start address: 0x%2").arg(flashAddress, 8, 16, QLatin1Char('0'));
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
                _errorString = QString("Flash failed: %1 at address 0x%2").arg(_errorString).arg(flashAddress, 8, 16, QLatin1Char('0'));
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

bool Bootloader::verify(QextSerialPort* port, const FirmwareImage* image)
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

/// @brief Verify the flash on bootloader eading it back and comparing it against the original image->
bool Bootloader::_verifyBytes(QextSerialPort* port, const FirmwareImage* image)
{
    if (image->imageIsBinFormat()) {
        return _binVerifyBytes(port, image);
    } else {
        return _ihxVerifyBytes(port, image);
    }
}

bool Bootloader::_binVerifyBytes(QextSerialPort* port, const FirmwareImage* image)
{
    Q_ASSERT(image->imageIsBinFormat());
    
    QFile firmwareFile(image->binFilename());
    if (!firmwareFile.open(QIODevice::ReadOnly)) {
        _errorString = tr("Unable to open firmware file %1: %2").arg(image->binFilename()).arg(firmwareFile.errorString());
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
            if (_read(port, readBuf, sizeof(readBuf))) {
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

bool Bootloader::_ihxVerifyBytes(QextSerialPort* port, const FirmwareImage* image)
{
    Q_ASSERT(!image->imageIsBinFormat());
    
    uint32_t imageSize = image->imageSize();
    uint32_t bytesVerified = 0;
    
    for (uint16_t index=0; index<image->ihxBlockCount(); index++) {
        bool        failed;
        uint16_t    readAddress;
        QByteArray  imageBytes;
        
        if (!image->ihxGetBlock(index, readAddress, imageBytes)) {
            _errorString = QString("Unable to retrieve block from ihx: index %1").arg(index);
            return false;
        }
        
        qCDebug(FirmwareUpgradeLog) << QString("Bootloader::_ihxVerifyBytes - address:%1 size:%2 block:%3").arg(readAddress).arg(imageBytes.count()).arg(index);
        
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
            _errorString = QString("Unable to set read start address: 0x%2").arg(readAddress, 8, 16, QLatin1Char('0'));
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
                    _errorString = QString("Compare failed: expected(0x%1) actual(0x%2) at address: 0x%3").arg(imageBytes[bytesIndex + i], 2, 16, QLatin1Char('0')).arg(readBuf[i], 2, 16, QLatin1Char('0')).arg(readAddress + i, 8, 16, QLatin1Char('0'));
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
bool Bootloader::_verifyCRC(QextSerialPort* port)
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

bool Bootloader::open(QextSerialPort* port, const QString portName)
{
    Q_ASSERT(!port->isOpen());
    
    port->setPortName(portName);
    port->setBaudRate(BAUD115200);
    port->setDataBits(DATA_8);
    port->setParity(PAR_NONE);
    port->setStopBits(STOP_1);
    port->setFlowControl(FLOW_OFF);
    
    if (!port->open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        _errorString = tr("Open failed on port %1: %2").arg(portName).arg(port->errorString());
        return false;
    }
    
    return true;
}

bool Bootloader::sync(QextSerialPort* port)
{
    // Send sync command
    if (_sendCommand(port, PROTO_GET_SYNC)) {
        return true;
    } else {
        _errorString.prepend("Sync: ");
        return false;
    }
}

bool Bootloader::getPX4BoardInfo(QextSerialPort* port, uint32_t& bootloaderVersion, uint32_t& boardID, uint32_t& flashSize)
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
    
    bootloaderVersion = _bootloaderVersion;
    boardID = _boardID;
    flashSize = _boardFlashSize;
    
    return true;
    
Error:
    _errorString.prepend("Get Board Info: ");
    return false;
}

bool Bootloader::get3DRRadioBoardId(QextSerialPort* port, uint32_t& boardID)
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
    _errorString.prepend("Get Board Id: ");
    return false;
}

bool Bootloader::reboot(QextSerialPort* port)
{
    return _write(port, PROTO_BOOT) && _write(port, PROTO_EOC);
}
