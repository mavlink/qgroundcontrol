/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtNetwork/QUdpSocket>

Q_DECLARE_LOGGING_CATEGORY(UdpIODeviceLog)

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
