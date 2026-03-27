#pragma once

#include "GPSTransport.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>

#include <memory>

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

    /// Wake from retry sleep during open() so shutdown is not blocked
    void cancelRetry();

private:
    static constexpr uint32_t kOpenRetryCount = 60;
    static constexpr uint32_t kOpenRetryIntervalMs = 500;

    QString _device;
    std::unique_ptr<QSerialPort> _serial;

    QMutex _retryMutex;
    QWaitCondition _retryCondition;
    bool _retryCancelled = false;
};
