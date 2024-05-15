#ifndef QSERIALPORT_ANDROID_P_H
#define QSERIALPORT_ANDROID_P_H

#include "qserialport_p.h"
#include <AndroidSerial.h>

#include <QtCore/QLoggingCategory>

#define BAD_PORT 0
#define MAX_READ_SIZE (16 * 1024);

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)

QT_BEGIN_NAMESPACE

class QSerialPortPrivate : public QSerialPortPrivateData
{
    Q_DECLARE_PUBLIC(QSerialPort)

public:
    QSerialPortPrivate(QSerialPort *q);

    bool open(QIODevice::OpenMode mode);
    void close();

    QSerialPort::PinoutSignals pinoutSignals();

    bool setDataTerminalReady(bool set);
    bool setRequestToSend(bool set);

    bool flush();
    bool clear(QSerialPort::Directions directions);

    bool sendBreak(int duration);
    bool setBreakEnabled(bool set);

    bool waitForReadyRead(int msecs);
    bool waitForBytesWritten(int msecs);

    bool setBaudRate();
    bool setBaudRate(qint32 baudRate, QSerialPort::Directions directions);
    bool setDataBits(QSerialPort::DataBits dataBits);
    bool setParity(QSerialPort::Parity parity);
    bool setStopBits(QSerialPort::StopBits stopBits);
    bool setFlowControl(QSerialPort::FlowControl flowControl);

    void newDataArrived(char *bytes, int length);
    void exceptionArrived(QString str);

    void stopReadThread();
    void startReadThread();

    qint64 bytesToWrite() const;
    qint64 writeData(const char *data, qint64 maxSize);

    static QList<qint32> standardBaudRates();

    int descriptor = -1;

private:
    qint64 pendingBytesWritten = 0;
    int deviceId = BAD_PORT;
    int m_dataBits = AndroidSerial::Data8;
    int m_stopBits = AndroidSerial::OneStop;
    int m_parity = AndroidSerial::NoParity;
    int m_flowControl = AndroidSerial::NoFlowControl;
    int m_breakEnabled = true;
    qint64 internalWriteTimeoutMsec = 0;
    bool isReadStopped = true;

    bool setParameters(int baudRate, int dataBits, int stopBits, int parity);
    qint64 writeToPort(const char *data, qint64 maxSize);
    bool writeDataOneShot();
    bool isDeviceValid() const { return (deviceId != BAD_PORT); }
};

QT_END_NAMESPACE

#endif // QSERIALPORT_ANDROID_P_H
