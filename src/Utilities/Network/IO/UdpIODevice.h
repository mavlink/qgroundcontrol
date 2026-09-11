#pragma once

#include <QtCore/QByteArray>
#include <QtNetwork/QUdpSocket>

/// \brief UdpIODevice provides a QIODevice interface over a QUdpSocket in server mode.
///
/// It allows line-based reading using canReadLine() and readLineData() even when the socket is in bound mode.
///
class UdpIODevice : public QUdpSocket
{
    Q_OBJECT

public:
    explicit UdpIODevice(QObject* parent = nullptr);
    ~UdpIODevice() override;

    /// Select one sender for the lifetime of this stream; close/rebind permits a new sender.
    void setSelectFirstPeer(bool enabled) { _selectFirstPeer = enabled; }

    QString selectedPeer() const { return _selectedPeer; }

    qint64 bytesAvailable() const override;
    bool canReadLine() const override;
    qint64 readLineData(char* data, qint64 maxSize) override;
    qint64 readData(char* data, qint64 maxSize) override;

    bool isSequential() const override { return true; }

    void close() override;

private slots:
    void _readAvailableData();

private:
    // Keep the newest complete lines; discard an overflowing partial line through its newline.
    static constexpr qsizetype kMaxBufferedBytes = 64 * 1024;
    QByteArray _buffer;
    QString _selectedPeer;
    bool _selectFirstPeer = false;
    bool _drainScheduled = false;
    quint64 _generation = 0;
    bool _discardUntilNewline = false;
};
