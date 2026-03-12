#pragma once

#include "GPSTransport.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(SerialGPSTransportLog)

class QSerialPort;

class SerialGPSTransport : public GPSTransport
{
    Q_OBJECT

public:
    explicit SerialGPSTransport(const QString &device, QObject *parent = nullptr);
    ~SerialGPSTransport() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;

    qint64 read(char *data, qint64 maxSize) override;
    qint64 write(const char *data, qint64 size) override;
    bool waitForReadyRead(int msecs) override;
    bool waitForBytesWritten(int msecs) override;
    qint64 bytesAvailable() const override;
    bool setBaudRate(qint32 baudRate) override;

private:
    QString _device;
    QSerialPort *_serial = nullptr;
};
