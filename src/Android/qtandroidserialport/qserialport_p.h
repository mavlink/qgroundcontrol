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

#include <atomic>
#include <memory>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/private/qiodevice_p.h>
#include <QtCore/private/qproperty_p.h>

#include "qserialport.h"
#include "qserialportbackend_p.h"

constexpr qint64 DEFAULT_WRITE_BUFFER_SIZE = 16 * 1024;
// Best-effort send of buffered writes when closing; a stalled link must not delay close.
constexpr int CLOSE_WRITE_TIMEOUT = 100;

#ifndef QSERIALPORT_BUFFERSIZE
#define QSERIALPORT_BUFFERSIZE DEFAULT_WRITE_BUFFER_SIZE
#endif

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)

QT_BEGIN_NAMESPACE

class QSocketNotifier;

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

    void flushBeforeClose();
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

    /// Takes input from the backend, on any thread, and returns whether the port has room for more. If not, the
    /// backend stops reading the device, which then holds its input back, until the port calls its
    /// startReading().
    bool newDataArrived(const char* bytes, int length);
    /// Returns whether the port has room for more input, on any thread.
    bool hasRoomForInput();
    /// Returns whether input has arrived that the read buffer has room for and that is not yet in it.
    bool inputArrived();
    /// Blocks until newDataArrived(), writeCompleted() or interruptWait() has been called or the deadline has
    /// passed, unless what one of them brought is still waiting.
    void waitForArrivedInput(const QDeadlineTimer& deadline);
    /// Takes the backend's report, on any thread, that it has written this many of the bytes it took, or with 0
    /// that it can take more.
    void writeCompleted(qint64 bytes);
    /// Blocks until writeCompleted() or interruptWait() has been called or the deadline has passed, unless the
    /// report of either is still waiting.
    void waitForWriteProgress(const QDeadlineTimer& deadline);
    /// Ends the wait that waitForArrivedInput() or waitForWriteProgress() is in, or else the next one, on any
    /// thread. For a backend that has something else to tell the port's thread, which runs no event loop while
    /// it waits.
    void interruptWait();

    static QList<qint32> standardBaudRates();

    QString systemLocation;
    qint32 inputBaudRate = QSerialPort::Baud9600;
    qint32 outputBaudRate = QSerialPort::Baud9600;
    qint64 readBufferMaxSize = 0;
    int descriptor = -1;

private:
    /// The backend for a port of this name: USB if Java names its ports like it, otherwise POSIX.
    QSerialPortBackend* _backendFor(const QString& location);
    /// The backend of the open port. If the port is not open, sets NotOpenError and returns null.
    QSerialPortBackend* _openBackend();
    void _scheduleWrite();
    /// Accounts for what the backend has written, announces it, and hands the backend more output unless it
    /// still has some. Never waits. Returns false after an error.
    bool _pumpWrites();
    /// Writes until no output is left or the deadline has passed. Returns false after an error, which a passed
    /// deadline is not.
    bool _writeUntil(const QDeadlineTimer& deadline);
    bool _stopAsyncRead();
    void _scheduleReadyRead();
    void _emitReadyRead();
    /// Announces input that was moved into the read buffer, once the backend reads on if that made room.
    void _announceTakenInput();
    /// Has the backend read on if it was told that the port had no room and the port has room again. Returns
    /// false after an error.
    bool _readOnIfRoom();
    bool _hasRoomForInputLocked() const;
    bool _canTakeInputLocked() const;
    /// Moves input that has arrived into the read buffer, as far as that has room, and returns whether it moved
    /// any. The caller announces it.
    bool _takeInputLocked();
    bool _takeArrivedInput();
    qsizetype _pendingSizeLocked() const;
    void _compactPendingDataLocked();
    void _discardPendingDataLocked();
    qint64 _drainPendingDataLocked(qint64 maxBytes = -1);
    /// Reports a failure as one error, described by failureText if the backend cannot tell why it failed.
    bool _setParameters(qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                        QSerialPort::Parity parity, const QString& failureText);

    // Each is made when first needed and then kept. A backend reports an error from inside its own function,
    // and the port's owner may close the port in response, so a close must not destroy it.
    std::unique_ptr<QSerialPortBackend> _usbBackend;
    std::unique_ptr<QSerialPortBackend> _posixBackend;
    // The backend of the open port, or null.
    QSerialPortBackend* _backend = nullptr;

    std::atomic<bool> _readyReadPending{false};
    bool _emittingReadyRead = false;
    std::atomic<bool> _writePending{false};
    // Guard, and announce, what other threads bring: input that arrived and reports of output written.
    QMutex _arrivalMutex;
    QWaitCondition _arrivalCondition;
    // Input that has arrived and is not yet in the read buffer. A read buffer size limits it as well: when it
    // reaches the limit, the backend is told that the port has no room.
    QByteArray _pendingData;
    qsizetype _pendingDataOffset = 0;
    // Whether the backend was last told that the port had no room, and has not been asked to read since.
    bool _readingHeldBack = false;
    // What writeCompleted() has been told since _pumpWrites() last looked, and whether it was called at all.
    qint64 _completedWriteBytes = 0;
    bool _writeProgress = false;
    // Whether interruptWait() has been called since a wait last ended.
    bool _waitInterrupted = false;

    // Output that the backend has taken and not yet reported written. bytesToWrite() counts it.
    qint64 _bytesInFlight = 0;
    // Output that clear() gave up while the backend had it. The backend still reports all of it, sent or not,
    // which the port does not announce.
    qint64 _discardedInFlight = 0;
};

QT_END_NAMESPACE
