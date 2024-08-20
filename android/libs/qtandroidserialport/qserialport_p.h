// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSERIALPORT_P_H
#define QSERIALPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qiodevice_p.h>
#include <QtCore/private/qproperty_p.h>
#include <QtCore/QLoggingCategory>

#include "qserialport.h"
#include <AndroidSerial.h>

#define BAD_PORT 0
#define MAX_READ_SIZE (16 * 1024);

#ifndef QSERIALPORT_BUFFERSIZE
#define QSERIALPORT_BUFFERSIZE 32768
#endif

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)

QT_BEGIN_NAMESPACE

class QSerialPortErrorInfo
{
public:
    QSerialPortErrorInfo(QSerialPort::SerialPortError newErrorCode = QSerialPort::UnknownError,
                                  const QString &newErrorString = QString());
    QSerialPort::SerialPortError errorCode = QSerialPort::UnknownError;
    QString errorString;
};

class QSerialPortPrivate : public QIODevicePrivate
{
public:
    Q_DECLARE_PUBLIC(QSerialPort)

    QSerialPortPrivate();

    bool open(QIODevice::OpenMode mode);
    void close();

    bool flush();
    bool clear(QSerialPort::Directions directions);

    QSerialPort::PinoutSignals pinoutSignals();

    bool setDataTerminalReady(bool set);
    bool setRequestToSend(bool set);

    bool setBaudRate();
    bool setBaudRate(qint32 baudRate, QSerialPort::Directions directions);
    bool setDataBits(QSerialPort::DataBits dataBits);
    bool setParity(QSerialPort::Parity parity);
    bool setStopBits(QSerialPort::StopBits stopBits);
    bool setFlowControl(QSerialPort::FlowControl flowControl);

    bool setBreakEnabled(bool set);

    void setError(const QSerialPortErrorInfo &errorInfo);

    void setBindableError(QSerialPort::SerialPortError error)
    { setError(error); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::SerialPortError, error,
        &QSerialPortPrivate::setBindableError, QSerialPort::NoError)

    bool setBindableDataBits(QSerialPort::DataBits dataBits)
    { return q_func()->setDataBits(dataBits); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::DataBits, dataBits,
        &QSerialPortPrivate::setBindableDataBits, QSerialPort::Data8)

    bool setBindableParity(QSerialPort::Parity parity)
    { return q_func()->setParity(parity); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::Parity, parity,
        &QSerialPortPrivate::setBindableParity, QSerialPort::NoParity)

    bool setBindableStopBits(QSerialPort::StopBits stopBits)
    { return q_func()->setStopBits(stopBits); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::StopBits, stopBits,
        &QSerialPortPrivate::setBindableStopBits, QSerialPort::OneStop)

    bool setBindableFlowControl(QSerialPort::FlowControl flowControl)
    { return q_func()->setFlowControl(flowControl); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::FlowControl, flowControl,
        &QSerialPortPrivate::setBindableFlowControl, QSerialPort::NoFlowControl)

    bool setBindableBreakEnabled(bool isBreakEnabled)
    { return q_func()->setBreakEnabled(isBreakEnabled); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, bool, isBreakEnabled,
        &QSerialPortPrivate::setBindableBreakEnabled, false)

    bool waitForReadyRead(int msec);
    bool waitForBytesWritten(int msec);

    bool startAsyncRead();

    qint64 writeData(const char *data, qint64 maxSize);

    void newDataArrived(char *bytes, int length);
    void exceptionArrived(const QString &ex);

    static QList<qint32> standardBaudRates();

    QString systemLocation;
    qint32 inputBaudRate = QSerialPort::Baud9600;
    qint32 outputBaudRate = QSerialPort::Baud9600;
    qint64 readBufferMaxSize = 0;
    int descriptor = -1;

private:
    qint64 _writeToPort(const char *data, qint64 maxSize, int timeout = 0, bool async = false);
    bool _stopAsyncRead();
    bool _setParameters(int baudRate, int dataBits, int stopBits, int parity);
    bool _writeDataOneShot(int msecs);

    static qint32 _settingFromBaudRate(qint32 baudRate);

    qint64 m_pendingBytesWritten = 0;

    int m_deviceId = BAD_PORT;
    int m_dataBits = AndroidSerial::Data8;
    int m_stopBits = AndroidSerial::OneStop;
    int m_parity = AndroidSerial::NoParity;
};

QT_END_NAMESPACE

#endif // QSERIALPORT_P_H
