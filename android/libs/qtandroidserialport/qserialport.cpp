/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2012 Andre Hartmann <aha_1980@gmx.de>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialport.h"
#include "qserialportinfo.h"
#include "qserialportinfo_p.h"

#include "qserialport_android_p.h"

#ifndef SERIALPORT_BUFFERSIZE
#  define SERIALPORT_BUFFERSIZE 16384
#endif

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QSerialPortPrivateData::QSerialPortPrivateData(QSerialPort *q)
    : readBufferMaxSize(0)
    , readBuffer(SERIALPORT_BUFFERSIZE)
    , writeBuffer(SERIALPORT_BUFFERSIZE)
    , error(QSerialPort::NoError)
    , inputBaudRate(9600)
    , outputBaudRate(9600)
    , dataBits(QSerialPort::Data8)
    , parity(QSerialPort::NoParity)
    , stopBits(QSerialPort::OneStop)
    , flowControl(QSerialPort::NoFlowControl)
#if QT_DEPRECATED_SINCE(5, 2)
    , policy(QSerialPort::IgnorePolicy)
#endif
#if QT_DEPRECATED_SINCE(5,3)
    , settingsRestoredOnClose(true)
#endif
    , q_ptr(q)
{
}

int QSerialPortPrivateData::timeoutValue(int msecs, int elapsed)
{
    if (msecs == -1)
        return msecs;
    msecs -= elapsed;
    return qMax(msecs, 0);
}

QSerialPort::QSerialPort(QObject *parent)
    : QIODevice(parent)
    , d_ptr(new QSerialPortPrivate(this))
{}

QSerialPort::QSerialPort(const QString &name, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new QSerialPortPrivate(this))
{
    setPortName(name);
}

QSerialPort::QSerialPort(const QSerialPortInfo &serialPortInfo, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new QSerialPortPrivate(this))
{
    setPort(serialPortInfo);
}

QSerialPort::~QSerialPort()
{
    /**/
    if (isOpen())
        close();
    delete d_ptr;
}

void QSerialPort::setPortName(const QString &name)
{
    Q_D(QSerialPort);
    d->systemLocation = QSerialPortInfoPrivate::portNameToSystemLocation(name);
}

void QSerialPort::setPort(const QSerialPortInfo &serialPortInfo)
{
    Q_D(QSerialPort);
    d->systemLocation = serialPortInfo.systemLocation();
}

QString QSerialPort::portName() const
{
    Q_D(const QSerialPort);
    return QSerialPortInfoPrivate::portNameFromSystemLocation(d->systemLocation);
}

bool QSerialPort::open(OpenMode mode)
{
    Q_D(QSerialPort);

    if (isOpen()) {
        setError(QSerialPort::OpenError);
        return false;
    }

    // Define while not supported modes.
    static const OpenMode unsupportedModes = Append | Truncate | Text | Unbuffered;
    if ((mode & unsupportedModes) || mode == NotOpen) {
        setError(QSerialPort::UnsupportedOperationError);
        return false;
    }

    clearError();
    if (!d->open(mode))
        return false;

    if (!d->setBaudRate()
        || !d->setDataBits(d->dataBits)
        || !d->setParity(d->parity)
        || !d->setStopBits(d->stopBits)
        || !d->setFlowControl(d->flowControl)) {
        d->close();
        return false;
    }

    QIODevice::open(mode);
    return true;
}

void QSerialPort::close()
{
    Q_D(QSerialPort);
    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        return;
    }

    QIODevice::close();
    d->close();
}

#if QT_DEPRECATED_SINCE(5,3)
void QSerialPort::setSettingsRestoredOnClose(bool restore)
{
    Q_D(QSerialPort);

    if (d->settingsRestoredOnClose != restore) {
        d->settingsRestoredOnClose = restore;
        emit settingsRestoredOnCloseChanged(d->settingsRestoredOnClose);
    }
}

bool QSerialPort::settingsRestoredOnClose() const
{
    Q_D(const QSerialPort);
    return d->settingsRestoredOnClose;
}
#endif // QT_DEPRECATED_SINCE(5,3)

bool QSerialPort::setBaudRate(qint32 baudRate, Directions directions)
{
    Q_D(QSerialPort);

    if (!isOpen() || d->setBaudRate(baudRate, directions)) {
        if (directions & QSerialPort::Input) {
            if (d->inputBaudRate != baudRate)
                d->inputBaudRate = baudRate;
            else
                directions &= ~QSerialPort::Input;
        }

        if (directions & QSerialPort::Output) {
            if (d->outputBaudRate != baudRate)
                d->outputBaudRate = baudRate;
            else
                directions &= ~QSerialPort::Output;
        }

        if (directions)
            emit baudRateChanged(baudRate, directions);

        return true;
    }

    return false;
}

qint32 QSerialPort::baudRate(Directions directions) const
{
    Q_D(const QSerialPort);
    if (directions == QSerialPort::AllDirections)
        return d->inputBaudRate == d->outputBaudRate ?
                    d->inputBaudRate : -1;
    return directions & QSerialPort::Input ? d->inputBaudRate : d->outputBaudRate;
}

bool QSerialPort::setDataBits(DataBits dataBits)
{
    Q_D(QSerialPort);

    if (!isOpen() || d->setDataBits(dataBits)) {
        if (d->dataBits != dataBits) {
            d->dataBits = dataBits;
            emit dataBitsChanged(d->dataBits);
        }
        return true;
    }

    return false;
}

QSerialPort::DataBits QSerialPort::dataBits() const
{
    Q_D(const QSerialPort);
    return d->dataBits;
}

bool QSerialPort::setParity(Parity parity)
{
    Q_D(QSerialPort);

    if (!isOpen() || d->setParity(parity)) {
        if (d->parity != parity) {
            d->parity = parity;
            emit parityChanged(d->parity);
        }
        return true;
    }

    return false;
}

QSerialPort::Parity QSerialPort::parity() const
{
    Q_D(const QSerialPort);
    return d->parity;
}

bool QSerialPort::setStopBits(StopBits stopBits)
{
    Q_D(QSerialPort);

    if (!isOpen() || d->setStopBits(stopBits)) {
        if (d->stopBits != stopBits) {
            d->stopBits = stopBits;
            emit stopBitsChanged(d->stopBits);
        }
        return true;
    }

    return false;
}

QSerialPort::StopBits QSerialPort::stopBits() const
{
    Q_D(const QSerialPort);
    return d->stopBits;
}

bool QSerialPort::setFlowControl(FlowControl flowControl)
{
    Q_D(QSerialPort);

    if (!isOpen() || d->setFlowControl(flowControl)) {
        if (d->flowControl != flowControl) {
            d->flowControl = flowControl;
            emit flowControlChanged(d->flowControl);
        }
        return true;
    }

    return false;
}

QSerialPort::FlowControl QSerialPort::flowControl() const
{
    Q_D(const QSerialPort);
    return d->flowControl;
}

bool QSerialPort::setDataTerminalReady(bool set)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    const bool dataTerminalReady = isDataTerminalReady();
    const bool retval = d->setDataTerminalReady(set);
    if (retval && (dataTerminalReady != set))
        emit dataTerminalReadyChanged(set);

    return retval;
}

bool QSerialPort::isDataTerminalReady()
{
    Q_D(QSerialPort);
    return d->pinoutSignals() & QSerialPort::DataTerminalReadySignal;
}

bool QSerialPort::setRequestToSend(bool set)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    const bool requestToSend = isRequestToSend();
    const bool retval = d->setRequestToSend(set);
    if (retval && (requestToSend != set))
        emit requestToSendChanged(set);

    return retval;
}

bool QSerialPort::isRequestToSend()
{
    Q_D(QSerialPort);
    return d->pinoutSignals() & QSerialPort::RequestToSendSignal;
}

QSerialPort::PinoutSignals QSerialPort::pinoutSignals()
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return QSerialPort::NoSignal;
    }

    return d->pinoutSignals();
}

bool QSerialPort::flush()
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    return d->flush();
}

bool QSerialPort::clear(Directions directions)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    if (directions & Input)
        d->readBuffer.clear();
    if (directions & Output)
        d->writeBuffer.clear();
    return d->clear(directions);
}

bool QSerialPort::atEnd() const
{
    Q_D(const QSerialPort);
    return QIODevice::atEnd() && (!isOpen() || (d->readBuffer.size() == 0));
}

#if QT_DEPRECATED_SINCE(5, 2)
bool QSerialPort::setDataErrorPolicy(DataErrorPolicy policy)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    const bool ret = d->policy == policy || d->setDataErrorPolicy(policy);
    if (ret && (d->policy != policy)) {
        d->policy = policy;
        emit dataErrorPolicyChanged(d->policy);
    }

    return ret;
}

QSerialPort::DataErrorPolicy QSerialPort::dataErrorPolicy() const
{
    Q_D(const QSerialPort);
    return d->policy;
}
#endif // QT_DEPRECATED_SINCE(5, 2)

QSerialPort::SerialPortError QSerialPort::error() const
{
    Q_D(const QSerialPort);
    return d->error;
}

void QSerialPort::clearError()
{
    setError(QSerialPort::NoError);
}

qint64 QSerialPort::readBufferSize() const
{
    Q_D(const QSerialPort);
    return d->readBufferMaxSize;
}

void QSerialPort::setReadBufferSize(qint64 size)
{
    Q_D(QSerialPort);

    if (d->readBufferMaxSize == size)
        return;
    d->readBufferMaxSize = size;
}

bool QSerialPort::isSequential() const
{
    return true;
}

qint64 QSerialPort::bytesAvailable() const
{
    Q_D(const QSerialPort);
    return d->readBuffer.size() + QIODevice::bytesAvailable();
}

qint64 QSerialPort::bytesToWrite() const
{
    Q_D(const QSerialPort);
    return d->bytesToWrite() + QIODevice::bytesToWrite();
}

bool QSerialPort::canReadLine() const
{
    Q_D(const QSerialPort);
    const bool hasLine = (d->readBuffer.size() > 0) && d->readBuffer.canReadLine();
    return hasLine || QIODevice::canReadLine();
}

bool QSerialPort::waitForReadyRead(int msecs)
{
    Q_D(QSerialPort);
    return d->waitForReadyRead(msecs);
}

bool QSerialPort::waitForBytesWritten(int msecs)
{
    Q_D(QSerialPort);
    return d->waitForBytesWritten(msecs);
}

bool QSerialPort::sendBreak(int duration)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    return d->sendBreak(duration);
}

bool QSerialPort::setBreakEnabled(bool set)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        setError(QSerialPort::NotOpenError);
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    return d->setBreakEnabled(set);
}

qint64 QSerialPort::readData(char *data, qint64 maxSize)
{
    Q_D(QSerialPort);
#ifdef Q_OS_ANDROID
        qint64 retL = d->readBuffer.read(data, maxSize);
        d->startReadThread();
        return retL;
#else
        return d->readData(data, maxSize);
#endif
}

qint64 QSerialPort::readLineData(char *data, qint64 maxSize)
{
#ifdef Q_OS_ANDROID
        qint64 retL = QIODevice::readLineData(data, maxSize);
        Q_D(QSerialPort);
        d->startReadThread();
        return retL;
#else
       return QIODevice::readLineData(data, maxSize);
#endif
}

qint64 QSerialPort::writeData(const char *data, qint64 maxSize)
{
    Q_D(QSerialPort);
    return d->writeData(data, maxSize);
}

void QSerialPort::setError(QSerialPort::SerialPortError serialPortError, const QString &errorString)
{
    Q_D(QSerialPort);

    d->error = serialPortError;

    if (errorString.isNull())
        setErrorString(qt_error_string(-1));
    else
        setErrorString(errorString);

    emit errorOccurred(serialPortError);
}

#include "moc_qserialport.cpp"

QT_END_NAMESPACE
