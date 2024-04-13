#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtSerialPort/QSerialPortInfo>

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialLog);

class AndroidSerial
{
public:
    static void onNewData(int deviceId, QByteArray data);
    static QList<QSerialPortInfo> availableDevices();
    static int open(QString portName);
    static void close(QString portName);
    static bool isOpen(QString portName);
    static QByteArray read(int deviceId, int length, int timeout);
    static void write(int deviceId, QByteArrayView data, int length, int timeout, bool async);
    static void setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity);
    static bool getCarrierDetect(int deviceId);
    static bool getClearToSend(int deviceId);
    static bool getDataSetReady(int deviceId);
    static bool getDataTerminalReady(int deviceId);
    static void setDataTerminalReady(int deviceId, bool set);
    static bool getRingIndicator(int deviceId);
    static bool getRequestToSend(int deviceId);
    static void setRequestToSend(int deviceId, bool set);
    static void flush(int deviceId, bool input, bool output);
    static void setBreak(int deviceId, bool set);
};
