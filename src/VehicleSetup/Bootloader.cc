/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Bootloader.h"
#include "QGCLoggingCategory.h"

#include <QFile>
#include <QSerialPortInfo>
#include <QDebug>
#include <QElapsedTimer>

#include "QGC.h"

/// This class manages interactions with the bootloader
Bootloader::Bootloader(bool sikRadio, QObject *parent)
    : QObject   (parent)
    , _sikRadio (sikRadio)
{

}

bool Bootloader::open(const QString portName)
{
    qCDebug(FirmwareUpgradeLog) << "open:" << portName;

    _port.setPortName   (portName);
    _port.setBaudRate   (QSerialPort::Baud115200);
    _port.setDataBits   (QSerialPort::Data8);
    _port.setParity     (QSerialPort::NoParity);
    _port.setStopBits   (QSerialPort::OneStop);
    _port.setFlowControl(QSerialPort::NoFlowControl);

    if (!_port.open(QIODevice::ReadWrite)) {
        _errorString = tr("Open failed on port %1: %2").arg(portName, _port.errorString());
        return false;
    }

    if (_sikRadio) {
        // Radios are slow to start up
        QGC::SLEEP::msleep(1000);
    }
    return true;
}

QString Bootloader::_getNextLine(int timeoutMsecs)
{
    QString         line;
    QElapsedTimer   timeout;
    bool            foundCR = false;

    timeout.start();
    while (timeout.elapsed() < timeoutMsecs) {
        char oneChar;
        _port.waitForReadyRead(100);
        if (_port.read(&oneChar, 1) > 0) {
            if (oneChar == '\r') {
                foundCR = true;
                continue;
            } else if (oneChar == '\n' && foundCR) {
                return line;
            }
            line += oneChar;
        }
    }

    return QString();
}

bool Bootloader::getBoardInfo(uint32_t& bootloaderVersion, uint32_t& boardID, uint32_t& flashSize)
{
    if (_sikRadio) {
        // Try sync to see if already in bootloader mode
        _sync();
        if (_inBootloaderMode) {
            qCDebug(FirmwareUpgradeLog) << "Radio in bootloader mode already";
            if (!_get3DRRadioBoardId(_boardID)) {
                goto Error;
            }
        } else {
            qCDebug(FirmwareUpgradeLog) << "Radio in normal mode";
            _port.readAll();
            _port.setBaudRate(QSerialPort::Baud57600);
            // Put radio into command mode
            _write("+++");
            if (!_port.waitForReadyRead(2000)) {
                _errorString = tr("Unable to put radio into command mode +++");
                goto Error;
            }
            QByteArray bytes = _port.readAll();
            if (!bytes.contains("OK")) {
                _errorString = tr("Radio did not respond to command mode");
                goto Error;
            }

            // Use ATI2 command to get board id
            _write("ATI2\r\n");
            QString echo = _getNextLine(2000);
            if (echo.isEmpty() || echo != "ATI2") {
                _errorString = tr("Radio did not respond to ATI2 command");
                goto Error;
            }
            QString boardIdStr = _getNextLine(2000);
            bool ok = false;
            _boardID = boardIdStr.toInt(&ok);
            if (boardIdStr.isEmpty() || !ok) {
                _errorString = tr("Radio did not return board id");
                goto Error;
            }
        }
        bootloaderVersion   = 0;
        boardID             = _boardID;
        flashSize           = 0;

        return true;
    } else {
        if (!_sync()) {
            goto Error;
        }
        if (!_protoGetDevice(INFO_BL_REV, _bootloaderVersion)) {
            goto Error;
        }
        if (_bootloaderVersion < BL_REV_MIN || _bootloaderVersion > BL_REV_MAX) {
            _errorString = tr("Found unsupported bootloader version: %1").arg(_bootloaderVersion);
            goto Error;
        }
        if (!_protoGetDevice(INFO_BOARD_ID, _boardID)) {
            goto Error;
        }
        if (!_protoGetDevice(INFO_FLASH_SIZE, _boardFlashSize)) {
            qWarning() << _errorString;
            goto Error;
        }

        // Older V2 boards have large flash space but silicon error which prevents it from being used. Bootloader v5 and above
        // will correctly account/report for this. Older bootloaders will not. Newer V2 board which support larger flash space are
        // reported as V3 board id.
        if (_boardID == boardIDPX4FMUV2 && _bootloaderVersion >= _bootloaderVersionV2CorrectFlash && _boardFlashSize > _flashSizeSmall) {
            _boardID = boardIDPX4FMUV3;
        }

        bootloaderVersion   = _bootloaderVersion;
        boardID             = _boardID;
        flashSize           = _boardFlashSize;

        return true;
    }

Error:
    qCDebug(FirmwareUpgradeLog) << "getBoardInfo failed:" << _errorString;
    _errorString.prepend(tr("Get Board Info: "));
    return false;
}

bool Bootloader::initFlashSequence(void)
{
    if (_sikRadio && !_inBootloaderMode) {
        _write("AT&UPDATE\r\n");
        if (!_port.waitForReadyRead(1500)) {
            _errorString = tr("Unable to reboot radio (ready read)");
            return false;
        }
        _port.setBaudRate(QSerialPort::Baud115200);

        if (!_sync()) {
            return false;
        }
    }
    return true;
}

bool Bootloader::erase(void)
{
    // Erase is slow, need larger timeout
    if (!_sendCommand(PROTO_CHIP_ERASE, _eraseTimeout)) {
        _errorString = tr("Erase failed: %1").arg(_errorString);
        return false;
    }

    return true;
}

bool Bootloader::program(const FirmwareImage* image)
{
    if (image->imageIsBinFormat()) {
        return _binProgram(image);
    } else {
        return _ihxProgram(image);
    }
}

bool Bootloader::reboot(void)
{
    bool success;
    if (_sikRadio && !_inBootloaderMode) {
        qCDebug(FirmwareUpgradeLog) << "reboot ATZ";
        _port.readAll();
        success = _write("ATZ\r\n");
    } else {
        qCDebug(FirmwareUpgradeLog) << "reboot";
        success = _write(PROTO_BOOT) && _write(PROTO_EOC);
    }
    _port.flush();
    if (success) {
        QGC::SLEEP::msleep(1000);
    }
    return success;
}

bool Bootloader::_write(const char* data)
{
    return _write((uint8_t*)data, qstrlen(data));
}

bool Bootloader::_write(const uint8_t* data, qint64 maxSize)
{
    qint64 bytesWritten = _port.write((const char*)data, maxSize);
    if (bytesWritten == -1) {
        _errorString = tr("Write failed: %1").arg(_port.errorString());
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

bool Bootloader::_write(const uint8_t byte)
{
    uint8_t buf[1] = { byte };
    return _write(buf, 1);
}

bool Bootloader::_read(uint8_t* data, qint64 cBytesExpected, int readTimeout)
{
    QElapsedTimer timeout;

    timeout.start();
    while (_port.bytesAvailable() < cBytesExpected) {
        if (timeout.elapsed() > readTimeout) {
            _errorString = tr("Timeout waiting for bytes to be available");
            return false;
        }
        _port.waitForReadyRead(100);
    }

    qint64 bytesRead;
    bytesRead = _port.read((char *)data, cBytesExpected);

    if (bytesRead != cBytesExpected) {
        _errorString = tr("Read failed: error: %1").arg(_port.errorString());
        return false;
    }

    return true;
}

/// Read a PROTO_SYNC command response from the bootloader
///     @param responseTimeout Msecs to wait for response bytes to become available on port
bool Bootloader::_getCommandResponse(int responseTimeout)
{
    uint8_t response[2];
    
    if (!_read(response, 2, responseTimeout)) {
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
bool Bootloader::_protoGetDevice(uint8_t param, uint32_t& value)
{
    uint8_t buf[3] = { PROTO_GET_DEVICE, param, PROTO_EOC };
    
    if (!_write(buf, sizeof(buf))) {
        goto Error;
    }
    if (!_read((uint8_t*)&value, sizeof(value))) {
        goto Error;
    }
    if (!_getCommandResponse()) {
        goto Error;
    }
    
    return true;
    
Error:
    _errorString.prepend(tr("Get Device: "));
    return false;
}

/// Send a command to the bootloader
///     @param cmd Command to send using PROTO_* enums
/// @return true: Command sent and valid sync response returned
bool Bootloader::_sendCommand(const uint8_t cmd, int responseTimeout)
{
    uint8_t buf[2] = { cmd, PROTO_EOC };
    
    if (!_write(buf, 2)) {
        goto Error;
    }
    _port.flush();

    if (!_getCommandResponse(responseTimeout)) {
        goto Error;
    }
    
    return true;

Error:
    _errorString.prepend(tr("Send Command: "));
    return false;
}

bool Bootloader::_binProgram(const FirmwareImage* image)
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
        if (_write(PROTO_PROG_MULTI) &&
                _write((uint8_t)bytesToSend) &&
                _write(imageBuf, bytesToSend) &&
                _write(PROTO_EOC)) {
            if (_getCommandResponse()) {
                failed = false;
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

bool Bootloader::_ihxProgram(const FirmwareImage* image)
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
        if (_write(PROTO_LOAD_ADDRESS) &&
                _write(flashAddress & 0xFF) &&
                _write((flashAddress >> 8) & 0xFF) &&
                _write(PROTO_EOC)) {
            _port.flush();
            if (_getCommandResponse()) {
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
            if (_write(PROTO_PROG_MULTI) &&
                    _write(bytesToWrite) &&
                    _write(&((uint8_t *)bytes.data())[bytesIndex], bytesToWrite) &&
                    _write(PROTO_EOC)) {
                _port.flush();
                if (_getCommandResponse()) {
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

bool Bootloader::verify(const FirmwareImage* image)
{
    bool ret;
    
    if (!image->imageIsBinFormat() || _bootloaderVersion <= 2) {
        ret = _verifyBytes(image);
    } else {
        ret = _verifyCRC();
    }
    
    reboot();
    
    return ret;
}

/// @brief Verify the flash on bootloader reading it back and comparing it against the original image
bool Bootloader::_verifyBytes(const FirmwareImage* image)
{
    if (image->imageIsBinFormat()) {
        return _binVerifyBytes(image);
    } else {
        return _ihxVerifyBytes(image);
    }
}

bool Bootloader::_binVerifyBytes(const FirmwareImage* image)
{
    Q_ASSERT(image->imageIsBinFormat());
    
    QFile firmwareFile(image->binFilename());
    if (!firmwareFile.open(QIODevice::ReadOnly)) {
        _errorString = tr("Unable to open firmware file %1: %2").arg(image->binFilename(), firmwareFile.errorString());
        return false;
    }
    uint32_t imageSize = (uint32_t)firmwareFile.size();
    
    if (!_sendCommand(PROTO_CHIP_VERIFY)) {
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
        if (_write(PROTO_READ_MULTI) &&
                _write((uint8_t)bytesToRead) &&
                _write(PROTO_EOC)) {
            _port.flush();
            if (_read(readBuf, bytesToRead)) {
                if (_getCommandResponse()) {
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

bool Bootloader::_ihxVerifyBytes(const FirmwareImage* image)
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
        if (_write(PROTO_LOAD_ADDRESS) &&
                _write(readAddress & 0xFF) &&
                _write((readAddress >> 8) & 0xFF) &&
                _write(PROTO_EOC)) {
            _port.flush();
            if (_getCommandResponse()) {
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
            if (_write(PROTO_READ_MULTI) &&
                    _write(bytesToRead) &&
                    _write(PROTO_EOC)) {
                _port.flush();
                if (_read(readBuf, bytesToRead)) {
                    if (_getCommandResponse()) {
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
bool Bootloader::_verifyCRC(void)
{
    uint8_t buf[2] = { PROTO_GET_CRC, PROTO_EOC };

    quint32 flashCRC;
    
    bool failed = true;
    if (_write(buf, 2)) {
        _port.flush();
        if (_read((uint8_t*)&flashCRC, sizeof(flashCRC), _verifyTimeout)) {
            if (_getCommandResponse()) {
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

bool Bootloader::_syncWorker(void)
{
    // Send sync command
    if (_sendCommand(PROTO_GET_SYNC)) {
        _inBootloaderMode = true;
        return true;
    } else {
        _errorString.prepend("Sync: ");
        return false;
    }
}

bool Bootloader::_sync(void)
{
    // Sometimes getting sync is flaky, try 3 times
    _port.readAll();
    bool success = false;
    for (int i=0; i<3; i++) {
        success = _syncWorker();

        if (success) {
            return true;
        }
    }
    return success;
}

bool Bootloader::_get3DRRadioBoardId(uint32_t& boardID)
{
    uint8_t buf[2] = { PROTO_GET_DEVICE, PROTO_EOC };

    if (!_write(buf, sizeof(buf))) {
        goto Error;
    }
    _port.flush();

    if (!_read((uint8_t*)buf, 2)) {
        goto Error;
    }
    if (!_getCommandResponse()) {
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
