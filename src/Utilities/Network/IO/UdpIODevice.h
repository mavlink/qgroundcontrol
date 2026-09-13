#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtNetwork/QUdpSocket>

/// \brief UdpIODevice provides a QIODevice interface over a QUdpSocket in server mode.
///
/// It exposes received datagrams as one sequential, read-only stream.
/// Overflow discards whole lines and invalidates read transactions. Binary protocols should use QUdpSocket directly.
///
class UdpIODevice : public QIODevice
{
    Q_OBJECT

public:
    explicit UdpIODevice(QObject* parent = nullptr);
    ~UdpIODevice() override;

    bool bind(const QHostAddress& address, quint16 port);

    quint16 localPort() const { return _socket.localPort(); }

    bool open(OpenMode mode) override;

    /// Select one sender for the lifetime of this stream; close/rebind permits a new sender.
    void setSelectFirstPeer(bool enabled) { _selectFirstPeer = enabled; }

    QString selectedPeer() const { return _selectedPeer; }

    qint64 bytesAvailable() const override;
    bool canReadLine() const override;

    bool isSequential() const override { return true; }

    void close() override;

protected:
    qint64 readLineData(char* data, qint64 maxSize) override;
    qint64 readData(char* data, qint64 maxSize) override;

    qint64 writeData(const char*, qint64) override { return -1; }

private slots:
    void _readAvailableData();

private:
    // Keep the newest complete lines; discard an overflowing partial line through its newline.
    static constexpr qsizetype kMaxBufferedBytes = 64 * 1024;
    QUdpSocket _socket;
    QByteArray _buffer;
    QString _selectedPeer;
    bool _selectFirstPeer = false;
    bool _drainScheduled = false;
    bool _emittingReadyRead = false;
    quint64 _generation = 0;
    bool _discardUntilNewline = false;
};
