#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtSerialPort/QSerialPortInfo>

class AndroidSerial
{
public:
    static void onNewData(int deviceId, QByteArray data);
    static QList<QSerialPortInfo> availableDevices();
    static int open(int deviceId);
    static void close(int deviceId);
    static bool isOpen(int deviceId);
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
    static void setBreak(int deviceId, bool on);
};
