// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

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

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/private/qiodevice_p.h>
#include <QtCore/private/qproperty_p.h>

#include <atomic>

#include "AndroidSerial.h"
#include "qserialport.h"

constexpr int INVALID_DEVICE_ID = 0;
constexpr int MIN_READ_TIMEOUT = 500;
constexpr qint64 MAX_READ_SIZE = 16 * 1024;
constexpr qint64 DEFAULT_READ_BUFFER_SIZE = MAX_READ_SIZE;
constexpr qint64 DEFAULT_WRITE_BUFFER_SIZE = 32 * 1024;
constexpr int DEFAULT_WRITE_TIMEOUT = 5000;
constexpr int DEFAULT_READ_TIMEOUT = 0;
#ifndef QSERIALPORT_BUFFERSIZE
#define QSERIALPORT_BUFFERSIZE DEFAULT_WRITE_BUFFER_SIZE
#endif

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)

QT_BEGIN_NAMESPACE

class QTimer;

class QSerialPortErrorInfo
{
public:
    QSerialPortErrorInfo(QSerialPort::SerialPortError newErrorCode = QSerialPort::UnknownError,
                         const QString& newErrorString = QString());
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

    bool sendBreak(int duration);
    bool setBreakEnabled(bool set);

    bool settingsRestoredOnClose = false; // No-op on Android — no persistent terminal settings

    void setError(const QSerialPortErrorInfo& errorInfo);

    void setBindableError(QSerialPort::SerialPortError error_)
    {
        setError(error_);
    }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::SerialPortError, error,
                                       &QSerialPortPrivate::setBindableError, QSerialPort::NoError)

    bool setBindableDataBits(QSerialPort::DataBits dataBits_)
    {
        return q_func()->setDataBits(dataBits_);
    }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::DataBits, dataBits,
                                       &QSerialPortPrivate::setBindableDataBits, QSerialPort::Data8)

    bool setBindableParity(QSerialPort::Parity parity_)
    {
        return q_func()->setParity(parity_);
    }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::Parity, parity,
                                       &QSerialPortPrivate::setBindableParity, QSerialPort::NoParity)

    bool setBindableStopBits(QSerialPort::StopBits stopBits_)
    {
        return q_func()->setStopBits(stopBits_);
    }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::StopBits, stopBits,
                                       &QSerialPortPrivate::setBindableStopBits, QSerialPort::OneStop)

    bool setBindableFlowControl(QSerialPort::FlowControl flowControl_)
    {
        return q_func()->setFlowControl(flowControl_);
    }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, QSerialPort::FlowControl, flowControl,
                                       &QSerialPortPrivate::setBindableFlowControl, QSerialPort::NoFlowControl)

    bool setBindableBreakEnabled(bool isBreakEnabled_)
    {
        return q_func()->setBreakEnabled(isBreakEnabled_);
    }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QSerialPortPrivate, bool, isBreakEnabled,
                                       &QSerialPortPrivate::setBindableBreakEnabled, false)

    bool waitForReadyRead(int msec);
    bool waitForBytesWritten(int msec);

    bool startAsyncRead();

    qint64 writeData(const char* data, qint64 maxSize);

    void newDataArrived(const char* bytes, int length);
    void exceptionArrived(const QString& ex);

    static QList<qint32> standardBaudRates();

    QString systemLocation;
    qint32 inputBaudRate = QSerialPort::Baud9600;
    qint32 outputBaudRate = QSerialPort::Baud9600;
    qint64 readBufferMaxSize = 0;
    qint64 writeBufferMaxSize = 0;
    int descriptor = -1;

private:
    qint64 _writeToPort(const char* data, qint64 maxSize, int timeout = DEFAULT_WRITE_TIMEOUT, bool async = false);
    bool _stopAsyncRead();
    void _scheduleReadyRead();
    void _drainWriteBuffer();
    bool _setParameters(qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                        QSerialPort::Parity parity);
    static int _stopBitsToAndroidStopBits(QSerialPort::StopBits stopBits);
    static int _dataBitsToAndroidDataBits(QSerialPort::DataBits dataBits);
    static int _parityToAndroidParity(QSerialPort::Parity parity);
    static int _flowControlToAndroidFlowControl(QSerialPort::FlowControl flowControl);

    int _deviceId = INVALID_DEVICE_ID;

    // Read: Java thread appends to _pendingData under _readMutex; owner thread
    // drains it into QIODevicePrivate::buffer in the _scheduleReadyRead lambda.
    std::atomic<bool> _readyReadPending{false};
    QMutex _readMutex;
    QWaitCondition _readWaitCondition;
    QByteArray _pendingData;

    // Write: writeData() appends to writeBuffer; _drainWriteBuffer() fires via
    // zero-interval QTimer on the owner thread's event loop (matches Qt Win pattern).
    QTimer *_writeTimer = nullptr;
};

QT_END_NAMESPACE
