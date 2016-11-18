/****************************************************************************
 *
 * Copyright (c) 2015, 2016 Gus Grubba. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file SerialComm.cc
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#include "SerialComm.h"
#include "m4.h"

#include <QDebug>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

//-----------------------------------------------------------------------------
M4SerialComm::M4SerialComm(QObject* parent)
    : QThread(parent)
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
    if(_status != SERIAL_PORT_CLOSED || _fd >= 0) {
        return false;
    }
    _fd = _openPort(_uart_name.toLatin1().data());
    if(_fd < 0) {
        perror("SERIAL");
        qDebug() << "SERIAL: Could not open port" << _uart_name;
        return false;
    }
    tcgetattr(_fd , &_savedtio);
    if(!_setupPort(_baudrate)) {
        return false;
    }
    _status = SERIAL_PORT_OPEN;
    //-- Start reading thread
#if defined(__android__)
    start();
#endif
    return true;
}

//-----------------------------------------------------------------------------
void
M4SerialComm::close()
{
    _status = SERIAL_PORT_CLOSED;
    if(_fd >= 0) {
        tcsetattr(_fd, TCSANOW, &_savedtio);
        ::close(_fd);
        _fd = -1;
    }
}

//-----------------------------------------------------------------------------
bool M4SerialComm::write(QByteArray data)
{
    return _writePort(data.data(), data.size()) == data.length();
}

//-----------------------------------------------------------------------------
bool M4SerialComm::write(void* data, int length)
{
    return _writePort(data, length) == length;
}

//-----------------------------------------------------------------------------
void
M4SerialComm::run()
{
    while(_status == SERIAL_PORT_OPEN) {
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
    qDebug() << "SERIAL: Exiting thread";
}

//-----------------------------------------------------------------------------
void
M4SerialComm::_readPacket(uint8_t length)
{
    uint8_t buffer[260];
    buffer[0] = 0x55;
    buffer[1] = 0x55;
    buffer[2] = length;
    //-- CRC is appended to end of data block
    if(::read(_fd, &buffer[3], length + 1) == length + 1) {
        uint8_t crc = M4Controller::crc8(&buffer[3], length);
        if(crc == buffer[length]) {
            QByteArray data((const char*)buffer, 3 + length); //-- 3 bytes in header plus data
            emit bytesReady(data);
        } else {
            qDebug() << "Bad CRC" << length << buffer[3] << buffer[4] << buffer[5] << buffer[6];
        }
    }
}

//-----------------------------------------------------------------------------
int
M4SerialComm::_openPort(const char* port)
{
    int fd = ::open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd >= 0) {
        fcntl(fd, F_SETFL, 0);
    }
    return fd;
}

//-----------------------------------------------------------------------------
bool
M4SerialComm::_setupPort(int baud)
{
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
        qWarning() << "SERIAL: Could not set baud rate of" << baud;
        return false;
    }
    tcflush(_fd, TCIFLUSH);
    if(tcsetattr(_fd, TCSANOW, &config) < 0) {
        qWarning() << "SERIAL: Could not set serial configuration";
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
int
M4SerialComm::_writePort(void* buffer, int len)
{
#if defined(__android__)
    int written = ::write(_fd, buffer, len);
    if(written != len && written >= 0) {
        qWarning() << QString("SERIAL: Wrote only %1 bytes out of %2 bytes").arg(written).arg(len);
    }
    return written;
#else
    Q_UNUSED(buffer);
    Q_UNUSED(len);
    return true;
#endif
}
