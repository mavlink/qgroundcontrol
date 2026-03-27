#include "UdpIODevice.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UdpIODeviceLog, "Comms.UdpIODevice")

UdpIODevice::UdpIODevice(QObject *parent)
    : QUdpSocket(parent)
{
    (void) connect(this, &QUdpSocket::readyRead, this, &UdpIODevice::_readAvailableData);
}

UdpIODevice::~UdpIODevice()
{
}

bool UdpIODevice::canReadLine() const
{
    return _buffer.canReadLine();
}

qint64 UdpIODevice::readLineData(char *data, qint64 maxSize)
{
    return _buffer.readLine(data, maxSize);
}

qint64 UdpIODevice::readData(char *data, qint64 maxSize)
{
    return _buffer.read(data, maxSize);
}

void UdpIODevice::_readAvailableData()
{
    while (hasPendingDatagrams()) {
        const qint64 size = pendingDatagramSize();
        QByteArray datagram(static_cast<int>(size), Qt::Uninitialized);
        (void) readDatagram(datagram.data(), size);
        (void) _buffer.append(datagram);
    }
}
