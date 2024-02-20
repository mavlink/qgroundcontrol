#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>

class AndroidSerial
{
public:
    static void onDisconnect(int deviceId);
    static void onNewData(int deviceId, QByteArray data);
    static void onException(int deviceId, QString msg);
    static bool open(int deviceId);
    static bool close(int deviceId);
    static bool setParameters(int deviceId, int baudRateA, int dataBitsA, int stopBitsA, int parityA);
    static void stopReadThread(int deviceId);
    static void startReadThread(int deviceId);
    static bool setDataTerminalReady(int deviceId, bool set);
    static bool setRequestToSend(int deviceId, bool set);
    static bool clear(int deviceId, bool input, bool output);
    static size_t write(const char *data, size_t length);
    static size_t read(char *data, size_t length);
    static bool isBusy(int deviceId);
    static bool isValid(int deviceId);
    static QList<QSerialPortInfo> availableDevices();

    static void startPort(int port);
    static void stopPort(int port);
    static bool isPortRunning(int port);
    static int getPortReadBufferSize(int port);
    static void setPortReadBufferSize(int port, int size);
    static int getPortWriteBufferSize(int port);
    static void setPortWriteBufferSize(int port, int size);
    static int getPortReadTimeout(int port);
    static void setPortReadTimeout(int port, int timeout);
    static int getPortWriteTimeout(int port);
    static void setPortWriteTimeout(int port, int timeout);
};
