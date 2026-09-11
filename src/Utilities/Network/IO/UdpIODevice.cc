#include "UdpIODevice.h"

#include <QtNetwork/QNetworkDatagram>

#include <algorithm>

#include "QGCLoggingCategory.h"
#include "UdpPeer.h"

QGC_LOGGING_CATEGORY(UdpIODeviceLog, "Utilities.UdpIODevice")

UdpIODevice::UdpIODevice(QObject* parent)
    : QUdpSocket(parent)
{
    qCDebug(UdpIODeviceLog) << this;

    (void) connect(this, &QUdpSocket::readyRead, this, &UdpIODevice::_readAvailableData);
}

UdpIODevice::~UdpIODevice()
{
    qCDebug(UdpIODeviceLog) << this;
}

qint64 UdpIODevice::bytesAvailable() const
{
    // Native datagrams are not stream bytes until _readAvailableData() has copied them.
    return QIODevice::bytesAvailable() + _buffer.size();
}

bool UdpIODevice::canReadLine() const
{
    return QIODevice::canReadLine() || _buffer.contains('\n');
}

qint64 UdpIODevice::readLineData(char* data, qint64 maxSize)
{
    const qint64 newlinePos = _buffer.indexOf('\n');
    return readData(data, newlinePos < 0 ? maxSize : std::min(newlinePos + 1, maxSize));
}

qint64 UdpIODevice::readData(char* data, qint64 maxSize)
{
    const qint64 length = std::min<qint64>(_buffer.size(), maxSize);
    (void) std::copy_n(_buffer.constData(), length, data);

    (void) _buffer.remove(0, length);
    return length;
}

void UdpIODevice::close()
{
    ++_generation;
    _drainScheduled = false;
    _selectedPeer.clear();
    _buffer.clear();
    _discardUntilNewline = false;
    QUdpSocket::close();
}

void UdpIODevice::_readAvailableData()
{
    UdpDrainBudget budget;
    while (hasPendingDatagrams() && budget.available()) {
        const QNetworkDatagram datagram = receiveDatagram();
        if (!datagram.isValid()) {
            break;
        }
        const QByteArray data = datagram.data();
        budget.consume(data.size());
        if (data.isEmpty()) {
            continue;
        }
        if (_selectFirstPeer) {
            const QString peer = udpPeerKey(datagram.senderAddress(), datagram.senderPort());
            if (_selectedPeer.isEmpty()) {
                _selectedPeer = peer;
                qCDebug(UdpIODeviceLog) << "Selected UDP sender" << peer;
            } else if (_selectedPeer != peer) {
                continue;
            }
        }
        qsizetype start = 0;
        if (_discardUntilNewline) {
            const qsizetype newline = data.indexOf('\n');
            if (newline < 0) {
                continue;
            }
            start = newline + 1;
            _discardUntilNewline = false;
        }
        _buffer.append(data.constData() + start, data.size() - start);
        if (_buffer.size() > kMaxBufferedBytes) {
            const qsizetype newline = _buffer.indexOf('\n', _buffer.size() - kMaxBufferedBytes - 1);
            if (newline < 0) {
                _buffer.clear();
                _discardUntilNewline = true;
            } else {
                _buffer.remove(0, newline + 1);
            }
        }
    }
    if (hasPendingDatagrams() && !_drainScheduled) {
        _drainScheduled = true;
        const auto generation = _generation;
        QMetaObject::invokeMethod(
            this,
            [this, generation]() {
                if (generation == _generation) {
                    _drainScheduled = false;
                    emit readyRead();
                }
            },
            Qt::QueuedConnection);
    }
}
