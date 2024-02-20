#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>

class AndroidSerial
{
public:
    static void onDisconnect(int serialPort);
    static void onNewData(int serialPort, QByteArray data);
    static void onException(int serialPort, QString msg);
};
