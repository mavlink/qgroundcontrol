/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UdpIODevice.h"
#include <algorithm>

UdpIODevice::UdpIODevice(QObject *parent) : QUdpSocket(parent)
{
    // this might cause data to be available only after a second readyRead() signal
    connect(this, &QUdpSocket::readyRead, this, &UdpIODevice::_readAvailableData);
}

bool UdpIODevice::canReadLine() const
{
    return _buffer.indexOf('\n') > -1;
}

qint64 UdpIODevice::readLineData(char *data, qint64 maxSize)
{
    int length = _buffer.indexOf('\n') + 1; // add 1 to include the '\n'
    if (length == 0) {
        return 0;
    }
    length = std::min(length, static_cast<int>(maxSize));
    // copy lines to output
    std::copy(_buffer.data(), _buffer.data() + length, data);
    // trim buffer to remove consumed line
    _buffer = _buffer.right(_buffer.size() - length);
    // return number of bytes read
    return length;
}

void UdpIODevice::_readAvailableData() {
    while (hasPendingDatagrams()) {
        int previousSize = _buffer.size();
        _buffer.resize(static_cast<int>(_buffer.size() + pendingDatagramSize()));
        readDatagram((_buffer.data() + previousSize), pendingDatagramSize());
    }
}
