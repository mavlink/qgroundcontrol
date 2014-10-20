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

#include "PX4Bootloader.h"

#include <QFile>
#include <QSerialPortInfo>
#include <QDebug>

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

// FIXME: QIODevice has error string
const struct PX4Bootloader::serialPortErrorString PX4Bootloader::_rgSerialPortErrors[14] = {
    { QSerialPort::NoError,                     "No error occurred." },
    { QSerialPort::DeviceNotFoundError,         "An error occurred while attempting to open a non-existing device." },
    { QSerialPort::PermissionError,             "An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open." },
    { QSerialPort::OpenError,                   "An error occurred while attempting to open an already opened device in this object." },
    { QSerialPort::NotOpenError,                "This error occurs when an operation is executed that can only be successfully performed if the device is open." },
    { QSerialPort::ParityError,                 "Parity error detected by the hardware while reading data." },
    { QSerialPort::FramingError,                "Framing error detected by the hardware while reading data." },
    { QSerialPort::BreakConditionError,         "Break condition detected by the hardware on the input line." },
    { QSerialPort::WriteError,                  "An I/O error occurred while writing the data." },
    { QSerialPort::ReadError,                   "An I/O error occurred while reading the data." },
    { QSerialPort::ResourceError,               "An I/O error occurred when a resource becomes unavailable, e.g. when the device is unexpectedly removed from the system." },
    { QSerialPort::UnsupportedOperationError,   "The requested device operation is not supported or prohibited by the running operating system." },
    { QSerialPort::TimeoutError,                "A timeout error occurred." },
    { QSerialPort::UnknownError,                "An unidentified error occurred." }
};

/// @Brief Constructs a new PX4Bootloader Widget. This widget is used within the PX4VehicleConfig set of screens.
PX4Bootloader::PX4Bootloader(QObject *parent) :
    QObject(parent)
{

}

PX4Bootloader::~PX4Bootloader(void)
{
    if (_port.isOpen()) {
        _port.close();
    }
}

/// @brief Translate a QSerialPort::SerialPortError code into a string.
const char* PX4Bootloader::_serialPortErrorString(int error)
{
Again:
    for (size_t i=0; i<sizeof(_rgSerialPortErrors)/sizeof(_rgSerialPortErrors[0]); i++) {
        if (error == _rgSerialPortErrors[i].error) {
            return _rgSerialPortErrors[i].errorString;
        }
    }
    
    error = QSerialPort::UnknownError;
    goto Again;
    
    Q_ASSERT(false);
    
    return NULL;
}

bool PX4Bootloader::write(const uint8_t* data, qint64 maxSize)
{
    Q_ASSERT(_port.isOpen());
    
    qint64 bytesWritten = _port.write((const char*)data, maxSize);
    if (bytesWritten == -1) {
        _errorString = tr("Write failed: %1").arg(_port.errorString());
        return false;
    }
    if (bytesWritten != maxSize) {
        _errorString = tr("Incorrect number of bytes returned for write: actual(%1) expected(%2)").arg(bytesWritten).arg(maxSize);
        return false;
    }
    if (!_port.waitForBytesWritten(1000)) {
        _errorString = tr("Timeout waiting for write");
        return false;
    }
    
    return true;
}

bool PX4Bootloader::write(const uint8_t byte)
{
    uint8_t buf[1] = { byte };
    return write(buf, 1);
}

bool PX4Bootloader::read(uint8_t* data, qint64 maxSize, int readTimeout)
{
    qint64 bytesRead;
    
    if (_port.bytesAvailable() < maxSize) {
        if (!_port.waitForReadyRead(readTimeout)) {
            _errorString = tr("Timeout waiting for read bytes available: %1").arg(_port.errorString());
            return false;
        }
    }
    
    bytesRead = _port.read((char*)data, maxSize);
    if (bytesRead == -1) {
        _errorString = tr("Read failed: Could not read %1 resonse, error: 12").arg(_port.errorString());
        return false;
    }
    if (bytesRead != maxSize) {
        _errorString = tr("In correct number of bytes returned for read: actual(%1) expected(%2)").arg(bytesRead).arg(maxSize);
        return false;
    }
    
    return true;
}

bool PX4Bootloader::getCommandResponse(int responseTimeout)
{
    uint8_t response[2];
    
    // Read command response
    if (!read(response, 2, responseTimeout)) {
        return false;
    }
    
    // Make sure the correct sync byte
    if (response[0] != PROTO_INSYNC || response[1] != PROTO_OK) {
        _errorString = tr("Invalid sync response: %1%2").arg((int)response[0], 0, 16).arg((int)response[1], 0, 16);
        return false;
    }
    
    return true;
}

bool PX4Bootloader::getBoardInfo(uint8_t param, uint32_t& value)
{
    uint8_t buf[3] = { PROTO_GET_DEVICE, param, PROTO_EOC };
    
    if (!write(buf, sizeof(buf))) {
        return false;
    }
    if (!read((uint8_t*)&value, sizeof(value))) {
        return false;
    }
    return getCommandResponse();
}

bool PX4Bootloader::sendCommand(const uint8_t cmd, int responseTimeout)
{
    uint8_t buf[2] = { cmd, PROTO_EOC };
    
    if (!write(buf, 2)) {
        return false;
    }
    return getCommandResponse(responseTimeout);
}

bool PX4Bootloader::program(const QString& firmwareFilename)
{
    Q_ASSERT(_port.isOpen());
    
    // Erase is slow, need larger timeout
    if (!sendCommand(PROTO_CHIP_ERASE, 20000)) {
        _errorString = tr("Board erase failed: %1").arg(_errorString);
        return false;
    }
    
    QFile firmwareFile(firmwareFilename);
    if (!firmwareFile.open(QIODevice::ReadOnly)) {
        _errorString = tr("Unable to open firmware file %1: %2").arg(firmwareFilename).arg(firmwareFile.errorString());
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
        
        int bytesWritten = firmwareFile.read((char *)imageBuf, bytesToSend);
        if (bytesWritten == -1 || bytesWritten != bytesToSend) {
            // FIXME: Better error handling
            _errorString = tr("Read failed: %1").arg(firmwareFile.errorString());
            return false;
        }
        
        Q_ASSERT(bytesToSend <= 0x8F);
        
        // FIXME: Error handling
        if (!write(PROTO_PROG_MULTI) ||
            !write((uint8_t)bytesToSend) ||
            !write(imageBuf, bytesToSend) ||
            !write(PROTO_EOC) ||
            !getCommandResponse()) {
            _errorString = tr("Flash failed: %1").arg(_errorString);
            return false;
        }
        
        bytesSent += bytesToSend;
        
        // Calculate the CRC now so we can test it after the board is flashed.
        _imageCRC = crc32((uint8_t *)imageBuf, bytesToSend, _imageCRC);
        
        emit updateProgramProgress(bytesSent, imageSize);
    }
    firmwareFile.close();
    
    // We calculate the CRC using the entire flash size, filling the remainder with 0xFF. We need to do this
    // because we don't know the amount of the flash that is taken up with real code bytes after it is written.
    while (bytesSent < _boardFlashSize) {
        const uint8_t fill = 0xFF;
        _imageCRC = crc32(&fill, 1, _imageCRC);
        bytesSent++;
    }
    
#if 0
    if (_bootloaderVersion <= 2) {
        _bootloaderVerifyRev2();
    } else {
        _bootloaderVerifyRev3();
    }
#endif
    
    return true;
}

#if 0

/// @brief Verify the flash on bootloader version 2 by reading it back and comparing it against
/// the original firmware file.
bool PX4Bootloader::_bootloaderVerifyRev2(void)
{
#if 0
    uint8_t	file_buf[4];
    int count;
    int ret;
    size_t sent = 0;
    
    log("verify...");
    _fw_fd.seek(0);
    
    send(PROTO_CHIP_VERIFY);
    send(PROTO_EOC);
    ret = get_sync();
    
    if (ret != OK)
        return ret;
    
    while (sent < fw_size) {
        /* get more bytes to verify */
        size_t n = fw_size - sent;
        if (n > sizeof(file_buf)) {
            n = sizeof(file_buf);
        }
        count = read_with_retry(_fw_fd, file_buf, n);
        
        if (count != (int)n) {
            log("firmware read of %u bytes at %u failed -> %d errno %d",
                (unsigned)n,
                (unsigned)sent,
                (int)count,
                (int)errno);
        }
        
        if (count == 0)
            break;
        
        sent += count;
        
        if (count < 0)
            return -errno;
        
        Q_ASSERT((count % 4) == 0);
        
        send(PROTO_READ_MULTI);
        send(count);
        send(PROTO_EOC);
        
        // XXX read more than one byte here
        
        for (int i = 0; i < count; i++) {
            uint8_t c;
            
            ret = recv(c, 5000);
            
            if (ret != OK) {
                log("%d: got %d waiting for bytes", sent + i, ret);
                return ret;
            }
            
            if (c != file_buf[i]) {
                log("%d: got 0x%02x expected 0x%02x", sent + i, c, file_buf[i]);
                return -EINVAL;
            }
        }
        
        ret = get_sync();
        
        /* verify with bootloader rev 2 55%...100% */
        emit upgradeProgressChanged(55 + (int)(((float)sent/(float)fw_size)*45.0f));
        
        if (ret != OK) {
            log("timeout waiting for post-verify sync");
            return ret;
        }
    }
    
    emit upgradeProgressChanged(100);
    
    return OK;
#endif
    return false;
}

/// @Brief Verify the flash on a version 3 or higher bootloader board by comparing CRCs.
bool PX4Bootloader::_bootloaderVerifyRev3(void)
{
    quint32 flashCRC;
    
    write(buf, 2, "PROTO_GET_CRC");
    read((flashCRC, sizeof(crc), "PROTO_GET_CRC"));
    
    if (_imageCRC != flashCRC) {
        // FIXME: Error message
        return false;
    }
    
    return true;
}
#endif

bool PX4Bootloader::open(const QString portName)
{
    Q_ASSERT(!_port.isOpen());
    
    _port.setPortName(portName);
    _port.setBaudRate(QSerialPort::Baud115200);
    _port.setDataBits(QSerialPort::Data8);
    _port.setParity(QSerialPort::NoParity);
    _port.setStopBits(QSerialPort::OneStop);
    _port.setFlowControl(QSerialPort::NoFlowControl);
    
    if (!_port.open(QIODevice::ReadWrite)) {
        _errorString = tr("Open failed on port %1: %2").arg(portName).arg(_serialPortErrorString(_port.error()));
        return false;
    }
    
    return true;
}

bool PX4Bootloader::sync(void)
{
    Q_ASSERT(_port.isOpen());

    // Drain out any remaining input or output from the port
    if (!_port.clear()) {
        _errorString = tr("Unable to clear port");
        return false;
    }
    
    // Send sync command
    return sendCommand(PROTO_GET_SYNC);
}

bool PX4Bootloader::getBoardInfo(uint32_t& bootloaderVersion, uint32_t& boardID, uint32_t& flashSize)
{
    
    if (!getBoardInfo(INFO_BL_REV, _bootloaderVersion)) {
        goto Error;
    }
    if (_bootloaderVersion < BL_REV_MIN || _bootloaderVersion > BL_REV_MAX) {
        _errorString = tr("Found unsupported bootloader version: %1").arg(_bootloaderVersion);
        goto Error;
    }
    
    if (!getBoardInfo(INFO_BOARD_ID, _boardID)) {
        goto Error;
    }
    if (_boardID != _boardIDPX4Flow && _boardID != _boardIDPX4FMUV1 && _boardID != _boardIDPX4FMUV2) {
        _errorString = tr("Unsupported board: %1").arg(_boardID);
        goto Error;
    }
    
    if (!getBoardInfo(INFO_FLASH_SIZE, _boardFlashSize)) {
        goto Error;
    }
    
    bootloaderVersion = _bootloaderVersion;
    boardID = _boardID;
    flashSize = _boardFlashSize;
    
    return true;
    
Error:
    close();
    return false;
}

void PX4Bootloader::close(void)
{
    Q_ASSERT(_port.isOpen());
    _port.close();
}

void PX4Bootloader::sendFMUReboot(const QString portName)
{
    qDebug() << "Rebooting board";
    
    Q_ASSERT(!_port.isOpen());
    
    _port.setPortName(portName);
    _port.setBaudRate(QSerialPort::Baud115200);
    _port.setDataBits(QSerialPort::Data8);
    _port.setParity(QSerialPort::NoParity);
    _port.setStopBits(QSerialPort::OneStop);
    _port.setFlowControl(QSerialPort::NoFlowControl);
    
    if (_port.open(QIODevice::ReadWrite)) {
        
        // Send command to reboot via NSH, then via MAVLink
        // XXX hacky but safe
        // Start NSH
        const uint8_t init[] = {0x0d, 0x0d, 0x0d};
        write(init, sizeof(init));
        
        // Reboot into bootloader
        const char* cmd = "reboot -b\n";
        write((uint8_t *)cmd, strlen(cmd));
        write(init, 2);
        
        // Old reboot command
        const char* cmd_old = "reboot\n";
        write((uint8_t *)cmd_old, strlen(cmd_old));
        write(init, 2);
    
        // FIXME: Don't think these are needed any more
#if 0
#ifdef Q_OS_WIN
#pragma warning (push)
#pragma warning (disable:4309)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#endif
#endif
    
        // Reboot via MAVLink (if enabled)
        // Try system ID 1
        const uint8_t mavlink_msg_id1[] = {0xfe,0x21,0x72,0xff,0x00,0x4c,0x00,0x00,0x80,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf6,0x00,0x01,0x00,0x00,0x48,0xf0};
        write(mavlink_msg_id1, sizeof(mavlink_msg_id1));
        // Try system ID 0 (broadcast)
        const uint8_t mavlink_msg_id0[] = {0xfe,0x21,0x45,0xff,0x00,0x4c,0x00,0x00,0x80,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf6,0x00,0x00,0x00,0x00,0xd7,0xac};
        write(mavlink_msg_id0, sizeof(mavlink_msg_id0));
#if 0
#ifdef Q_OS_WIN
#pragma warning (pop)
#else
#pragma GCC diagnostic pop
#endif
#endif
        
        // FIXME: Need timeout on this
        while (_port.pinoutSignals() != QSerialPort::NoSignal) {
        }

        close();
    } else {
        qDebug() << "Open failed for reboot" << _serialPortErrorString(_port.error());
    }
}
