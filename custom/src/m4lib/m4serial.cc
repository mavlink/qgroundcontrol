/**
 * @file SerialComm.cc
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#include "m4serial.h"
#include "m4util.h"

#if defined(__androidx86__)
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#endif

//-----------------------------------------------------------------------------
M4SerialComm::M4SerialComm(QObject* parent)
    : QObject(parent)
    , _fd(-1)
    , _baudrate(230400)
    , _status(SERIAL_PORT_CLOSED)
    , _currentPacketStatus(PACKET_NONE)
{

}

//-----------------------------------------------------------------------------
M4SerialComm::~M4SerialComm()
{
    close();
}

//-----------------------------------------------------------------------------
bool
M4SerialComm::init(QString port, int baud)
{
    _uart_name = port;
    _baudrate  = baud;
    return true;
}

//-----------------------------------------------------------------------------
bool
M4SerialComm::open()
{
#if defined(__androidx86__)
    if(_status != SERIAL_PORT_CLOSED || _fd >= 0) {
        return false;
    }
    _fd = _openPort(_uart_name.toLatin1().data());
    if(_fd < 0) {
        perror("SERIAL");
        qCDebug(YuneecLog) << "SERIAL: Could not open port" << _uart_name;
        return false;
    }
  //tcgetattr(_fd , &_savedtio);
    if(!_setupPort(_baudrate)) {
        return false;
    }
    _status = SERIAL_PORT_OPEN;
#endif
    return true;
}

//-----------------------------------------------------------------------------
void
M4SerialComm::close()
{
#if defined(__androidx86__)
    _status = SERIAL_PORT_CLOSED;
    if(_fd >= 0) {
      //tcsetattr(_fd, TCSANOW, &_savedtio);
        ::close(_fd);
    }
    //if(!wait(1000)) {
    //    qCDebug(YuneecLog) << "SERIAL: Timeout waiting for thread to end";
    //}
    _fd = -1;
#endif
}

//-----------------------------------------------------------------------------
bool M4SerialComm::write(QByteArray data, bool debug)
{
    if(debug) {
        qCDebug(YuneecLog) << data.toHex();
    }
    return _writePort(data.data(), data.size()) == data.length();
}

//-----------------------------------------------------------------------------
bool M4SerialComm::write(void* data, int length)
{
    return _writePort(data, length) == length;
}

//-----------------------------------------------------------------------------
void
M4SerialComm::tryRead()
{
#if defined(__androidx86__)
    if(_status == SERIAL_PORT_OPEN) {
        uint8_t b;
        if(::read(_fd, &b, 1) == 1) {
            switch (_currentPacketStatus) {
                case PACKET_NONE:
                    if(b == 0x55) {
                        _currentPacketStatus = PACKET_FIRST_ID;
                    }
                    break;
                case PACKET_FIRST_ID:
                    if(b == 0x55) {
                        _currentPacketStatus = PACKET_SECOND_ID;
                    } else {
                        _currentPacketStatus = PACKET_NONE;
                    }
                    break;
                case PACKET_SECOND_ID:
                    _readPacket(b);
                    _currentPacketStatus = PACKET_NONE;
                    break;
            }
        }
    }
#endif
}

//-----------------------------------------------------------------------------
bool
M4SerialComm::_readData(void *buffer, int len)
{
#if defined(__androidx86__)
    int tries = 0;
    int left  = len;
    uint8_t* ptr = (uint8_t*)buffer;
    while(left > 0) {
        int count = ::read(_fd, ptr, left);
        if(count < 0 || _status != SERIAL_PORT_OPEN || _fd < 0) {
            return false;
        }
        left -= count;
        ptr  += count;
        if(++tries > 5) {
            return false;
        }
    }
    return true;
#else
    Q_UNUSED(buffer);
    Q_UNUSED(len);
    return false;
#endif
}

//-----------------------------------------------------------------------------
void
M4SerialComm::_readPacket(uint8_t length)
{
#if defined(__androidx86__)
    if(length > 1) {
        length--; //-- Skip CRC from count
        uint8_t buffer[260];
        if(_readData(buffer, length)) {
            //-- CRC is appended to end of data block
            uint8_t iCRC;
            if(::read(_fd, &iCRC, 1) == 1) {
                uint8_t oCRC = crc8(buffer, length);
                if(iCRC == oCRC) {
                    QByteArray data((const char*)buffer, length);
                    emit bytesReady(data);
                } else {
                    qCDebug(YuneecLog) << "Bad CRC" << length << iCRC << oCRC;
                }
            } else {
                qCDebug(YuneecLog) << "Missed CRC";
            }
        } else {
            qCDebug(YuneecLog) << "Missed message payload";
        }
    }
#else
    Q_UNUSED(length);
#endif
}

//-----------------------------------------------------------------------------
int
M4SerialComm::_openPort(const char* port)
{
#if defined(__androidx86__)
    int fd = ::open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd >= 0) {
        fcntl(fd, F_SETFL, 0);
    }
    return fd;
#else
    Q_UNUSED(port);
    return 0;
#endif
}

//-----------------------------------------------------------------------------
bool
M4SerialComm::_setupPort(int baud)
{
#if defined(__androidx86__)
    struct termios config;
    bzero(&config, sizeof(config));
    config.c_cflag |= (CS8 | CLOCAL | CREAD);
    config.c_cc[VMIN]  = 1;
    config.c_cc[VTIME] = 5;
    bool baudError = false;
    switch(baud) {
        case 9600:
            cfsetispeed(&config, B9600);
            cfsetospeed(&config, B9600);
            break;
        case 19200:
            cfsetispeed(&config, B19200);
            cfsetospeed(&config, B19200);
            break;
        case 38400:
            baudError = (cfsetispeed(&config, B38400) < 0 || cfsetospeed(&config, B38400) < 0);
            break;
        case 57600:
            baudError = (cfsetispeed(&config, B57600) < 0 || cfsetospeed(&config, B57600) < 0);
            break;
        case 115200:
            baudError = (cfsetispeed(&config, B115200) < 0 || cfsetospeed(&config, B115200) < 0);
            break;
        case 230400:
            baudError = (cfsetispeed(&config, B230400) < 0 || cfsetospeed(&config, B230400) < 0);
            break;
        default:
            baudError = true;
            break;
    }
    if(baudError) {
        qCWarning(YuneecLog) << "SERIAL: Could not set baud rate of" << baud;
        return false;
    }
    tcflush(_fd, TCIFLUSH);
    if(tcsetattr(_fd, TCSANOW, &config) < 0) {
        qCWarning(YuneecLog) << "SERIAL: Could not set serial configuration";
        return false;
    }
    return true;
#else
    Q_UNUSED(baud)
    return true;
#endif
}

//-----------------------------------------------------------------------------
int
M4SerialComm::_writePort(void* buffer, int len)
{
#if defined(__androidx86__)
    int written = ::write(_fd, buffer, len);
    if(written != len && written >= 0) {
        qCWarning(YuneecLog) << QString("SERIAL: Wrote only %1 bytes out of %2 bytes").arg(written).arg(len);
    }
    return written;
#else
    Q_UNUSED(buffer);
    Q_UNUSED(len);
    return len;
#endif
}
