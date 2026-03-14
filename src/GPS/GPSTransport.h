#pragma once

#include <QtCore/QObject>

class GPSTransport : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~GPSTransport() override = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual qint64 read(char *data, qint64 maxSize) = 0;
    virtual qint64 write(const char *data, qint64 size) = 0;
    virtual bool waitForReadyRead(int msecs) = 0;
    virtual bool waitForBytesWritten(int msecs) = 0;
    virtual qint64 bytesAvailable() const = 0;
    virtual bool setBaudRate(qint32 baudRate) = 0;
};
