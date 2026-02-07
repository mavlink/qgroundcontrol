#pragma once

#include <QtCore/QByteArray>
#include <QtNetwork/QUdpSocket>


/// UdpIODevice provides a QIODevice interface over a QUdpSocket in server mode.
/// It allows line-based reading using canReadLine() and readLineData() even when the socket is in bound mode.
class UdpIODevice: public QUdpSocket
{
    Q_OBJECT

public:
    explicit UdpIODevice(QObject *parent = nullptr);
    ~UdpIODevice();

    bool canReadLine() const override;
    qint64 readLineData(char* data, qint64 maxSize) override;
    qint64 readData(char* data, qint64 maxSize) override;
    bool isSequential() const override { return true; }

private slots:
    void _readAvailableData();

private:
    QByteArray _buffer;
};
