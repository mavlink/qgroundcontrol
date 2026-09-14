#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtNetwork/QUdpSocket>

#include <chrono>
#include <deque>

#include "ReadTimestamp.h"

/// \brief UdpIODevice provides a QIODevice interface over a QUdpSocket in server mode.
///
/// It exposes received datagrams as one sequential, read-only stream.
/// Overflow discards whole lines and invalidates read transactions. Binary protocols should use QUdpSocket directly.
///
class UdpIODevice : public QIODevice, public ReadTimestamp
{
    Q_OBJECT

public:
    explicit UdpIODevice(QObject* parent = nullptr);
    ~UdpIODevice() override;

    bool bind(const QHostAddress& address, quint16 port);

    quint16 localPort() const { return _socket.localPort(); }

    bool open(OpenMode mode) override;

    /// Original receipt of the last underlying read. Use Unbuffered for timestamp-aware consumers.
    quint64 lastReadTimestampUs() const override { return _lastReadTimestampUs; }

    /// Select one sender; replacements require close/rebind unless an idle timeout is enabled.
    void setSelectFirstPeer(bool enabled) { _selectFirstPeer = enabled; }

    /// Positive timeouts permit replacement after the selected sender stops sending data.
    void setPeerIdleTimeout(std::chrono::milliseconds timeout) { _peerIdleTimeout = timeout; }

    QString selectedPeer() const { return _selectedPeer; }

    qint64 bytesAvailable() const override;
    bool canReadLine() const override;

    bool isSequential() const override { return true; }

    void close() override;

signals:
    /// Emitted after discarding old stream bytes, before exposing the replacement sender's data.
    void peerReplaced(const QString& previousPeer, const QString& peer);

protected:
    qint64 readLineData(char* data, qint64 maxSize) override;
    qint64 readData(char* data, qint64 maxSize) override;

    qint64 writeData(const char*, qint64) override { return -1; }

private slots:
    void _readAvailableData();

private:
    // Keep the newest complete lines; discard an overflowing partial line through its newline.
    static constexpr qsizetype kMaxBufferedBytes = 64 * 1024;
    void _consumeReceipts(qsizetype size);
    void _scheduleRead();

    struct Receipt
    {
        qsizetype size;
        quint64 timestampUs;
    };

    std::deque<Receipt> _receipts;
    quint64 _lastReadTimestampUs = 0;
    QUdpSocket _socket;
    QByteArray _buffer;
    QString _selectedPeer;
    std::chrono::milliseconds _peerIdleTimeout{0};
    quint64 _lastPeerDataUs = 0;
    bool _selectFirstPeer = false;
    bool _drainScheduled = false;
    bool _readingDatagrams = false;
    bool _emittingReadyRead = false;
    quint64 _generation = 0;
    bool _discardUntilNewline = false;
};
