#include "UdpIODevice.h"

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QNetworkDatagram>

#include <algorithm>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UdpIODeviceLog, "Utilities.UdpIODevice")

namespace {
QString udpPeerKey(const QHostAddress& address, quint16 port)
{
    return address.toString() + QLatin1Char(':') + QString::number(port);
}

struct UdpDrainBudget
{
    static constexpr qsizetype MAX_DATAGRAMS = 16;
    static constexpr qsizetype MAX_BYTES = 64 * 1024;
    qsizetype datagrams = 0;
    qsizetype bytes = 0;

    bool available() const { return datagrams < MAX_DATAGRAMS && bytes < MAX_BYTES; }

    void consume(qsizetype size)
    {
        ++datagrams;
        bytes += size;
    }
};
}  // namespace

UdpIODevice::UdpIODevice(QObject* parent) : QIODevice(parent), _socket(this)
{
    qCDebug(UdpIODeviceLog) << this;

    connect(&_socket, &QUdpSocket::readyRead, this, &UdpIODevice::_readAvailableData);
    connect(&_socket, &QUdpSocket::errorOccurred, this, [this]() { setErrorString(_socket.errorString()); });
}

UdpIODevice::~UdpIODevice()
{
    qCDebug(UdpIODeviceLog) << this;
}

bool UdpIODevice::bind(const QHostAddress& address, quint16 port)
{
    const QPointer<UdpIODevice> guard(this);
    close();
    if (!guard) {
        return false;
    }
    if (!_socket.bind(address, port)) {
        setErrorString(_socket.errorString());
        return false;
    }
    return open(ReadOnly);
}

bool UdpIODevice::open(OpenMode mode)
{
    if (_socket.state() != QAbstractSocket::BoundState || !(mode & ReadOnly) || (mode & WriteOnly)) {
        setErrorString(tr("UDP stream requires a bound socket and read-only mode"));
        return false;
    }
    return QIODevice::open(mode);
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
    qint64 length = std::min<qint64>(_buffer.size(), maxSize);
    if (length > 0 && !_receipts.empty()) {
        _lastReadTimestampUs = _receipts.front().timestampUs;
        if (openMode().testFlag(Unbuffered)) {
            length = std::min<qint64>(length, _receipts.front().size);
        }
    }
    (void) std::copy_n(_buffer.constData(), length, data);

    (void) _buffer.remove(0, length);
    _consumeReceipts(length);
    return length;
}

void UdpIODevice::close()
{
    ++_generation;
    _drainScheduled = false;
    _selectedPeer.clear();
    _lastPeerDataUs = 0;
    _buffer.clear();
    _receipts.clear();
    _lastReadTimestampUs = 0;
    _discardUntilNewline = false;
    _socket.close();
    QIODevice::close();
}

void UdpIODevice::_readAvailableData()
{
    if (!isOpen() || _readingDatagrams) {
        return;
    }
    _readingDatagrams = true;
    const QPointer<UdpIODevice> readingGuard(this);
    const auto finishReading = qScopeGuard([readingGuard]() {
        if (readingGuard) {
            readingGuard->_readingDatagrams = false;
            readingGuard->_scheduleRead();
        }
    });
    UdpDrainBudget budget;
    bool receivedData = false;
    while (_socket.hasPendingDatagrams() && budget.available()) {
        const QNetworkDatagram datagram = _socket.receiveDatagram();
        if (!datagram.isValid()) {
            break;
        }
        const QByteArray data = datagram.data();
        budget.consume(data.size());
        if (data.isEmpty()) {
            continue;
        }
        const auto receivedAtUs = ReadTimestamp::nowUs();
        if (_selectFirstPeer) {
            const QString peer = udpPeerKey(datagram.senderAddress(), datagram.senderPort());
            if (_selectedPeer.isEmpty()) {
                _selectedPeer = peer;
                qCDebug(UdpIODeviceLog) << "Selected UDP sender" << peer;
            } else if (_selectedPeer != peer) {
                const auto idleMs = MonotonicClock::ageMilliseconds(_lastPeerDataUs, receivedAtUs);
                if (_peerIdleTimeout <= std::chrono::milliseconds::zero() || idleMs < _peerIdleTimeout.count()) {
                    continue;
                }
                const QString previousPeer = _selectedPeer;
                _selectedPeer = peer;
                _buffer.clear();
                _receipts.clear();
                _lastReadTimestampUs = 0;
                _discardUntilNewline = false;
                if (isTransactionStarted()) {
                    commitTransaction();
                }
                // Reset Qt's read/peek buffers without closing the listening socket.
                QIODevice::open(openMode());
                _drainScheduled = false;
                const auto generation = ++_generation;
                _lastPeerDataUs = receivedAtUs;
                qCDebug(UdpIODeviceLog) << "Replacing idle UDP sender" << previousPeer << "with" << peer;
                const QPointer<UdpIODevice> guard(this);
                emit peerReplaced(previousPeer, peer);
                if (!guard || generation != _generation || !isOpen()) {
                    return;
                }
            }
            _lastPeerDataUs = receivedAtUs;
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
        // QIODevice counts retained bytes before Text mode removes carriage returns.
        const bool textMode = isTextModeEnabled();
        setTextModeEnabled(false);
        const auto restoreTextMode = qScopeGuard([this, textMode]() { setTextModeEnabled(textMode); });
        qint64 transactionBytes = -1;
        if (isTransactionStarted()) {
            // Consumed transaction bytes are retained by Qt but excluded from
            // bytesAvailable(). Restore them while checking the memory bound.
            const auto unreadBytes = QIODevice::bytesAvailable();
            rollbackTransaction();
            transactionBytes = QIODevice::bytesAvailable() - unreadBytes;
        }
        _buffer.append(data.constData() + start, data.size() - start);
        if (data.size() > start) {
            _receipts.push_back({data.size() - start, receivedAtUs});
        }
        receivedData |= data.size() > start;
        if (bytesAvailable() > kMaxBufferedBytes) {
            // peek() and short reads retain a prefix in QIODevice. Include it in
            // the same overflow decision so dropped input cannot join two lines.
            const auto bufferedBytes = QIODevice::bytesAvailable();
            if (bufferedBytes > 0) {
                const auto prefix = QIODevice::read(bufferedBytes);
                _buffer.prepend(prefix);
                if (!prefix.isEmpty()) {
                    _receipts.push_front({prefix.size(), _lastReadTimestampUs});
                }
            }
            const qsizetype newline = _buffer.indexOf('\n', _buffer.size() - kMaxBufferedBytes - 1);
            if (newline < 0) {
                _buffer.clear();
                _receipts.clear();
                _discardUntilNewline = true;
            } else {
                _buffer.remove(0, newline + 1);
                _consumeReceipts(newline + 1);
            }
        } else if (transactionBytes >= 0) {
            startTransaction();
            (void) skip(transactionBytes);
        }
    }
    if (receivedData && bytesAvailable() > 0 && !_emittingReadyRead) {
        _emittingReadyRead = true;
        const QPointer<UdpIODevice> guard(this);
        emit readyRead();
        if (guard) {
            _emittingReadyRead = false;
        }
    }
}

void UdpIODevice::_consumeReceipts(qsizetype size)
{
    while (size > 0 && !_receipts.empty()) {
        auto& receipt = _receipts.front();
        const auto consumed = std::min(size, receipt.size);
        receipt.size -= consumed;
        size -= consumed;
        if (receipt.size == 0) {
            _receipts.pop_front();
        }
    }
}

void UdpIODevice::_scheduleRead()
{
    if (isOpen() && _socket.hasPendingDatagrams() && !_drainScheduled) {
        _drainScheduled = true;
        const auto generation = _generation;
        QMetaObject::invokeMethod(
            this,
            [this, generation]() {
                if (generation == _generation) {
                    _drainScheduled = false;
                    _readAvailableData();
                }
            },
            Qt::QueuedConnection);
    }
}
