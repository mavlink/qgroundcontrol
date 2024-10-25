/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UdpIODevice.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UdpIODeviceLog, "qgc.comms.udpiodevice")

UdpIODevice::UdpIODevice(QObject *parent)
    : QUdpSocket(parent)
{
    // qCDebug(UdpIODeviceLog) << Q_FUNC_INFO << this;

    (void) connect(this, &QUdpSocket::readyRead, this, &UdpIODevice::_readAvailableData);
}

UdpIODevice::~UdpIODevice()
{
    // qCDebug(UdpIODeviceLog) << Q_FUNC_INFO << this;
}

bool UdpIODevice::canReadLine() const
{
    return _buffer.contains('\n');
}

qint64 UdpIODevice::readLineData(char *data, qint64 maxSize)
{
    const qint64 newlinePos = _buffer.indexOf('\n');
    if (newlinePos < 0) {
        return 0;
    }

    const qint64 length = std::min(newlinePos + 1, maxSize);
    (void) std::copy_n(_buffer.constData(), length, data);

    (void) _buffer.remove(0, length);
    return length;
}

qint64 UdpIODevice::readData(char *data, qint64 maxSize)
{
    const qint64 length = std::min<qint64>(_buffer.size(), maxSize);
    (void) std::copy_n(_buffer.constData(), length, data);

    (void) _buffer.remove(0, length);
    return length;
}

void UdpIODevice::_readAvailableData()
{
    while (hasPendingDatagrams()) {
        const qint64 size = pendingDatagramSize();
        const int oldSize = _buffer.size();
        _buffer.resize(oldSize + static_cast<int>(size));
        (void) readDatagram(_buffer.data() + oldSize, size);
    }
}
