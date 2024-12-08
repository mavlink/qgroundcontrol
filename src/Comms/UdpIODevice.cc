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
    if (_buffer.isEmpty()) {
        return -1; // No data available
    }

    const qsizetype newlineIndex = _buffer.indexOf('\n');
    qsizetype length;
    if (newlineIndex != -1) {
        length = newlineIndex + 1; // Include the newline character
    } else {
        // No newline character found
        length = qMin(_buffer.size(), static_cast<qsizetype>(maxSize));
    }

    const qsizetype bytesToCopy = qMin(length, static_cast<qsizetype>(maxSize));
    (void) memcpy(data, _buffer.constData(), bytesToCopy);
    _buffer.remove(0, bytesToCopy);

    return bytesToCopy;
}

qint64 UdpIODevice::readData(char* data, qint64 maxSize)
{
    if (_buffer.isEmpty()) {
        return -1; // No data available
    }

    const qsizetype bytesToCopy = qMin(static_cast<qsizetype>(maxSize), _buffer.size());
    (void) memcpy(data, _buffer.constData(), bytesToCopy);
    _buffer.remove(0, bytesToCopy);

    return bytesToCopy;
}

qint64 UdpIODevice::bytesAvailable() const
{
    return (_buffer.size() + QIODevice::bytesAvailable());
}

void UdpIODevice::_readAvailableData()
{
    while (hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        const qint64 bytesRead = readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        if (bytesRead > 0) {
            (void) _buffer.append(datagram.left(bytesRead));
        } else {
            qCWarning(UdpIODeviceLog) << "Error reading datagram:" << errorString();
        }
    }

    if (!_buffer.isEmpty()) {
        emit readyRead();
    }
}
