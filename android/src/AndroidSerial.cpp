#include "AndroidSerial.h"
#include "qserialport.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>

#define BAD_PORT 0

static Q_LOGGING_CATEGORY(AndroidSerialLog, "qgc.android.serial");

void AndroidSerial::onDisconnect(int serialPort)
{
    if(serialPort != BAD_PORT)
    {
        auto serialPort = reinterpret_cast<QSerialPortPrivate*>(serialPort);
        qCDebug(AndroidSerialPortLog) << "Device disconnected" << serialPort->systemLocation.toLatin1().data();
        serialPort->q_ptr->close();
    }
}

void AndroidSerial::onNewData(int serialPort, QByteArray data)
{
    if(serialPort != BAD_PORT)
    {
        auto serialPort = reinterpret_cast<QSerialPortPrivate*>(serialPort);
        serialPort->newDataArrived(data.constData(), data.size());
    }
}

void AndroidSerial::onException(int serialPort, QString msg)
{
    if(serialPort != BAD_PORT)
    {
        auto serialPort = reinterpret_cast<QSerialPortPrivate*>(serialPort);
        serialPort->exceptionArrived(msg.constData());
    }
}
